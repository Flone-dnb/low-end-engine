#include "game/node/SkeletonNode.h"

// External.
#include "nameof.hpp"
#include "ozz/base/io/stream.h"
#include "ozz/base/io/archive.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/local_to_model_job.h"

namespace {
    constexpr std::string_view sTypeGuid = "385659e9-bd1a-4ebd-a92a-67e2ba657d4d";
}

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

void SkeletonNode::setAnimationPlaybackSpeed(float speed) { playbackSpeed = speed; }

void SkeletonNode::stopAnimation() {
    if (pPlayingAnimation == nullptr) {
        return;
    }

    pPlayingAnimation = nullptr;
    animationRatio = 0.0F;
    setRestPoseToBoneMatrices();
}

ozz::animation::Animation* SkeletonNode::findOrLoadAnimation(const std::string& sRelativePathToAnimation) {
    ozz::animation::Animation* pAnimation = nullptr;

    const auto animationIt = loadedAnimations.find(sRelativePathToAnimation);
    if (animationIt == loadedAnimations.end()) {
        // Construct full path.
        const auto pathToAnimationFile =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToAnimation;
        if (!std::filesystem::exists(pathToAnimationFile)) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("path to animation \"{}\" does not exist", sRelativePathToAnimation));
        }

        // Load animation.
        auto pAnimationUptr =
            loadAnimation(pathToAnimationFile, static_cast<unsigned int>(pSkeleton->num_joints()));
        pAnimation = pAnimationUptr.get();
        loadedAnimations[sRelativePathToAnimation] = std::move(pAnimationUptr);
    } else {
        pAnimation = animationIt->second.get();
    }

    return pAnimation;
}

void SkeletonNode::playAnimation(const std::string& sRelativePathToAnimation, bool bLoop, bool bRestart) {
    if (pSkeleton == nullptr) {
        return;
    }

    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "this function should only be called while the node is spawned (node \"{}\"", getNodeName()));
    }

    const auto pPreviousPlayingAnimation = pPlayingAnimation;
    pPlayingAnimation = findOrLoadAnimation(sRelativePathToAnimation);

    bLoopAnimation = bLoop;

    if (pPreviousPlayingAnimation == pPlayingAnimation && !bRestart) {
        return;
    }

    animationRatio = 0.0F;
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

    if (pPlayingAnimation == nullptr) {
        return;
    }

    // Update current animation position.
    animationRatio += timeSincePrevFrameInSec * playbackSpeed / pPlayingAnimation->duration();
    if (bLoopAnimation) {
        // Wraps in [0; 1] interval.
        const float loopCount = std::floor(animationRatio);
        animationRatio = animationRatio - loopCount;
    } else {
        // Clamp to [0; 1] interval.
        animationRatio = std::clamp(animationRatio, 0.0F, 1.0F);
    }

    // Sample bone local transforms.
    ozz::animation::SamplingJob samplingJob;
    samplingJob.animation = pPlayingAnimation;
    samplingJob.context = &samplingJobContext;
    samplingJob.ratio = animationRatio;
    samplingJob.output = ozz::make_span(vLocalTransforms);
    if (!samplingJob.Run()) {
        Log::error(std::format("skeleton sampling job failed for node \"{}\"", getNodeName()));
        return;
    }

    convertLocalTransformsToSkinningMatrices();
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
    const auto pathToSkeletonDirectory = pathToSkeletonFile.parent_path();
    for (const auto& sRelativePath : pathsToAnimationsToPreload) {
        // Construct full path.
        const auto pathToAnimationFile = pathToSkeletonDirectory / (sRelativePath + ".ozz");
        if (!std::filesystem::exists(pathToAnimationFile)) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "path to animation \"{}\" results in the full path of \"{}\" which does not exist",
                sRelativePath,
                pathToAnimationFile.string()));
        }

        // Load animation.
        loadedAnimations[sRelativePath] =
            loadAnimation(pathToAnimationFile, static_cast<unsigned int>(pSkeleton->num_joints()));
    }
    pathsToAnimationsToPreload.clear();

    // Allocate matrices.
    vLocalTransforms.resize(static_cast<size_t>(pSkeleton->num_soa_joints()));
    vBoneMatrices.resize(static_cast<size_t>(pSkeleton->num_joints()));
    vSkinningMatrices.resize(vBoneMatrices.size());
    if (vInverseBindPoseMatrices.size() != vSkinningMatrices.size()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "skeleton bone matrix mismatch {} != {}",
            vInverseBindPoseMatrices.size(),
            vSkinningMatrices.size()));
    }
    setRestPoseToBoneMatrices();

    // Create sampling job context.
    samplingJobContext.Resize(pSkeleton->num_joints());
}

void SkeletonNode::unloadAnimationContextData() {
    pPlayingAnimation = nullptr;

    pSkeleton = nullptr;
    loadedAnimations.clear();

    vLocalTransforms.clear();
    vLocalTransforms.shrink_to_fit();

    vBoneMatrices.clear();
    vBoneMatrices.shrink_to_fit();

    vInverseBindPoseMatrices.clear();
    vInverseBindPoseMatrices.shrink_to_fit();

    vSkinningMatrices.clear();
    vSkinningMatrices.shrink_to_fit();
}

void SkeletonNode::setRestPoseToBoneMatrices() {
    if (pSkeleton == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the skeleton to be loaded while calling this function");
    }

    ozz::vector<ozz::math::Transform> transforms(static_cast<size_t>(pSkeleton->num_joints()));
    for (size_t i = 0; i < vLocalTransforms.size(); ++i) {
        vLocalTransforms[i] = pSkeleton->joint_rest_poses()[i];
    }

    convertLocalTransformsToSkinningMatrices();
}

void SkeletonNode::convertLocalTransformsToSkinningMatrices() {
    PROFILE_FUNC

    if (pSkeleton == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the skeleton to be loaded while calling this function");
    }

    // Convert local space matrices to model space.
    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = pSkeleton.get();
    localToModelJob.input = ozz::make_span(vLocalTransforms);
    localToModelJob.output = ozz::make_span(vBoneMatrices);
    if (!localToModelJob.Run()) {
        Log::error(std::format(
            "failed to convert bone local space matrices to model space for node \"{}\"", getNodeName()));
        return;
    }

    for (size_t i = 0; i < vBoneMatrices.size(); i++) {
        glm::mat4x4 boneMatrix = glm::identity<glm::mat4x4>();
        for (int k = 0; k < 4; k++) {
            ozz::math::StorePtr(vBoneMatrices[i].cols[k], &boneMatrix[k].x);
        }

        vSkinningMatrices[i] = vInverseBindPoseMatrices[i] * boneMatrix;
    }
}

float SkeletonNode::getCurrentAnimationDurationSec() const {
    if (pPlayingAnimation == nullptr) {
        return 0.0F;
    }

    return pPlayingAnimation->duration();
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

std::unique_ptr<ozz::animation::Animation>
SkeletonNode::loadAnimation(const std::filesystem::path& pathToAnimation, unsigned int iSkeletonBoneCount) {
    std::string sFullPathToAnimationFile = pathToAnimation.string();

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
    if (static_cast<unsigned int>(pAnimation->num_tracks()) != iSkeletonBoneCount) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "animation \"{}\" is not compatible with the skeleton, animation has {} track(s) and skeleton {} "
            "bone(s) these numbers need to match",
            pathToAnimation.string(),
            pAnimation->num_tracks(),
            iSkeletonBoneCount));
    }

    return pAnimation;
}
