#include "node/EditorCameraNode.h"

// Standard.
#include <format>

// Custom.
#include "misc/Error.h"
#include "input/EditorInputEventIds.hpp"

EditorCameraNode::EditorCameraNode() : EditorCameraNode("Editor Camera Node") {}

EditorCameraNode::EditorCameraNode(const std::string& sNodeName) : CameraNode(sNodeName) {
    // Enable tick and input.
    setIsCalledEveryFrame(true);
    setIsReceivingInput(false); // will be enabled later

    // Initialize current speed.
    currentMovementSpeed = movementSpeed;

    // Bind axis events.
    {
        // Get axis events.
        auto& mtxAxisEvents = getAxisEventBindings();
        std::scoped_lock guard(mtxAxisEvents.first);

        // Move right.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD)] =
            [this](KeyboardModifiers modifiers, float input) { lastKeyboardInputDirection.x = input; };

        // Move forward.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT)] =
            [this](KeyboardModifiers modifiers, float input) { lastKeyboardInputDirection.y = input; };

        // Gamepad move right.
        mtxAxisEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_FORWARD)] =
            [this](KeyboardModifiers modifiers, float input) { lastGamepadInputDirection.x = -input; };

        // Gamepad move forward.
        mtxAxisEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_RIGHT)] =
            [this](KeyboardModifiers modifiers, float input) { lastGamepadInputDirection.y = input; };

        // Move up.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP)] =
            [this](KeyboardModifiers modifiers, float input) { lastKeyboardInputDirection.z = input; };

        // Gamepad look right.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_RIGHT)] =
            [this](KeyboardModifiers modifiers, float input) {
                lastGamepadLookInput.x = input * gamepadLookInputMult;
            };

        // Gamepad look up.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_UP)] =
            [this](KeyboardModifiers modifiers, float input) {
                lastGamepadLookInput.y = input * gamepadLookInputMult;
            };
    }

    // Bind action events.
    {
        // Get action events.
        auto& mtxActionEvents = getActionEventBindings();
        std::scoped_lock guard(mtxActionEvents.first);

        // Bind increase movement speed.
        mtxActionEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_MOVEMENT_SPEED)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                if (bIsPressed) {
                    currentMovementSpeedMultiplier = speedIncreaseMultiplier;
                } else {
                    currentMovementSpeedMultiplier = 1.0F;
                }
            };

        // Bind decrease movement speed.
        mtxActionEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_MOVEMENT_SPEED)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                if (bIsPressed) {
                    currentMovementSpeedMultiplier = speedDecreaseMultiplier;
                } else {
                    currentMovementSpeedMultiplier = 1.0F;
                }
            };
    }
}

void EditorCameraNode::setIsMouseCaptured(bool bCaptured) {
    bIsMouseCaptured = bCaptured;

    if (!bCaptured && !bIsGamepadConnected) {
        setIsReceivingInput(false);
    }

    lastKeyboardInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);
    currentMovementSpeedMultiplier = 1.0F;

    if (!bCaptured) {
        return;
    }

    setIsReceivingInput(true);
}

void EditorCameraNode::onGamepadConnected() {
    setIsReceivingInput(true);
    bIsGamepadConnected = true;

    lastGamepadInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);
    lastGamepadLookInput = glm::vec2(0.0F, 0.0F);
}

void EditorCameraNode::onGamepadDisconnected() {
    lastGamepadInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);
    lastGamepadLookInput = glm::vec2(0.0F, 0.0F);

    if (!bIsMouseCaptured) {
        setIsReceivingInput(false);
    }

    bIsGamepadConnected = false;
}

void EditorCameraNode::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    CameraNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    if (!isReceivingInput()) {
        return;
    }

    if (!glm::all(glm::epsilonEqual(lastGamepadLookInput, glm::vec2(0.0F, 0.0F), inputEpsilon))) {
        applyLookInput(lastGamepadLookInput.x, lastGamepadLookInput.y);
    }

    // Check for early exit and make sure input direction is not zero to avoid NaNs during `normalize` below.
    glm::vec3 movementDirection = glm::vec3(0.0F, 0.0F, 0.0F);
    if (bIsMouseCaptured &&
        !glm::all(glm::epsilonEqual(lastKeyboardInputDirection, glm::vec3(0.0F, 0.0F, 0.0F), inputEpsilon))) {
        // Normalize direction to avoid speed up on diagonal movement and apply speed.
        movementDirection = glm::normalize(lastKeyboardInputDirection) * timeSincePrevFrameInSec *
                            movementSpeed * currentMovementSpeedMultiplier;
    } else if (
        bIsGamepadConnected &&
        !glm::all(glm::epsilonEqual(lastGamepadInputDirection, glm::vec3(0.0F, 0.0F, 0.0F), inputEpsilon))) {
        movementDirection = lastGamepadInputDirection * timeSincePrevFrameInSec * movementSpeed *
                            currentMovementSpeedMultiplier;
    } else {
        return;
    }

    // Get node's world location.
    auto newWorldLocation = getWorldLocation();

    // Calculate new world location.
    newWorldLocation += getWorldForwardDirection() * movementDirection.x;
    newWorldLocation += getWorldRightDirection() * movementDirection.y;
    newWorldLocation += Globals::WorldDirection::up * movementDirection.z;

    // Apply movement.
    setWorldLocation(newWorldLocation);
}

void EditorCameraNode::onMouseMove(double xOffset, double yOffset) {
    CameraNode::onMouseMove(xOffset, yOffset);

    if (!isReceivingInput() || !bIsMouseCaptured) {
        return;
    }

    applyLookInput(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void EditorCameraNode::applyLookInput(float xDelta, float yDelta) {
    // Modify rotation.
    auto currentRotation = getRelativeRotation();
    currentRotation.z += xDelta * rotationSensitivity;
    currentRotation.y += yDelta * rotationSensitivity;

    // Apply rotation.
    setRelativeRotation(currentRotation);
}
