#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

// External.
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

class CapsuleCollisionShape;

namespace JPH {
    class PhysicsSystem;
    class TempAllocator;
}

/**
 * Used to represent the physical body of a NPC or a player character.
 * It's expected that you create a new node that derives from this class
 * and implement custom movement logic in the onBeforePhysicsUpdate function.
 */
class CharacterBodyNode : public SpatialNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    CharacterBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    CharacterBodyNode(const std::string& sNodeName);

    virtual ~CharacterBodyNode() override;

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
     * Sets the maximum angle of slope that character can still walk on (in degrees).
     *
     * @param degrees Angle in degrees.
     */
    void setMaxWalkSlopeAngle(float degrees);

    /**
     * Sets the maximum height of the stairs to automatically step up on.
     *
     * @param newMaxStepHeight New max step height.
     */
    void setMaxStepHeight(float newMaxStepHeight);

    /**
     * Returns the maximum angle of slope that character can still walk on (in degrees).
     *
     * @return Angle.
     */
    float getMaxWalkSlopeAngle() const { return maxWalkSlopeAngleDeg; }

    /**
     * Maximum height of the stairs to automatically step up on.
     *
     * @return Step height.
     */
    float getMaxStepHeight() const { return maxStepHeight; }

protected:
    /** State of the floor under the character. */
    enum class GroundState : unsigned char {
        OnGround,      ///< Character is on the ground and can move freely.
        OnSteepGround, ///< Character is on a slope that is too steep and can't climb up any further.
        NotSupported,  ///< Character is touching an object, but is not supported by it and should fall. The
                       ///< GetGroundXXX functions will return information about the touched object.
        InAir,         ///< Character is in the air and is not touching anything.
    };

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
     * @warning If overriding you must call parent version before doing your logic.
     *
     * @remark Called only while spawned and have a physical body.
     *
     * @param deltaTime Time (in seconds) that has passed since the last physics update.
     */
    virtual void onBeforePhysicsUpdate(float deltaTime);

    /**
     * Tries changing the shape.
     * 
     * @param newShape Shape to take.
     *
     * @return `true` if the shape was changed, `false` if something will collide with the shape.
     */
    bool trySetNewShape(CapsuleCollisionShape& newShape);

    /**
     * Returns collision shape.
     *
     * @return Shape.
     */
    const CapsuleCollisionShape& getBodyShape() { return *pCollisionShape; }

    /**
     * Sets linear velocity of the body.
     *
     * @param velocity Velocity.
     */
    void setLinearVelocity(const glm::vec3& velocity);

    /**
     * Returns linear velocity of the body.
     *
     * @return Velocity.
     */
    glm::vec3 getLinearVelocity();

    /**
     * Returns the current state of the "floor" under the character.
     *
     * @return State.
     */
    GroundState getGroundState();

    /**
     * Checks if the normal of the ground surface is too steep to walk on.
     *
     * @param normal Normal of the ground.
     */
    bool isSlopeTooSteep(const glm::vec3& normal);

    /**
     * Returns normal of the ground (if there is a ground below the character).
     *
     * @return Normal.
     */
    glm::vec3 getGroundNormal();

    /**
     * Returns velocity of the ground (if there is a ground below the character).
     * For example if staying on a moving platform this returns velocity of the platform.
     *
     * @return Ground velocity.
     */
    glm::vec3 getGroundVelocity();

    /**
     * Returns gravity.
     *
     * @return Gravity.
     */
    glm::vec3 getGravity();

private:
    /**
     * Adjusts @ref pCollisionShape so that its bottom is at (0, 0, 0)
     * and returns the created body.
     *
     * @param shape Shape to use.
     *
     * @return Created shape.
     */
    static JPH::Ref<JPH::Shape> createAdjustedJoltShapeForCharacter(const CapsuleCollisionShape& shape);

    /** Generally called after some property is changed to recreate the body. */
    void recreateBodyIfSpawned();

    /** Creates @ref pCharacterBody. */
    void createCharacterBody();

    /** Destroys @ref pCharacterBody. */
    void destroyCharacterBody();

    /**
     * Called after @ref onBeforePhysicsUpdate (after user logic) to calculate updated body position.
     *
     * @param physicsSystem Physics system.
     * @param tempAllocator Temp allocator.
     * @param deltaTime Time (in seconds) that has passed since the last physics update.
     */
    void updateCharacterPosition(
        JPH::PhysicsSystem& physicsSystem, JPH::TempAllocator& tempAllocator, float deltaTime);

    /** Collision shape of the character. */
    std::unique_ptr<CapsuleCollisionShape> pCollisionShape;

    /** Not `nullptr` if spawned. */
    JPH::Ref<JPH::CharacterVirtual> pCharacterBody;

    /** Maximum angle of slope that character can still walk on (in degrees). */
    float maxWalkSlopeAngleDeg = 45.0f;

    /** Maximum height of the stairs to automatically step up on. */
    float maxStepHeight = 0.4F;

    /** `true` if inside of @ref updateCharacterPosition. */
    bool bIsApplyingUpdateResults = false;

#if defined(DEBUG)
    /** `true` if we produced a warning in case the body fell out of the world. */
    bool bWarnedAboutFallingOutOfWorld = false;

    /** `true` if we are running before physics tick functionality. */
    bool bIsInPhysicsTick = false;
#endif
};