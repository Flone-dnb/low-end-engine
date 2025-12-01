#pragma once

// Custom.
#include "game/node/CameraNode.h"

/** Camera used in the editor's viewport. */
class EditorCameraNode : public CameraNode {
public:
    EditorCameraNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    EditorCameraNode(const std::string& sNodeName);

    virtual ~EditorCameraNode() override = default;

    /**
     * Tells if the camera's movement/rotation should be enabled (captured) or not.
     *
     * @param bCaptured `true` to enable input.
     */
    void setIsMouseCaptured(bool bCaptured);

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     */
    virtual void onSpawning() override;

    /**
     * Called before a new frame is rendered.
     *
     * @remark This function will only be called while this node is spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic (if there is any).
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) override;

    /**
     * Called when the window received mouse movement.
     *
     * @remark This function will only be called while this node is spawned.
     *
     * @param xOffset  Mouse X movement delta in pixels (plus if moved to the right,
     * minus if moved to the left).
     * @param yOffset  Mouse Y movement delta in pixels (plus if moved up,
     * minus if moved down).
     */
    virtual void onMouseMove(double xOffset, double yOffset) override;

    /**
     * Called after a gamepad controller was connected.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param sGamepadName Name of the connected gamepad.
     */
    virtual void onGamepadConnected(std::string_view sGamepadName) override;

    /**
     * Called after a gamepad controller was disconnected.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onGamepadDisconnected() override;

private:
    /**
     * Applies rotation to the camera.
     *
     * @param xDelta Horizontal rotation.
     * @param yDelta Vertical rotation.
     */
    void applyLookInput(float xDelta, float yDelta);

    /** Last received user input direction for moving the camera. */
    glm::vec3 lastKeyboardInputDirection = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Same as @ref lastKeyboardInputDirection but for gamepad input. */
    glm::vec3 lastGamepadInputDirection = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Gamepad input for looking right (x) and up (y). */
    glm::vec2 lastGamepadLookInput = glm::vec2(0.0f, 0.0f);

    /** Editor camera's current movement speed. */
    float currentMovementSpeed = 0.0f;

    /**
     * Stores @ref speedIncreaseMultiplier or @ref speedDecreaseMultiplier when the user holds
     * a special button.
     */
    float currentMovementSpeedMultiplier = 1.0f;

    /** Rotation multiplier for editor's camera. */
    float rotationSensitivity = 0.1f;

    /** Tells if the movement is currently enabled (captured) or not. */
    bool bIsMouseCaptured = false;

    /** Tells if the movement is currently enabled or not. */
    bool bIsGamepadConnected = false;

    /** Whether to apply or ignore input. */
    bool bIgnoreInput = true;

    /** Constant multiplier for gamepad's rotation input. */
    static constexpr float gamepadLookInputMult = 10.0f;

    /** Speed of editor camera's movement. */
    static constexpr float movementSpeed = 5.0f;

    /** Camera speed multiplier when fast movement mode is enabled (for ex. Shift is pressed). */
    static constexpr float speedIncreaseMultiplier = 2.0f;

    /** Camera speed multiplier when slow movement mode is enabled (for ex. Ctrl is pressed). */
    static constexpr float speedDecreaseMultiplier = 0.5f;

    /** Used to compare input to zero. */
    static constexpr float inputEpsilon = 0.0001f;
};
