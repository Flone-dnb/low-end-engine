#pragma once

// Custom.
#include "game/node/physics/SimulatedBodyNode.h"

/**
 * Kinematic body that is moved by velocities. For example this node can be used to create things like moving
 * platforms that the player's character can stand on. By default it's not affected by the gravity but derived
 * classes can implement this and other physics-related logic in onBeforePhysicsUpdate.
 */
class MovingBodyNode : public SpatialNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    MovingBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    MovingBodyNode(const std::string& sNodeName);

    virtual ~MovingBodyNode() override;

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
     * Returns linear velocity of the body.
     *
     * @return Velocity.
     */
    glm::vec3 getLinearVelocity();

    /**
     * Returns angular velocity of the body.
     *
     * @return Velocity.
     */
    glm::vec3 getAngularVelocity();

    /**
     * Returns used collision shape.
     *
     * @return Shape.
     */
    CollisionShape& getShape() const;

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     *
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
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
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark If you change location/rotation/scale inside of this function,
     * this function will not be called again (no recursion will occur).
     */
    virtual void onWorldLocationRotationScaleChanged() override;

    /**
     * Called before a physics update is executed.
     * Can be used to update game specific physics parameters of the body (such as velocity for example).
     *
     * @remark Called only while spawned and have a physical body.
     *
     * @param deltaTime Time (in seconds) that has passed since the last physics update.
     */
    virtual void onBeforePhysicsUpdate(float deltaTime) {}

    /**
     * Sets velocity of the body such that it will be positioned at the specified position/rotation in deltaTime
     * seconds.
     *
     * You can easily create a smooth vertically moving (up and down) platform by using this function in @ref
     * onBeforePhysicsUpdate like so:
     * @code
     * void MyMovingBodyNode::onBeforePhysicsUpdate(float deltaTime) {
     *     totalTime += deltaTime;
     *
     *     constexpr float height = 3.0F;
     *     setVelocityToBeAt(
     *         getWorldLocation() + glm::vec3(0.0F, 0.0F, height * std::sin(totalTime)),
     *         getWorldRotation(),
     *         deltaTime);
     * }
     * @endcode
     *
     * @param worldLocation Location to be in after deltaTime seconds.
     * @param worldRotation Rotation to be in after deltaTime seconds.
     * @param deltaTime     Time in seconds.
     */
    void setVelocityToBeAt(const glm::vec3& worldLocation, const glm::vec3& worldRotation, float deltaTime);

    /**
     * Sets linear velocity of the body.
     *
     * @remark Does nothing if the node is not spawned.
     *
     * @param velocity Velocity.
     */
    void setLinearVelocity(const glm::vec3& velocity);

    /**
     * Sets angular velocity of the body.
     *
     * @remark Does nothing if the node is not spawned.
     *
     * @param velocity Velocity.
     */
    void setAngularVelocity(const glm::vec3& velocity);

    /**
     * Returns physics body.
     *
     * @return `nullptr` if not created yet.
     */
    JPH::Body* getBody() const { return pBody; }

    /**
     * Returns gravity.
     *
     * @return Gravity.
     */
    glm::vec3 getGravityWhileSpawned();

private:
    /**
     * Called by physics manager to apply simulation tick results.
     *
     * @param worldLocation World location of the body.
     * @param worldRotation World rotation of the body.
     */
    void setPhysicsSimulationResults(const glm::vec3& worldLocation, const glm::vec3& worldRotation);

    /** Sets `onChanged` callback to @ref pShape. */
    void setOnShapeChangedCallback();

    /** Generally called after some property is changed to recreate the body. */
    void recreateBodyIfSpawned();

    /** Collision shape. */
    std::unique_ptr<CollisionShape> pShape;

    /** Not `nullptr` when spawned. */
    JPH::Body* pBody = nullptr;

    /** `true` if we are in the @ref setPhysicsSimulationResults. */
    bool bIsApplyingSimulationResults = false;

#if defined(DEBUG)
    /** `true` if we produced a warning in case the body fell out of the world. */
    bool bWarnedAboutFallingOutOfWorld = false;

    /** The total number of times the body was re-created after spawning. */
    size_t iBodyRecreateCountAfterSpawn = 0;

    /** `true` if we produced a warning about a body being recreated often. */
    bool bWarnedAboutBodyRecreatingOften = false;

    /** `true` if we are running before physics tick functionality. */
    bool bIsInPhysicsTick = false;
#endif
};