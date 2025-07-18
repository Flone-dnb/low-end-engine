#pragma once

// Custom.
#include "game/node/CameraNode.h"

/** Camera used in the editor. */
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
     * Tells if movement should be enabled (captured) or not.
     *
     * @param bCaptured `true` to enable input.
     */
    void setIsMouseCaptured(bool bCaptured);

    /** Called after a gamepad was connected. */
    void onGamepadConnected();

    /** Called after a gamepad was disconnected. */
    void onGamepadDisconnected();

protected:
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

private:
    /**
     * Applies rotation to the camera.
     *
     * @param xDelta Horizontal rotation.
     * @param yDelta Vertical rotation.
     */
    void applyLookInput(float xDelta, float yDelta);

    /** Last received user input direction for moving the camera. */
    glm::vec3 lastKeyboardInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);

    /** Same as @ref lastKeyboardInputDirection but for gamepad input. */
    glm::vec3 lastGamepadInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);

    /** Gamepad input for looking right (x) and up (y). */
    glm::vec2 lastGamepadLookInput = glm::vec2(0.0F, 0.0F);

    /** Editor camera's current movement speed. */
    float currentMovementSpeed = 0.0F;

    /**
     * Stores @ref speedIncreaseMultiplier or @ref speedDecreaseMultiplier when the user holds
     * a special button.
     */
    float currentMovementSpeedMultiplier = 1.0F;

    /** Rotation multiplier for editor's camera. */
    float rotationSensitivity = 0.1F;

    /** Tells if the movement is currently enabled (captured) or not. */
    bool bIsMouseCaptured = false;

    /** Tells if the movement is currently enabled or not. */
    bool bIsGamepadConnected = false;

    /** Constant multiplier for gamepad's rotation input. */
    static constexpr float gamepadLookInputMult = 10.0F;

    /** Speed of editor camera's movement. */
    static constexpr float movementSpeed = 5.0F;

    /** Camera speed multiplier when fast movement mode is enabled (for ex. Shift is pressed). */
    static constexpr float speedIncreaseMultiplier = 2.0F;

    /** Camera speed multiplier when slow movement mode is enabled (for ex. Ctrl is pressed). */
    static constexpr float speedDecreaseMultiplier = 0.5F;

    /** Used to compare input to zero. */
    static constexpr float inputEpsilon = 0.0001F;
};
