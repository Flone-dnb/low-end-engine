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
        Logger::get().error(std::format("path \"{}\" does not exist", pathToFile.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToFile)) {
        Logger::get().error(std::format("expected the path \"{}\" to point to a file", pathToFile.string()));
        return;
    }

    sPathToSkeletonRelativeRes = sPathToNewSkeleton;

    if (isSpawned() && pSkeleton != nullptr) {
        unloadAnimationContextData();
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

void SkeletonNode::playAnimation(const std::string& sRelativePathToAnimation, bool bRestart, bool bLoop) {
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
        Logger::get().warn(std::format(
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
    SpatialNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    if (pPlayingAnimation == nullptr) {
        return;
    }

    // Update current animation position.
    animationRatio += timeSincePrevFrameInSec * playbackSpeed / pPlayingAnimation->duration();
    if (bLoopAnimation) {
        // Wraps in [0; 1] interval.
        const float loopCount = std::floorf(animationRatio);
        animationRatio = animationRatio - loopCount;
    } else {
        // Clamp to [0; 1] interval.
        animationRatio = std::clamp(animationRatio, 0.0F, 1.0F);
    }

    // Sample animation.
    ozz::animation::SamplingJob samplingJob;
    samplingJob.animation = pPlayingAnimation;
    samplingJob.context = &samplingJobContext;
    samplingJob.ratio = animationRatio;
    samplingJob.output = ozz::make_span(vLocalTransforms);
    if (!samplingJob.Run()) {
        Logger::get().error(std::format("skeleton sampling job failed for node \"{}\"", getNodeName()));
        return;
    }

    // Convert local space matrices to model space.
    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = pSkeleton.get();
    localToModelJob.input = ozz::make_span(vLocalTransforms);
    localToModelJob.output = ozz::make_span(vBoneMatrices);
    if (!localToModelJob.Run()) {
        Logger::get().error(std::format(
            "failed to convert bone local space matrices to model space for node \"{}\"", getNodeName()));
        return;
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
    pSkeleton = loadSkeleton(pathToSkeletonFile);

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

    // Initialize model matrices as identity matrices to properly display geometry while no animation is
    // started.
    for (auto& matrix : vBoneMatrices) {
        matrix = ozz::math::Float4x4::identity();
    }

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
}

float SkeletonNode::getCurrentAnimationDurationSec() const {
    if (pPlayingAnimation == nullptr) {
        return 0.0F;
    }

    return pPlayingAnimation->duration();
}

std::unique_ptr<ozz::animation::Skeleton>
SkeletonNode::loadSkeleton(const std::filesystem::path& pathToSkeleton) {
    std::string sFullPathToSkeletonFile = pathToSkeleton.string();

    // Open file.
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
    auto pSkeleton = std::make_unique<ozz::animation::Skeleton>();
    archive >> *pSkeleton;

    if (static_cast<unsigned int>(pSkeleton->num_joints()) > SkeletonNode::getMaxBoneCountAllowed())
        [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "skeleton \"{}\" bone count {} exceeds the maximum allowed bone count of {}",
            sFullPathToSkeletonFile,
            pSkeleton->num_joints(),
            SkeletonNode::getMaxBoneCountAllowed()));
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
