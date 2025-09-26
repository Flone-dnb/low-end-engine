#pragma once

// Custom.
#include "game/node/physics/CharacterBodyNode.h"

/**
 * Implementation of CharacterBodyNode which provides out of the box functionality for character movement,
 * jumping and crounching, it's more limited than CharacterBodyNode but simpler to use. You can derive your
 * character node from this type and then if you would need more features change the base class to CharacterBodyNode.
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
     * Sets the current input for movement.
     * Forward movement in the X component in range [1.0F; -1.0F] and right movement in the Y component.
     * 
     * @param input Current input.
     */
    void setMovementInput(const glm::vec2& input);

    /** Makes the character jump on the next physics update. */
    void jump();

private:
    /**
     * Stores forward movement in the X component in range [1.0F; -1.0F] and right movement in the Y component.
     * Normalized vector.
     */
    glm::vec2 movementInput = glm::vec2(0.0F, 0.0F);

    /** Speed of movement. */
    float movementSpeed = 5.0F;

    /** The bigger this value to higher the jump. */
    float jumpPower = 3.0F;

    /** `true` if @ref jump was called. */
    bool bWantsToJump = false;
};