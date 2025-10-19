#pragma once

// Standard.
#include <unordered_set>
#include <unordered_map>
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

// External.
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/containers/vector.h"
#include "ozz/animation/runtime/sampling_job.h"

namespace ozz {
    namespace animation {
        class Skeleton;
        class Animation;
    }
}

/** Groups data needed to sample an animation. */
class AnimationSampler {
public:
    AnimationSampler() = delete;

    /**
     * Initializes the sampler.
     *
     * @param pInAnimation Animation to sample.
     * @param pSkeleton  Skeleton that will be used with the animation.
     */
    AnimationSampler(
        std::unique_ptr<ozz::animation::Animation> pInAnimation, ozz::animation::Skeleton* pSkeleton);

    AnimationSampler(const AnimationSampler&) = delete;
    AnimationSampler& operator=(const AnimationSampler&) = delete;

    /** Move constructor. */
    AnimationSampler(AnimationSampler&&) noexcept = default;

    /** Move operator=. @return This. */
    AnimationSampler& operator=(AnimationSampler&&) noexcept = default;

    /**
     * Resets animation parameters to their defaults (does not remove the loaded animation).
     *
     * @param bLoop `true` to loop the animation.
     */
    void prepareForPlaying(bool bLoop);

    /**
     * Updates playing animation state.
     *
     * @param deltaTime Delta time.
     * @param bSampleBoneMatrices `true` to sample bone matrices (do animation sampling).
     */
    void updateAnimation(float deltaTime, bool bSampleBoneMatrices);

    /**
     * Sets playback speed where 1.0 means default speed.
     *
     * @param speed New speed.
     */
    void setPlaybackSpeed(float speed) { playbackSpeed = speed; }

    /**
     * Sets new weight.
     *
     * @param newWeight Weight.
     */
    void setWeight(float newWeight) { weight = newWeight; }

    /**
     * Returns weight or animation blending in range [0.0; 1.0].
     *
     * @return Weight.
     */
    float getWeight() const { return weight; }

    /**
     * Returns duration of the animation.
     *
     * @return Duration.
     */
    float getDuration() const;

    /**
     * Returns local bone transforms.
     *
     * @return Transforms.
     */
    ozz::vector<ozz::math::SoaTransform>& getLocalTransforms() { return vLocalTransforms; }

private:
    /** Loaded animation. */
    std::unique_ptr<ozz::animation::Animation> pAnimation;

    /** Context for sampling animation state. */
    std::unique_ptr<ozz::animation::SamplingJob::Context> pSamplingJobContext;

    /** Sampled local transforms (relative to parent bone), 1 soa transform can store multiple (4) bones. */
    ozz::vector<ozz::math::SoaTransform> vLocalTransforms;

    /** Skeleton. */
    ozz::animation::Skeleton* pSkeleton = nullptr;

    /** Current position of the animation in range [0; 1] where 0 means animation start and 1 means end. */
    float animationRatio = 0.0F;

    /** Playback speed for the animation. */
    float playbackSpeed = 1.0F;

    /** Weight for animation blending, in range [0.0; 1.0]. */
    float weight = 1.0F;

    /** `true` to loop the animation. */
    bool bLoopAnimation = false;
};

/** Plays animations and moves child nodes SkeletalMeshNode according to their per-vertex weights. */
class SkeletonNode : public SpatialNode {
public:
    SkeletonNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SkeletonNode(const std::string& sNodeName);

    virtual ~SkeletonNode() override = default;

    /**
     * Return the maximum number of bones allowed (per skeleton).
     *
     * @return Bone count.
     */
    static constexpr unsigned int getMaxBoneCountAllowed() { return iMaxBoneCountAllowed; }

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets path to the `skeleton.ozz` file relative to the `res` directory.
     *
     * @param sPathToNewSkeleton Path to file.
     */
    void setPathToSkeletonRelativeRes(std::string sPathToNewSkeleton);

    /**
     * Adds a path to .ozz animation file relative to the `res` directory to load when spawning or (immediatly
     * if already spawned).
     *
     * @param sRelativePathToAnimation Path to the .ozz animation file relative to the `res` directory.
     */
    void addPathToAnimationToPreload(std::string& sRelativePathToAnimation);

    /** Stops currently playing animation (if an animation was playing). */
    void stopAnimation();

    /**
     * Loads the specified animation (if it was not preloaded, see @ref addPathToAnimationToPreload)
     * and plays it.
     *
     * @warning If not spawned shows an error.
     *
     * @param sRelativePathToAnimation Path to .ozz animation file relative to the `res` directory.
     * @param bLoop                    Specify `true` to automatically play the animation again after it
     * finished, `false` otherwise.
     */
    void playAnimation(const std::string& sRelativePathToAnimation, bool bLoop);

