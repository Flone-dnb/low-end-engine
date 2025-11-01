#pragma once

// Standard.
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <optional>
#include <unordered_map>
#include <queue>

// Custom.
#include "math/GLMath.hpp"

// External.
#include "Jolt/Jolt.h" // Always include Jolt.h before including any other Jolt header.
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/SubShapeIDPair.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"

namespace JPH {
    class PhysicsSystem;
    class JobSystemThreadPool;
    class TempAllocatorImpl;
    class TempAllocator;
    class Body;
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
class TriggerVolumeNode;
class ContactListener;
class GameManager;
class TriggerVolumeNode;
class Node;

#if defined(ENGINE_DEBUG_TOOLS)
class PhysicsDebugDrawer;
#endif

/** Handles game physics. */
class PhysicsManager {
    // Only game manager is expected to create physics manager.
    friend class GameManager;

    // Adds contacts events to the queue.
    friend class ContactListener;

public:
    /** Hit result of a ray cast. */
    struct RayCastHit {
        /** ID of the body hit. */
        JPH::BodyID bodyId;

        /** Position of the hit. */
        glm::vec3 hitPosition;

        /** Normal of the hit. */
        glm::vec3 hitNormal;
    };

    ~PhysicsManager();

    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager) = delete;

#if defined(ENGINE_DEBUG_TOOLS)
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
    void createBodyForNode(TriggerVolumeNode* pNode);

    /**
     * Destroys previously created body (see @ref createBodyForNode).
     *
     * @param pNode Node to destroy collision.
     */
    void destroyBodyForNode(TriggerVolumeNode* pNode);

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
     * Casts a ray until something is hit.
     *
     * @param rayStartPosition Position of the ray's origin.
     * @param rayEndPosition   Position of the end of the ray.
     * @param vIgnoredBodies   Optional array of bodies to ignore.
     *
     * @return Empty if nothing was hit.
     */
    std::optional<RayCastHit> castRayUntilHit(
        const glm::vec3& rayStartPosition,
        const glm::vec3& rayEndPosition,
        const std::vector<JPH::BodyID>& vIgnoredBodies = {});

    /**
     * Casts a ray and collect all hits (don't stop after the first hit).
     *
     * @param rayStartPosition Position of the ray's origin.
     * @param rayEndPosition   Position of the end of the ray.
     * @param vIgnoredBodies   Optional array of bodies to ignore.
     *
     * @return Empty if nothing was hit, otherwise array of hits sorted by distance (starting with closest
     * first).
     */
    std::vector<RayCastHit> castRayHitMultipleSort(
        const glm::vec3& rayStartPosition,
        const glm::vec3& rayEndPosition,
        const std::vector<JPH::BodyID>& vIgnoredBodies = {});

    /**
     * Adds or removes the body from the physics world (does not destroys the body).
     *
     * @param pBody     Body to modify.
     * @param bAdd      `true` to add to the physics world, `false` to remove.
     * @param bActivate If adding to the physics world, specify `true` to also activate the body.
     */
    void addRemoveBody(JPH::Body* pBody, bool bAdd, bool bActivate);

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
     * Tells if a body with the specified ID is a sensor.
     *
     * @param bodyId ID of the body.
     *
     * @return `true` if sensor.
     */
    bool isBodySensor(JPH::BodyID bodyId);

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
     * @param bodyId Body ID.
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
    /** Groups information about a collision contact. */
    struct ContactInfo {
        /** `true` if contact added, `false` if contact lost. */
        bool bIsAdded = false;

        /** Sensor node id. */
        size_t iSensorNodeId = 0;

        /** Other node id. */
        size_t iOtherNodeId = 0;

        /** World space normal of the contact. */
        glm::vec3 worldNormal = glm::vec3(0.0F);

        /** World space location of the contact point. */
        glm::vec3 contactPointLocation = glm::vec3(0.0F);
    };

    /** Groups info about an active contact with sensor. */
    struct SensorContactInfo {
        /** Node ID of the sensor body. */
        size_t iSensorNodeId = 0;

        /** Node ID of the other body. */
        size_t iOtherNodeId = 0;
    };

    /** Groups data related to contacts. */
    struct ContactData {
        /** Contact add events to process. */
        std::queue<ContactInfo> newContactsAdded;

        /** Contact remove events to process. */
        std::queue<ContactInfo> newContactsRemoved;

        /** Added contacts that were not removed yet. */
        std::unordered_map<JPH::SubShapeIDPair, SensorContactInfo> activeSensorContacts;
    };

    /**
     * Constructor.
     *
     * @param pGameManager Game manager.
     */
    PhysicsManager(GameManager* pGameManager);

    /**
     * Checks if needs to run a physics tick and runs if needed.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    void onBeforeNewFrame(float timeSincePrevFrameInSec);

    /**
     * Creates a new body and adds to @ref bodyIdToPtr if successful.
     *
     * @param settings Body creation settings.
     *
     * @return `nullptr` if failed to create body.
     */
    JPH::Body* createBody(const JPH::BodyCreationSettings& settings);

    /**
     * Destroys a body and removes it from @ref bodyIdToPtr.
     *
     * @param bodyId ID of the body to destroy.
     */
    void destroyBody(const JPH::BodyID& bodyId);

    /** Data related to contacts. */
    std::pair<std::mutex, ContactData> mtxContactData;

    /** Used to update node position/rotation according to the simulated Jolt body. */
    std::unordered_set<SimulatedBodyNode*> simulatedBodies;

    /** Used to update node position/rotation according to the Jolt body. */
    std::unordered_set<MovingBodyNode*> movingBodies;

    /** Active character bodies. */
    std::unordered_set<CharacterBodyNode*> characterBodies;

    /** Mapping from body ID to body pointer for non destroyed bodies. */
    std::unordered_map<JPH::BodyID, JPH::Body*> bodyIdToPtr;

    /** Broad phase layers. */
    std::unique_ptr<BroadPhaseLayerInterfaceImpl> pBroadPhaseLayerInterfaceImpl;

    /** Object layers. */
    std::unique_ptr<ObjectLayerPairFilterImpl> pObjectLayerPairFilterImpl;

    /** Mapping between broad phase layers and object layers. */
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> pObjectVsBroadPhaseLayerFilterImpl;

    /** List of active characters so they can collide. */
    std::unique_ptr<JPH::CharacterVsCharacterCollisionSimple> pCharVsCharCollision;

    /** A listener class that receives collision contact events. */
    std::unique_ptr<ContactListener> pContactListener;

    /** Jolt physics system. */
    std::unique_ptr<JPH::PhysicsSystem> pPhysicsSystem;

    /** Thread pool. */
    std::unique_ptr<JPH::JobSystemThreadPool> pJobSystem;

    /** Temp allocator. */
    std::unique_ptr<JPH::TempAllocatorImpl> pTempAllocator;

    /** Game manager. */
    GameManager* const pGameManager = nullptr;

#if defined(ENGINE_DEBUG_TOOLS)
    /** Debug rendering of the physics. */
    std::unique_ptr<PhysicsDebugDrawer> pPhysicsDebugDrawer;

    /** Enables/disabled rendering of the physics bodies. */
    bool bEnableDebugRendering = false;
#endif
};
