#pragma once

// Standard.
#include <memory>
#include <atomic>
#include <unordered_set>

// Custom.
#include "math/GLMath.hpp"

namespace JPH {
    class PhysicsSystem;
    class JobSystemThreadPool;
    class TempAllocatorImpl;
    class TempAllocator;
    class Body;
    class BodyID;
    class CharacterVsCharacterCollisionSimple;
}

class BroadPhaseLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;
class CollisionNode;
class SimulatedBodyNode;
class MovingBodyNode;
class CompoundCollisionNode;
class CharacterBodyNode;
class CapsuleCollisionShape;

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
     * Optimizes broad phase if added a lot of bodies before a physics update.
     * Don't call this every frame.
     */
    void optimizeBroadPhase();

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
     * Creates a physics body for the specified node.
     *
     * @param pNode Node to create collision for.
     */
    void createBodyForNode(SimulatedBodyNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(SimulatedBodyNode* pNode);

    /**
     * Creates a physics body for the specified node.
     *
     * @param pNode Node to create collision for.
     */
    void createBodyForNode(MovingBodyNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(MovingBodyNode* pNode);

    /**
     * Creates a physics body for the specified node.
     *
     * @param pNode Node to create collision for.
     */
    void createBodyForNode(CompoundCollisionNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(CompoundCollisionNode* pNode);

    /**
     * Creates a physics body for the specified node.
     *
     * @param pNode Node to create collision for.
     */
    void createBodyForNode(CharacterBodyNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(CharacterBodyNode* pNode);

    /**
     * Sets new location and rotation to the specified physics body.
     *
     * @param pBody    Body to change.
     * @param location New location of the body.
     * @param rotation New rotation of the body.
     */
    void setBodyLocationRotation(JPH::Body* pBody, const glm::vec3& location, const glm::vec3& rotation);

    /**
     * Activates or deactivates a body.
     *
     * @param pBody Body to update.
     * @param bActivate New state.
     */
    void setBodyActiveState(JPH::Body* pBody, bool bActivate);

    /**
     * Adds impulse to the body.
     *
     * @param pBody  Body to update.
     * @param impulse Impulse.
     */
    void addImpulseToBody(JPH::Body* pBody, const glm::vec3& impulse);

    /**
     * Adds angular impulse to the body.
     *
     * @param pBody  Body to update.
     * @param impulse Impulse.
     */
    void addAngularImpulseToBody(JPH::Body* pBody, const glm::vec3& impulse);

    /**
     * Adds force to the body.
     *
     * @param pBody Body to update.
     * @param force Force.
     */
    void addForce(JPH::Body* pBody, const glm::vec3& force);

    /**
     * Sets velocity of body such that it will be positioned at the specified position/rotation in deltaTime
     * seconds.
     *
     * @param pBody         Body to modify.
     * @param worldLocation Location to be in after deltaTime seconds.
     * @param worldRotation Rotation to be in after deltaTime seconds.
     * @param deltaTime     Time in seconds.
     */
    void moveKinematic(
        JPH::Body* pBody, const glm::vec3& worldLocation, const glm::vec3& worldRotation, float deltaTime);

    /**
     * Sets linear velocity to a body.
     *
     * @param pBody    Body to update.
     * @param velocity Velocity.
     */
    void setLinearVelocity(JPH::Body* pBody, const glm::vec3& velocity);

    /**
     * Sets angular velocity to a body.
     *
     * @param pBody    Body to update.
     * @param velocity Velocity.
     */
    void setAngularVelocity(JPH::Body* pBody, const glm::vec3& velocity);

    /**
     * Returns linear velocity to a body.
     *
     * @param pBody Body to check.
     *
     * @return Velocity.
     */
    glm::vec3 getLinearVelocity(JPH::Body* pBody);

    /**
     * Returns angular velocity to a body.
     *
     * @param pBody Body to check.
     *
     * @return Velocity.
     */
    glm::vec3 getAngularVelocity(JPH::Body* pBody);

    /**
     * Returns user data from the body.
     *
     * @return User data.
     */
    uint64_t getUserDataFromBody(JPH::BodyID bodyId);

    /**
     * Returns gravity.
     *
     * @return Gravity.
     */
    glm::vec3 getGravity();

    /**
     * Returns internal physics system.
     *
     * @return Physics system.
     */
    JPH::PhysicsSystem& getPhysicsSystem();

    /**
     * Returns internal temp allocator.
     *
     * @return Temp allocator.
     */
    JPH::TempAllocator& getTempAllocator();

private:
    PhysicsManager();

    /**
     * Checks if needs to run a physics tick and runs if needed.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    void onBeforeNewFrame(float timeSincePrevFrameInSec);

    /** Used to update node position/rotation according to the simulated Jolt body. */
    std::unordered_set<SimulatedBodyNode*> simulatedBodies;

    /** Used to update node position/rotation according to the Jolt body. */
    std::unordered_set<MovingBodyNode*> movingBodies;

    /** Active character bodies. */
    std::unordered_set<CharacterBodyNode*> characterBodies;

    /** Broad phase layers. */
    std::unique_ptr<BroadPhaseLayerInterfaceImpl> pBroadPhaseLayerInterfaceImpl;

    /** Object layers. */
    std::unique_ptr<ObjectLayerPairFilterImpl> pObjectLayerPairFilterImpl;

    /** Mapping between broad phase layers and object layers. */
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> pObjectVsBroadPhaseLayerFilterImpl;

    /** List of active characters so they can collide. */
    std::unique_ptr<JPH::CharacterVsCharacterCollisionSimple> pCharVsCharCollision;

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

#if defined(DEBUG)
    /** Enables/disabled rendering of the physics bodies. */
    bool bEnableDebugRendering = false;
#endif
};
