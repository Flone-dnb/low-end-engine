#include "game/node/SkeletonNode.h"

// External.
#include "nameof.hpp"
#include "ozz/base/io/stream.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/span.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/blending_job.h"

namespace {
    constexpr std::string_view sTypeGuid = "385659e9-bd1a-4ebd-a92a-67e2ba657d4d";
}

AnimationSampler::AnimationSampler(
    std::unique_ptr<ozz::animation::Animation> pInAnimation, ozz::animation::Skeleton* pSkeleton)
    : pSkeleton(pSkeleton) {
    pAnimation = std::move(pInAnimation);

    pSamplingJobContext = std::make_unique<ozz::animation::SamplingJob::Context>();
    pSamplingJobContext->Resize(pSkeleton->num_joints());

    vLocalTransforms.resize(static_cast<size_t>(pSkeleton->num_soa_joints()));
    for (size_t i = 0; i < vLocalTransforms.size(); i++) {
        vLocalTransforms[i] = pSkeleton->joint_rest_poses()[i];
    }
}

void AnimationSampler::prepareForPlaying(bool bLoop) {
    for (size_t i = 0; i < vLocalTransforms.size(); i++) {
        vLocalTransforms[i] = pSkeleton->joint_rest_poses()[i];
    }

    animationRatio = 0.0F;
    playbackSpeed = 1.0F;
    weight = 1.0F;
    bLoopAnimation = bLoop;
}

void AnimationSampler::updateAnimation(float deltaTime, bool bSampleBoneMatrices) {
    animationRatio += deltaTime * playbackSpeed / pAnimation->duration();
    if (bLoopAnimation) {
        // Wraps in [0; 1] interval.
        const float loopCount = std::floor(animationRatio);
        animationRatio = animationRatio - loopCount;
    } else {
        // Clamp to [0; 1] interval.
        animationRatio = std::clamp(animationRatio, 0.0F, 1.0F);
    }

    if (!bSampleBoneMatrices) {
        return;
    }

    // Sample bone local transforms.
    ozz::animation::SamplingJob samplingJob;
    samplingJob.animation = pAnimation.get();
    samplingJob.context = pSamplingJobContext.get();
    samplingJob.ratio = animationRatio;
    samplingJob.output = ozz::make_span(vLocalTransforms);
    if (!samplingJob.Run()) [[unlikely]] {
        Error::showErrorAndThrowException("skeleton sampling job failed for node \"{}\"");
    }
}

float AnimationSampler::getDuration() const { return pAnimation->duration(); }

std::string SkeletonNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SkeletonNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SkeletonNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.strings[NAMEOF_MEMBER(&SkeletonNode::sPathToSkeletonRelativeRes).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<SkeletonNode*>(pThis)->setPathToSkeletonRelativeRes(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<SkeletonNode*>(pThis)->getPathToSkeletonRelativeRes();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SkeletonNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SkeletonNode>(); },
        std::move(variables));
}

SkeletonNode::SkeletonNode() : SkeletonNode("Skeleton Node") {}

SkeletonNode::SkeletonNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    setIsCalledEveryFrame(true);
}

void SkeletonNode::setPathToSkeletonRelativeRes(std::string sPathToNewSkeleton) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToNewSkeleton.size(); i++) {
        if (sPathToNewSkeleton[i] == '\\') {
            sPathToNewSkeleton[i] = '/';
        }
    }

    if (sPathToSkeletonRelativeRes == sPathToNewSkeleton) {
        return;
    }

    // Make sure the path is valid.
    const auto pathToFile = ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToNewSkeleton;
    if (!std::filesystem::exists(pathToFile)) {
        Log::error(std::format("path \"{}\" does not exist", pathToFile.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToFile)) {
        Log::error(std::format("expected the path \"{}\" to point to a file", pathToFile.string()));
        return;
    }

    sPathToSkeletonRelativeRes = sPathToNewSkeleton;

    if (isSpawned()) {
        if (pSkeleton != nullptr) {
            unloadAnimationContextData();
        }
        loadAnimationContextData();
    }
}

void SkeletonNode::addPathToAnimationToPreload(std::string& sRelativePathToAnimation) {
    // Normalize slash.
    for (size_t i = 0; i < sRelativePathToAnimation.size(); i++) {
        if (sRelativePathToAnimation[i] == '\\') {
            sRelativePathToAnimation[i] = '/';
        }
    }

    if (!isSpawned() || pSkeleton == nullptr) {
        pathsToAnimationsToPreload.insert(sRelativePathToAnimation);
        return;
    }

    findOrLoadAnimation(sRelativePathToAnimation);
}

