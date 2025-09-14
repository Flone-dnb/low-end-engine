#pragma once

// Standard.
#include <memory>

// Custom.
#include "math/GLMath.hpp"

namespace JPH {
    class PhysicsSystem;
    class JobSystemThreadPool;
    class TempAllocatorImpl;
    class Body;
}

class BroadPhaseLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;
class CollisionNode;

#if defined(DEBUG)
class PhysicsDebugDrawer;
#endif

/** Handles game physics. */
class PhysicsManager {
    // Only game manager is expected to create physics manager.
    friend class GameManager;

public:
    ~PhysicsManager();

    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager) = delete;

#if defined(DEBUG)
    /**
     * Enables/disabled rendering of the physics bodies.
     *
     * @param bEnable New state.
     */
    void setEnableDebugRendering(bool bEnable);
#endif

    /**
     * Creates a physics body for the specified node.
     *
     * @param pNode Node to create collision for.
     */
    void createBodyForNode(CollisionNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(CollisionNode* pNode);

    /**
     * Sets new location and rotation to the specified physics body.
     *
     * @param pBody    Body to change.
     * @param location New location of the body.
     * @param rotation New rotation of the body.
     */
    void setBodyLocationRotation(JPH::Body* pBody, const glm::vec3& location, const glm::vec3& rotation);

private:
    PhysicsManager();

    /**
     * Checks if needs to run a physics tick and runs if needed.
     *
     * @parma timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    void onBeforeNewFrame(float timeSincePrevFrameInSec);

    /** Broad phase layers. */
    std::unique_ptr<BroadPhaseLayerInterfaceImpl> pBroadPhaseLayerInterfaceImpl;

    /** Object layers. */
    std::unique_ptr<ObjectLayerPairFilterImpl> pObjectLayerPairFilterImpl;

    /** Mapping between broad phase layers and object layers. */
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> pObjectVsBroadPhaseLayerFilterImpl;

    /** Jolt physics system. */
    std::unique_ptr<JPH::PhysicsSystem> pPhysicsSystem;

    /** Thread pool. */
    std::unique_ptr<JPH::JobSystemThreadPool> pJobSystem;

    /** Temp allocator. */
    std::unique_ptr<JPH::TempAllocatorImpl> pTempAllocator;

#if defined(DEBUG)
    /** Debug rendering of the physics. */
    std::unique_ptr<PhysicsDebugDrawer> pPhysicsDebugDrawer;
#endif

    /** Time in seconds that has passed since the last time we updated Jolt physics. */
    float timeSinceLastJoltUpdate = 0.0F;

#if defined(DEBUG)
    /** Enables/disabled rendering of the physics bodies. */
    bool bEnableDebugRendering = false;
#endif
};
