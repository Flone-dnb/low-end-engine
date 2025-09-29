#pragma once

// Custom.
#include "game/node/physics/CharacterBodyNode.h"

/**
 * Implementation of CharacterBodyNode which provides out of the box functionality for character movement,
 * jumping and crounching, it's more limited than CharacterBodyNode but simpler to use. You can derive your
 * character node from this type and then if you would need more features change the base class to
 * CharacterBodyNode.
 */
class SimpleCharacterBodyNode : public CharacterBodyNode {
public:
    SimpleCharacterBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SimpleCharacterBodyNode(const std::string& sNodeName);

    virtual ~SimpleCharacterBodyNode() override;

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
     * Sets movement speed.
     *
     * @param newSpeed New speed.
     */
    void setMovementSpeed(float newSpeed);

    /**
     * The bigger this value to higher the jump.
     *
     * @param newJumpPower New jump power.
     */
    void setJumpPower(float newJumpPower);

    /**
     * Sets multiplier for gravity in range [0.0F; +inf].
     *
     * @param newMultiplier New multiplier.
     */
    void setGravityMultiplier(float newMultiplier);

    /**
     * Defines if movement (movement control using input) in the air is possible or not and how much.
     * Value in range [0.0F; 1.0F] where 0.0 means input is ignored while in the air.
     *
     * @param factor Value in range [0.0F; 1.0F].
     */
    void setAirMovementControlFactor(float factor);

    /**
     * Sets a value in range [0.0F; 1.0F] that defines how to change character height when crouching.
     *
     * @param factor Value in range [0.0F; 1.0F].
     */
    void setCrouchingHeightFactor(float factor);

    /**
     * Changes the character's state standing/crouching.
     * Note that the change from crouching to standing might be ignored if there
     * is no room to stand, use return value or @ref getIsCrouching to check the result.
     *
     * @param bIsCrouching New state.
     * 
     * @return `true` if successfully changed the state.
     */
    bool trySetIsCrouching(bool bIsCrouching);

    /**
     * Returns movement speed.
     *
     * @return Speed.
     */
    float getMovementSpeed() const { return movementSpeed; }

    /**
     * The bigger this value to higher the jump.
     *
     * @return Jump power.
     */
    float getJumpPower() const { return jumpPower; }

    /**
     * Returns multiplier for gravity in range [0.0F; +inf].
     *
     * @return Multiplier.
     */
    float getGravityMultiplier() const { return gravityMultiplier; }

    /**
     * Defines if movement (movement control using input) in the air is possible or not and how much.
     * Value in range [0.0F; 1.0F] where 0.0 means input is ignored while in the air.
     *
     * @return Value in range [0.0F; 1.0F].
     */
    float getAirMovementControlFactor() const { return airMovementControlFactor; }

    /**
     * Return value in range [0.0F; 1.0F] that defines how to change character height when crouching.
     *
     * @param Crouching height factor.
     */
    float getCrouchingHeightFactor() const { return crouchingHeightFactor; }

    /**
     * Returns `true` if the character is crouching.
     *
     * @return `true` if crouching.
     */
    bool getIsCrouching() const { return bIsCrouching; }

protected:
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
    virtual void onBeforePhysicsUpdate(float deltaTime) override;

    /**
     * Sets the current input for moving forward/back in range [1.0F; -1.0F].
     *
     * @param input Current input.
     */
    void setForwardMovementInput(float input);

    /**
     * Sets the current input for moving right/left in range [1.0F; -1.0F].
     *
     * @param input Current input.
     */
    void setRightMovementInput(float input);

    /**
     * Makes the character jump on the next physics update if the character was on the ground.
     *
     * @param bEvenIfInAir Jump even if the character is in the air.
     */
    void jump(bool bEvenIfInAir);

private:
    /**
     * Stores forward movement in the X component in range [1.0F; -1.0F] and right movement in the Y
     * component. Normalized vector.
     */
    glm::vec2 movementInput = glm::vec2(0.0F, 0.0F);

    /** Speed of movement. */
    float movementSpeed = 5.0F;

    /** The bigger this value to higher the jump. */
    float jumpPower = 8.0F;

    /**
     * Defines if movement (movement control using input) in the air is possible or not and how much.
     * Value in range [0.0F; 1.0F] where 0.0 means input is ignored while in the air.
     */
    float airMovementControlFactor = 1.0F;

    /** Multiplier for gravity. */
    float gravityMultiplier = 2.0F;

    /** Value in range [0.0F; 1.0F] that defines how to change character height when crouching. */
    float crouchingHeightFactor = 0.45F;

    /** Initial half height of the character before crouching. */
    float charHalfHeightBeforeCrouching = 0.0F;

    /** `true` if @ref jump was called. */
    bool bWantsToJump = false;

    /** `true` if crounching. */
    bool bIsCrouching = false;

    /** `true` if @ref jump was called. */
    bool bWantsToJumpEvenIfInAir = false;
};