void SkeletonNode::stopAnimation() {
    animState.vPlayingAnimations.clear();

    // Set rest pose.
    for (size_t i = 0; i < vResultingLocalTransforms.size(); i++) {
        vResultingLocalTransforms[i] = pSkeleton->joint_rest_poses()[i];
    }
    convertResultingLocalTransformsToSkinning();
}

AnimationSampler* SkeletonNode::findOrLoadAnimation(const std::string& sRelativePathToAnimation) {
    AnimationSampler* pAnimationSampler = nullptr;

    auto animationIt = loadedAnimations.find(sRelativePathToAnimation);
    if (animationIt == loadedAnimations.end()) {
        // Construct full path.
        const auto pathToAnimationFile =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToAnimation;
        if (!std::filesystem::exists(pathToAnimationFile)) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("path to animation \"{}\" does not exist", sRelativePathToAnimation));
        }

        loadAnimation(sRelativePathToAnimation, pSkeleton.get());
        animationIt = loadedAnimations.find(sRelativePathToAnimation);
        if (animationIt == loadedAnimations.end()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected the animation for \"{}\" to be loaded", sRelativePathToAnimation));
        }
    }

    pAnimationSampler = animationIt->second.get();

    return pAnimationSampler;
}

void SkeletonNode::playAnimation(const std::string& sRelativePathToAnimation, bool bLoop) {
    if (pSkeleton == nullptr) {
        return;
    }

    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "this function should only be called while the node is spawned (node \"{}\"", getNodeName()));
    }

    auto pSampler = findOrLoadAnimation(sRelativePathToAnimation);

    // Playing a single animation (without blending).
    animState.vPlayingAnimations.clear();
    pSampler->prepareForPlaying(bLoop);
    animState.vPlayingAnimations.push_back(pSampler);
}

void SkeletonNode::setBlendFactor(float blendFactor) { animState.blendFactor = blendFactor; }

void SkeletonNode::playBlendedAnimations(
    const std::vector<std::string>& vRelativePathsToAnimations, float blendFactor) {
    if (vRelativePathsToAnimations.size() == 1) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("this function expects that at least 2 animations will be specified"));
    }

    animState.vPlayingAnimations.clear();
    animState.blendFactor = blendFactor;
    for (const auto& sRelativePathToAnimation : vRelativePathsToAnimations) {
        auto pSampler = findOrLoadAnimation(sRelativePathToAnimation);
        pSampler->prepareForPlaying(true);
        animState.vPlayingAnimations.push_back(pSampler);
    }
}

void SkeletonNode::onSpawning() {
    SpatialNode::onSpawning();

    if (sPathToSkeletonRelativeRes.empty()) {
        Log::warn(std::format(
            "path to skeleton file was not specified for node \"{}\", node will do nothing", getNodeName()));
        return;
    }

    loadAnimationContextData();
}

void SkeletonNode::onDespawning() {
    SpatialNode::onDespawning();

    unloadAnimationContextData();
}

void SkeletonNode::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    SpatialNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    if (animState.vPlayingAnimations.empty()) {
        return;
    }

    if (animState.vPlayingAnimations.size() > 1) {
        // Calculate weights for blending.
        const size_t iIntervalCount = animState.vPlayingAnimations.size() - 1;
        const float intervalSize = 1.0F / static_cast<float>(iIntervalCount);
        for (size_t i = 0; i < animState.vPlayingAnimations.size(); ++i) {
            const float intervalStart = static_cast<float>(i) * intervalSize;
            float weight = animState.blendFactor - intervalStart;
            weight = ((weight < 0.0F ? weight : -weight) + intervalSize) * static_cast<float>(iIntervalCount);
            animState.vPlayingAnimations[i]->setWeight(ozz::math::Max(0.0F, weight));
        }

        // Selects 2 samplers that define interval that contains blend factor.
        const float clampedFactor = ozz::math::Clamp(0.0F, animState.blendFactor, 0.999F);
        const size_t iLeftSamplerIndex =
            static_cast<size_t>(clampedFactor * static_cast<float>(iIntervalCount));
        const auto pLeftSampler = animState.vPlayingAnimations[iLeftSamplerIndex];
        const auto pRightSampler = animState.vPlayingAnimations[iLeftSamplerIndex + 1];

        // Interpolate animation duration using weights to find loop cycle duration.
        const float loopDuration = pLeftSampler->getDuration() * pLeftSampler->getWeight() +
                                   pRightSampler->getDuration() * pRightSampler->getWeight();

        // Calculate speed for all samplers.
        const float invLoopDuration = 1.0F / loopDuration;
        for (const auto& pSampler : animState.vPlayingAnimations) {
            const float speed = pSampler->getDuration() * invLoopDuration;
            pSampler->setPlaybackSpeed(speed);
        }
    }

    // Update each playing animation (no blending yet).
    for (auto& pSampler : animState.vPlayingAnimations) {
        pSampler->updateAnimation(timeSincePrevFrameInSec, pSampler->getWeight() > 0.0F);
    }

    if (animState.vPlayingAnimations.size() > 1) {
        // Prepare for blending.
        ozz::vector<ozz::animation::BlendingJob::Layer> vLayers(animState.vPlayingAnimations.size());
        for (size_t i = 0; i < vLayers.size(); ++i) {
            vLayers[i].transform = ozz::make_span(animState.vPlayingAnimations[i]->getLocalTransforms());
            vLayers[i].weight = animState.vPlayingAnimations[i]->getWeight();
        }

        ozz::animation::BlendingJob blend_job;
        blend_job.threshold = ozz::animation::BlendingJob().threshold;
        blend_job.layers = ozz::make_span(vLayers);
        blend_job.rest_pose = pSkeleton->joint_rest_poses();
        blend_job.output = ozz::make_span(vResultingLocalTransforms);
    } else {
        vResultingLocalTransforms = animState.vPlayingAnimations[0]->getLocalTransforms();
    }

    convertResultingLocalTransformsToSkinning();
}

