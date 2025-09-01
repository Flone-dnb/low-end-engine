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

    /**
     * Sets animation playback speed where 1 means no modification to the speed of the animation.
     *
     * @param speed New speed.
     *
     */
    void setAnimationPlaybackSpeed(float speed);

    /**
     * Loads the specified animation (if it was not preloaded, see @ref addPathToAnimationToPreload)
     * and plays it.
     *
     * @warning If not spawned shows an error.
     *
     * @param sRelativePathToAnimation Path to .ozz animation file relative to the `res` directory.
     * @param bRestart                 If the specified animation is already playing specify `true` to restart
     * the animation to play it from the beginning or `false` to just continue playing it without restarting.
     * @param bLoop                    Specify `true` to automatically play the animation again after it
     * finished, `false` otherwise.
     */
    void playAnimation(const std::string& sRelativePathToAnimation, bool bRestart, bool bLoop);

    /**
     * Returns matrices that convert skeleton bones from local space to model space.
     * Returned matrices are updated every frame if an animation is being played.
     *
     * @return Matrices.
     */
    const ozz::vector<ozz::math::Float4x4>& getSkeletonBoneMatrices() const { return vBoneMatrices; }

    /**
     * Returns path (if was set) to the `skeleton.ozz` file relative to the `res` directory.
     *
     * @return Empty string if the path is not set.
     */
    std::string getPathToSkeletonRelativeRes() const { return sPathToSkeletonRelativeRes; }

    /**
     * Returns animation playback speed where 1 means no modification to the speed of the animation.
     *
     * @return Speed.
     */
    float getAnimationPlaybackSpeed() const { return playbackSpeed; }

    /**
     * Returns current position of the playing animation in range [0; 1] where 0 means animation start and 1
     * animation end.
     *
     * @return Animation position.
     */
    float getCurrentAnimationTime() const { return animationRatio; }

    /**
     * Returns duration of the currently playing animation (or 0 if not playing an animation).
     *
     * @return Duration.
     */
    float getCurrentAnimationDurationSec() const;

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
    /**
     * Loads skeleton from file.
     *
     * @param pathToSkeleton Path to skeleton .ozz file.
     *
     * @return Loaded skeleton.
     */
    static std::unique_ptr<ozz::animation::Skeleton>
    loadSkeleton(const std::filesystem::path& pathToSkeleton);

    /**
     * Loads animation from file.
     *
     * @param pathToAnimation     Path to animation .ozz file.
     * @param iSkeletonBoneCount Checks that the animation has the same number of bones as the skeleton.
     *
     * @return Loaded animation.
     */
    static std::unique_ptr<ozz::animation::Animation>
    loadAnimation(const std::filesystem::path& pathToAnimation, unsigned int iSkeletonBoneCount);

    /**
     * Looks if the animation is loaded in @ref loadedAnimations and returns it, otherwise
     * loads it.
     *
     * @param sRelativePathToAnimation Path to .ozz animation file relative to the `res` directory.
     *
     * @return Pointer to animation from @ref loadedAnimations.
     */
    ozz::animation::Animation* findOrLoadAnimation(const std::string& sRelativePathToAnimation);

    /**
     * Loads all data needed to play animations: skeleton, preloaded animations, runtime buffers and etc.
     * Expects that @ref sPathToSkeletonRelativeRes is valid.
     */
    void loadAnimationContextData();

    /** Unloads everything that was loaded in @ref loadAnimationContextData (if it was loaded). */
    void unloadAnimationContextData();

    /** `nullptr` if the skeleton is not loaded. */
    std::unique_ptr<ozz::animation::Skeleton> pSkeleton;

    /** Pairs of
     * - key: path to .ozz anim file (without .ozz extension) relative to @ref sPathToSkeletonRelativeRes
     * - value: deserialized and loaded animations ready to be played.
     */
    std::unordered_map<std::string, std::unique_ptr<ozz::animation::Animation>> loadedAnimations;

    /**
     * Paths to .ozz animation files (without .ozz ext) relative to @ref sPathToSkeletonRelativeRes
     * to add to @ref loadedAnimations on spawn.
     */
    std::unordered_set<std::string> pathsToAnimationsToPreload;

    /** Pointer to item from @ref loadedAnimations that should be currently playing (or `nullptr`). */
    ozz::animation::Animation* pPlayingAnimation = nullptr;

    /** Context for sampling animation state. */
    ozz::animation::SamplingJob::Context samplingJobContext;

    /** Sampled local transforms, 1 soa transform can store multiple (4) bones. */
    ozz::vector<ozz::math::SoaTransform> vLocalTransforms;

    /** Matrices that transform bone from local space to model space. */
    ozz::vector<ozz::math::Float4x4> vBoneMatrices;

    /** Path to the `skeleton.ozz` file relative to the `res` directory. */
    std::string sPathToSkeletonRelativeRes;

    /** Current position of the animation in range [0; 1] where 0 means animation start and 1 means end. */
    float animationRatio = 0.0F;

    /** Playback speed for the animation. */
    float playbackSpeed = 1.0F;

    /** `true` to loop the currently playing animation. */
    bool bLoopAnimation = false;

    /** The maximum number of bones allowed (per skeleton). */
    static constexpr unsigned int iMaxBoneCountAllowed = 64; // same as in shaders
};
