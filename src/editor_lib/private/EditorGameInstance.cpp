#include "EditorGameInstance.h"

// Custom.
#include "input/InputManager.h"
#include "game/Window.h"
#include "input/EditorInputEventIds.hpp"
#include "game/camera/CameraManager.h"
#include "node/EditorCameraNode.h"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
    registerEditorInputEvents();

    // Create world.
    createWorld([this]() {
        // Spawn editor camera.
        const auto pEditorCameraNode = getWorldRootNode()->addChildNode(std::make_unique<EditorCameraNode>());
        pEditorCameraNode->makeActive();
    });
}

void EditorGameInstance::registerEditorInputEvents() {
    // Prepare a handy lambda.
    const auto showErrorIfNotEmpty = [](const std::optional<Error>& optionalError) {
        if (optionalError.has_value()) [[unlikely]] {
            auto error = std::move(*optionalError);
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
    };

    // Register action events.
    {
        // Close editor.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE_EDITOR),
            {KeyboardButton::ESCAPE, GamepadButton::BACK}));

        // Capture mouse.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR),
            {MouseButton::RIGHT}));

        // Increase camera speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_SPEED),
            {KeyboardButton::LEFT_SHIFT}));

        // Decrease camera speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_SPEED),
            {KeyboardButton::LEFT_CONTROL}));
    }

    // Bind to action events.
    {
        auto& mtxActionEvents = getActionEventBindings();
        std::scoped_lock guard(mtxActionEvents.first);

        // Close editor.
        mtxActionEvents.second[static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE_EDITOR)] =
            [this](KeyboardModifiers modifiers, bool bIsPressedDown) { getWindow()->close(); };

        // Capture mouse.
        mtxActionEvents.second[static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                // Get active camera.
                auto& mtxActiveCamera = getCameraManager()->getActiveCamera();
                std::scoped_lock guard(mtxActiveCamera.first);

                if (mtxActiveCamera.second == nullptr) {
                    return;
                }

                // Cast to editor camera.
                const auto pEditorCameraNode = dynamic_cast<EditorCameraNode*>(mtxActiveCamera.second);
                if (pEditorCameraNode == nullptr) [[unlikely]] {
                    Error error(std::format(
                        "expected the active camera to be an editor camera (node \"{}\")",
                        mtxActiveCamera.second->getNodeName()));
                    error.showErrorAndThrowException();
                }

                // Set cursor visibility.
                getWindow()->setCursorVisibility(!bIsPressed);

                // Notify editor camera.
                pEditorCameraNode->setIgnoreInput(!bIsPressed);
            };
    }
}