void SkeletonNode::convertResultingLocalTransformsToSkinning() {
    // Convert local space matrices to model space.
    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = pSkeleton.get();
    localToModelJob.input = ozz::make_span(vResultingLocalTransforms);
    localToModelJob.output = ozz::make_span(vBoneMatrices);
    if (!localToModelJob.Run()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to convert bone local space matrices to model space for node \"{}\"", getNodeName()));
    }

    for (size_t i = 0; i < vBoneMatrices.size(); i++) {
        glm::mat4x4 boneMatrix = glm::identity<glm::mat4x4>();
        for (int k = 0; k < 4; k++) {
            ozz::math::StorePtr(vBoneMatrices[i].cols[k], &boneMatrix[k].x);
        }

        vSkinningMatrices[i] = vInverseBindPoseMatrices[i] * boneMatrix;
    }
}

void SkeletonNode::loadAnimationContextData() {
    if (sPathToSkeletonRelativeRes.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected path to the skeleton to be valid, node \"{}\"", getNodeName()));
    }

    // Load skeleton.
    const auto pathToSkeletonFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToSkeletonRelativeRes;
    if (!std::filesystem::exists(pathToSkeletonFile)) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected path to skeleton to exist \"{}\"", pathToSkeletonFile.string()));
    }
    pSkeleton = loadSkeleton(pathToSkeletonFile, vInverseBindPoseMatrices);

    // Preload some animations.
    for (const auto& sRelativePath : pathsToAnimationsToPreload) {
        loadAnimation(sRelativePath, pSkeleton.get());
    }
    pathsToAnimationsToPreload.clear();

    // Allocate matrices.
    vResultingLocalTransforms.resize(static_cast<size_t>(pSkeleton->num_soa_joints()));
    vBoneMatrices.resize(static_cast<size_t>(pSkeleton->num_joints()));
    vSkinningMatrices.resize(vBoneMatrices.size());
    if (vInverseBindPoseMatrices.size() != vSkinningMatrices.size()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "skeleton bone matrix mismatch {} != {}",
            vInverseBindPoseMatrices.size(),
            vSkinningMatrices.size()));
    }

    // Set rest pose.
    if (pSkeleton->joint_rest_poses().size() != vResultingLocalTransforms.size()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "mismatched local transform count {} != {}",
            pSkeleton->joint_rest_poses().size(),
            vResultingLocalTransforms.size()));
    }
    for (size_t i = 0; i < vResultingLocalTransforms.size(); i++) {
        vResultingLocalTransforms[i] = pSkeleton->joint_rest_poses()[i];
    }
    convertResultingLocalTransformsToSkinning();
}

void SkeletonNode::unloadAnimationContextData() {
    animState.vPlayingAnimations.clear();

    pSkeleton = nullptr;
    loadedAnimations.clear();

    vResultingLocalTransforms.clear();
    vResultingLocalTransforms.shrink_to_fit();

    vBoneMatrices.clear();
    vBoneMatrices.shrink_to_fit();

    vInverseBindPoseMatrices.clear();
    vInverseBindPoseMatrices.shrink_to_fit();

    vSkinningMatrices.clear();
    vSkinningMatrices.shrink_to_fit();
}

