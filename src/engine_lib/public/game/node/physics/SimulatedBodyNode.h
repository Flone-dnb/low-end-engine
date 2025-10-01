#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

namespace JPH {
    class Body;
}

class CollisionShape;

/**
 * Simple simulated body that is moved by forces and is affected by the gravity. For example it may be used
 * to simulate an object that the player throws with some initial impulse (such as a grenade).
 */
class SimulatedBodyNode : public SpatialNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    SimulatedBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SimulatedBodyNode(const std::string& sNodeName);

    virtual ~SimulatedBodyNode() override;

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
     * Sets a new shape.
     *
     * @param pNewShape New shape.
     */
    void setShape(std::unique_ptr<CollisionShape> pNewShape);

    /**
     * Sets uniform density of the interior of the object (kg / m^3).
     *
     * @warning Causes the physics body to be recreated.
     *
     * @param newDensity Density.
     */
    void setDensity(float newDensity);

    /**
     * Sets mass of the shape (kg). 0.0 (default) to automatically calculate based on the shape and its
     * density.
     *
     * @warning Causes the physics body to be recreated.
     *
     * @param newMassKg New mass.
     */
    void setMass(float newMassKg);

    /**
     * Sets dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force
     * that presses the two bodies together.
     *
     * @warning Causes the physics body to be recreated.
     *
     * @param newFriction Friction.
     */
    void setFriction(float newFriction);

    /**
     * Specify `true` if this body should be simulated, `false` if the simulation should be paused for this
     * body.
     *
     * @remark Note that the body will not be simulated while despawned.
     *
     * @param bActivate New state.
     */
    void setIsSimulated(bool bActivate);

    /**
     * Applies impulse to the body. Used for one-time pushes, if you need to constantly (every tick)
     * apply a specific force to the body use @ref setForceForNextTick.
     *
     * @remark Does nothing if the node is not spawned.
     *
     * @param impulse Impulse.
     */
    void applyOneTimeImpulse(const glm::vec3& impulse);

    /**
     * Adds angular impulse to the body.
     *
     * @remark Does nothing if the node is not spawned.
     *
     * @param impulse Impulse.
     */
    void applyOneTimeAngularImpulse(const glm::vec3& impulse);

    /**
     * Sets a force that will be applied during the next physics tick and will be reset after that.
     * Used to apply force that's acting over time (not a single one-time impulse).
     *
     * @param force Force.
     */
    void setForceForNextTick(const glm::vec3& force);

    /**
     * Returns used collision shape.
     *
     * @return Shape.
     */
    CollisionShape& getShape() const;

    /**
     * Returns uniform density of the interior of the object (kg / m^3).
     *
     * @return Density.
     */
    float getDensity() const { return density; }

    /**
     * Returns mass of the shape (kg). 0.0 to automatically calculate based on the shape and its density.
     *
     * @return Mass.
     */
    float getMass() const { return massKg; }

    /**
     * Returns dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force
     * that presses the two bodies together.
     *
     * @return Friction.
     */
    float getFriction() const { return friction; }

    /**
     * `true` if this body is simulated, `false` if the simulation is paused for this body.
     *
     * @return State.
     */
    bool isSimulated() const { return bIsSimulated; }

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
     * Can be used to update game specific physics parameters of the body (such as force for example).
     *
     * @remark Called only while spawned and have a physical body.
     *
     * @param deltaTime Time (in seconds) that has passed since the last physics update.
     */
    virtual void onBeforePhysicsUpdate(float deltaTime) {}

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

    /** Uniform density of the interior of the convex object (kg / m^3). */
    float density = 1000.0F;

    /** Mass of the shape (kg). 0.0 to automatically calculate based on the shape and its density. */
    float massKg = 0.0F;

    /**
     * Dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force that
     * presses the two bodies together.
     */
    float friction = 0.2F;

    /** `false` to pause simulation for this body. */
    bool bIsSimulated = true;

    /** `true` if we are in the @ref setPhysicsSimulationResults. */
    bool bIsApplyingSimulationResults = false;

#if defined(DEBUG)
    /** `true` if we produced a warning in case the body fell out of the world. */
    bool bWarnedAboutFallingOutOfWorld = false;

    /** The total number of times the body was re-created after spawning. */
    size_t iBodyRecreateCountAfterSpawn = 0;

    /** `true` if we produced a warning about a body being recreated often. */
    bool bWarnedAboutBodyRecreatingOften = false;
#endif
};