    /**
     * Loads animations (if it was not preloaded, see @ref addPathToAnimationToPreload) and plays them
     * while using the blend factor to blend between animations (also see @ref setBlendFactor).
     *
     * @param vRelativePathsToAnimations Path to .ozz animation file relative to the `res` directory.
     * @param blendFactor                Initial blend factor in range [0.0; 1.0] where 0.0 means play
     * animation 0 (from the specified array of animations) and value 1.0 means play the last animation.
     */
    void playBlendedAnimations(const std::vector<std::string>& vRelativePathsToAnimations, float blendFactor);

    /**
     * Returns matrices that convert skeleton bones from local space to model space.
     * Returned matrices are updated every frame if an animation is being played.
     *
     * @return Matrices.
     */
    const std::vector<glm::mat4x4>& getSkeletonBoneMatrices() const { return vSkinningMatrices; }

    /**
     * Returns path (if was set) to the `skeleton.ozz` file relative to the `res` directory.
     *
     * @return Empty string if the path is not set.
     */
    std::string getPathToSkeletonRelativeRes() const { return sPathToSkeletonRelativeRes; }

    /**
     * Sets blend factor in range [0.0; 1.0] for animations that were previously set in @ref
     * playBlendedAnimations.
     *
     * @param blendFactor Blend ratio in [0.0; 1.0].
     */
    void setBlendFactor(float blendFactor);

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() override;

    /**
     * Called before a new frame is rendered.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic (if there is any).
     *
     * @remark This function is disabled by default, use @ref setIsCalledEveryFrame to enable it.
     * @remark This function will only be called while this node is spawned.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) override;

private:
    /** State of the node. */
    struct AnimationState {
        /** Currently playing animations. */
        std::vector<AnimationSampler*> vPlayingAnimations;

        /** Value in [0.0; 1.0] that determines which animation from @ref vPlayingAnimations to play. */
        float blendFactor = 0.0F;
    };

    /**
     * Loads skeleton from file.
     *
     * @param pathToSkeleton Path to skeleton .ozz file.
     * @param vInverseBindPoseMatrices Inverse bind pose matrices.
     *
     * @return Loaded skeleton.
     */
    static std::unique_ptr<ozz::animation::Skeleton> loadSkeleton(
        const std::filesystem::path& pathToSkeleton, std::vector<glm::mat4x4>& vInverseBindPoseMatrices);

    /**
     * Loads animation and adds it to @ref loadedAnimations.
     *
     * @param sRelativePathToAnimation Path to .ozz animation file relative to the `res` directory.
     * @param pSkeleton                Skeleton that will be used with the animation.
     */
    void loadAnimation(const std::string& sRelativePathToAnimation, ozz::animation::Skeleton* pSkeleton);

    /**
     * Looks if the animation is loaded in @ref loadedAnimations and returns it, otherwise
     * loads it.
     *
     * @param sRelativePathToAnimation Path to .ozz animation file relative to the `res` directory.
     *
     * @return Pointer to animation from @ref loadedAnimations.
     */
    AnimationSampler* findOrLoadAnimation(const std::string& sRelativePathToAnimation);

    /**
     * Loads all data needed to play animations: skeleton, preloaded animations, runtime buffers and etc.
     * Expects that @ref sPathToSkeletonRelativeRes is valid.
     */
    void loadAnimationContextData();

    /** Unloads everything that was loaded in @ref loadAnimationContextData (if it was loaded). */
    void unloadAnimationContextData();

    /** Converts @ref vResultingLocalTransforms to @ref vSkinningMatrices. */
    void convertResultingLocalTransformsToSkinning();

    /** State of the node. */
    AnimationState animState;

    /** `nullptr` if the skeleton is not loaded. */
    std::unique_ptr<ozz::animation::Skeleton> pSkeleton;

    /**
     * Pairs of
     * - key: path to .ozz anim file relative to `res` directory
     * - value: deserialized and loaded animations ready to be played.
     */
    std::unordered_map<std::string, std::unique_ptr<AnimationSampler>> loadedAnimations;

    /**
     * Paths to .ozz animation files relative to `res` directory
     * to add to @ref loadedAnimations on spawn.
     */
    std::unordered_set<std::string> pathsToAnimationsToPreload;

    /** Result of sampling/blending playing animations. 1 soa transform can store multiple (4) bones. */
    ozz::vector<ozz::math::SoaTransform> vResultingLocalTransforms;

    /** Matrices that transform bone from local space to model space. In ozz-animation system. */
    ozz::vector<ozz::math::Float4x4> vBoneMatrices;

    /** Matrix per bone in @ref vBoneMatrices to create @ref vSkinningMatrices. */
    std::vector<glm::mat4x4> vInverseBindPoseMatrices;

    /** Matrices used for skinning. */
    std::vector<glm::mat4x4> vSkinningMatrices;

    /** Path to the `skeleton.ozz` file relative to the `res` directory. */
    std::string sPathToSkeletonRelativeRes;

    /** The maximum number of bones allowed (per skeleton). */
    static constexpr unsigned int iMaxBoneCountAllowed = 64; // same as in shaders
};