std::unique_ptr<ozz::animation::Skeleton> SkeletonNode::loadSkeleton(
    const std::filesystem::path& pathToSkeleton, std::vector<glm::mat4x4>& vInverseBindPoseMatrices) {
    unsigned int iBoneCount = 0;
    std::unique_ptr<ozz::animation::Skeleton> pSkeleton;
    {
        // Open file.
        const std::string sFullPathToSkeletonFile = pathToSkeleton.string();
        ozz::io::File file(sFullPathToSkeletonFile.c_str(), "rb");
        if (!file.opened()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("unable to open the skeleton file \"{}\"", sFullPathToSkeletonFile));
        }
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Skeleton>()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "the skeleton file does not seem to store a skeleton \"{}\"", sFullPathToSkeletonFile));
        }

        // Create skeleton.
        pSkeleton = std::make_unique<ozz::animation::Skeleton>();
        archive >> *pSkeleton;

        iBoneCount = static_cast<unsigned int>(pSkeleton->num_joints());
        if (iBoneCount > SkeletonNode::getMaxBoneCountAllowed()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "skeleton \"{}\" bone count {} exceeds the maximum allowed bone count of {}",
                sFullPathToSkeletonFile,
                iBoneCount,
                SkeletonNode::getMaxBoneCountAllowed()));
        }
    }

    // Load inverse bind pose matrices.
    {
        // Open file.
        const auto pathToInverseBindPoseFile =
            pathToSkeleton.parent_path() /
            ("skeletonInverseBindPose." + std::string(Serializable::getBinaryFileExtension()));
        std::ifstream file(pathToInverseBindPoseFile.c_str(), std::ios::binary);
        if (!file.is_open()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("unable to open the file \"{}\"", pathToInverseBindPoseFile.string().c_str()));
        }

        // Get file size.
        file.seekg(0, std::ios::end);
        const size_t iFileSizeInBytes = static_cast<size_t>(file.tellg());
        file.seekg(0);

        // Read matrix count.
        size_t iReadByteCount = 0;
        unsigned int iMatrixCount = 0;
        if (iReadByteCount + sizeof(iMatrixCount) > iFileSizeInBytes) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("unexpected end of file \"{}\"", pathToInverseBindPoseFile.string().c_str()));
        }
        file.read(reinterpret_cast<char*>(&iMatrixCount), sizeof(iMatrixCount));
        iReadByteCount += sizeof(iMatrixCount);

        // Check matrix count.
        if (iBoneCount != iMatrixCount) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "skeleton bone count {} does not match inverse bind pose matrix count {}",
                iBoneCount,
                vInverseBindPoseMatrices.size()));
        }

        // Read matrices.
        vInverseBindPoseMatrices.resize(iMatrixCount);
        for (unsigned int i = 0; i < iMatrixCount; i++) {
            if (iReadByteCount + sizeof(glm::mat4x4) > iFileSizeInBytes) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("unexpected end of file \"{}\"", pathToInverseBindPoseFile.string()));
            }

            file.read(
                reinterpret_cast<char*>(glm::value_ptr(vInverseBindPoseMatrices[i])), sizeof(glm::mat4x4));
            iReadByteCount += sizeof(glm::mat4x4);
        }
    }

    return pSkeleton;
}

void SkeletonNode::loadAnimation(
    const std::string& sRelativePathToAnimation, ozz::animation::Skeleton* pSkeleton) {
    if (pSkeleton == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to load animation \"{}\" because the specified skeleton is `nullptr`",
            sRelativePathToAnimation));
    }

    // Construct full path.
    const auto pathToAnimationFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToAnimation;
    if (!std::filesystem::exists(pathToAnimationFile)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "path to animation \"{}\" results in the full path of \"{}\" which does not exist",
            sRelativePathToAnimation,
            pathToAnimationFile.string()));
    }

    if (loadedAnimations.find(sRelativePathToAnimation) != loadedAnimations.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("animation for path \"{}\" is already loaded", sRelativePathToAnimation));
    }

    std::string sFullPathToAnimationFile = pathToAnimationFile.string();

    // Open file.
    ozz::io::File file(sFullPathToAnimationFile.c_str(), "rb");
    if (!file.opened()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to open the animation file \"{}\"", sFullPathToAnimationFile));
    }
    ozz::io::IArchive archive(&file);
    if (!archive.TestTag<ozz::animation::Animation>()) {
        Error::showErrorAndThrowException(std::format(
            "the animation file does not seem to store an animation \"{}\"", sFullPathToAnimationFile));
    }

    // Create animation.
    auto pAnimation = std::make_unique<ozz::animation::Animation>();
    archive >> *pAnimation;

    // Make sure animation is compatible.
    if (pAnimation->num_tracks() != pSkeleton->num_joints()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "animation \"{}\" is not compatible with the skeleton, animation has {} track(s) and "
            "skeleton {} bone(s) these numbers need to match",
            sRelativePathToAnimation,
            pAnimation->num_tracks(),
            pSkeleton->num_joints()));
    }

    loadedAnimations.emplace(
        sRelativePathToAnimation, std::make_unique<AnimationSampler>(std::move(pAnimation), pSkeleton));
}
