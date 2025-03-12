#include "EditorGameInstance.h"

// Custom.
#include "input/InputManager.h"
#include "game/Window.h"
#include "input/EditorInputEventIds.hpp"
#include "game/camera/CameraManager.h"
#include "node/EditorCameraNode.h"
#include "game/node/MeshNode.h"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
    registerEditorInputEvents();

    // Create world.
    createWorld([this]() {
        // Spawn editor camera.
        const auto pEditorCameraNode = getWorldRootNode()->addChildNode(std::make_unique<EditorCameraNode>());
        pEditorCameraNode->makeActive();

        auto pMesh = std::make_unique<MeshNode>();
        pMesh->setRelativeLocation(glm::vec3(2.0F, 0.0F, 0.0F));
        getWorldRootNode()->addChildNode(std::move(pMesh));
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
                    Error::showErrorAndThrowException(std::format(
                        "expected the active camera to be an editor camera (node \"{}\")",
                        mtxActiveCamera.second->getNodeName()));
                }

                // Set cursor visibility.
                getWindow()->setCursorVisibility(!bIsPressed);

                // Notify editor camera.
                pEditorCameraNode->setIgnoreInput(!bIsPressed);
            };
    }

    // Register axis events.
    {
        // Move forward.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD),
            {{KeyboardButton::W, KeyboardButton::S}},
            {GamepadAxis::LEFT_STICK_Y}));

        // Move right.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT),
            {{KeyboardButton::D, KeyboardButton::A}},
            {GamepadAxis::LEFT_STICK_X}));

        // Move up.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP),
            {{KeyboardButton::E, KeyboardButton::Q}},
            {GamepadAxis::LEFT_TRIGGER}));
    }
}
