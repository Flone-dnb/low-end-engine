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

        // Bind move right.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD)] =
            [this](KeyboardModifiers modifiers, float input) { lastInputDirection.x = input; };

        // Bind move forward.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT)] =
            [this](KeyboardModifiers modifiers, float input) { lastInputDirection.y = input; };

        // Bind move up.
        mtxAxisEvents.second[static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP)] =
            [this](KeyboardModifiers modifiers, float input) { lastInputDirection.z = input; };
    }

    // Bind action events.
    {
        // Get action events.
        auto& mtxActionEvents = getActionEventBindings();
        std::scoped_lock guard(mtxActionEvents.first);

        // Bind increase speed.
        mtxActionEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_SPEED)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                if (bIsPressed) {
                    currentMovementSpeedMultiplier = speedIncreaseMultiplier;
                } else {
                    currentMovementSpeedMultiplier = 1.0F;
                }
            };

        // Bind decrease speed.
        mtxActionEvents
            .second[static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_SPEED)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                if (bIsPressed) {
                    currentMovementSpeedMultiplier = speedDecreaseMultiplier;
                } else {
                    currentMovementSpeedMultiplier = 1.0F;
                }
            };
    }
}

void EditorCameraNode::setIgnoreInput(bool bIgnore) {
    setIsReceivingInput(!bIgnore);

    if (bIgnore) {
        // Reset any previous input (for ex. if the user was holding some button).
        lastInputDirection = glm::vec3(0.0F, 0.0F, 0.0F);
        currentMovementSpeedMultiplier = 1.0F;
    }
}

void EditorCameraNode::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    CameraNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    // Check for early exit.
    // Also make sure input direction is not zero to avoid NaNs during `normalize` (see below).
    if (!isReceivingInput() ||
        glm::all(glm::epsilonEqual(lastInputDirection, glm::vec3(0.0F, 0.0F, 0.0F), 0.0001F))) { // NOLINT
        return;
    }

    // Normalize direction to avoid speed up on diagonal movement and apply speed.
    const auto movementDirection = glm::normalize(lastInputDirection) * timeSincePrevFrameInSec *
                                   movementSpeed * currentMovementSpeedMultiplier;

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

    if (!isReceivingInput()) {
        return;
    }

    // Modify rotation.
    auto currentRotation = getRelativeRotation();
    currentRotation.z += static_cast<float>(xOffset * rotationSensitivity);
    currentRotation.y += static_cast<float>(yOffset * rotationSensitivity);

    // Apply rotation.
    setRelativeRotation(currentRotation);
}

void EditorCameraNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    CameraNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    // Make sure we don't have a spatial node in our parent chain
    // so that nothing will affect our movement/rotation.
    auto& mtxSpatialParent = getClosestSpatialParent();
    std::scoped_lock guard(mtxSpatialParent.first);

    if (mtxSpatialParent.second != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "editor camera node was attached to some node (tree) and there is now a "
            "spatial node \"{}\" in the editor camera's parent chain but having a spatial node "
            "in the editor camera's parent chain might cause the camera to move/rotate according "
            "to the parent (which is undesirable)",
            mtxSpatialParent.second->getNodeName()));
    }
}
