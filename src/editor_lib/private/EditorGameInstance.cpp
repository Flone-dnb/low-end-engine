#include "EditorGameInstance.h"

// Custom.
#include "input/InputManager.h"
#include "game/Window.h"
#include "input/EditorInputEventIds.hpp"
#include "game/camera/CameraManager.h"
#include "node/EditorCameraNode.h"
#include "game/node/MeshNode.h"
#include "math/MathHelpers.hpp"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/light/PointLightNode.h"
#include "game/node/ui/TextNode.h"

// External.
#include "hwinfo/hwinfo.h"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
    registerEditorInputEvents();

    // Create world.
    createWorld([this]() { addEditorNodesToCurrentWorld(); });
}

void EditorGameInstance::onGamepadConnected(std::string_view sGamepadName) {
    if (getWorldRootNode() == nullptr) {
        return;
    }

    const auto pEditorCameraNode = getEditorCameraNode();
    pEditorCameraNode->setIgnoreInput(false);
}

void EditorGameInstance::onGamepadDisconnected() {
    if (getWorldRootNode() == nullptr) {
        return;
    }

    const auto pEditorCameraNode = getEditorCameraNode();
    pEditorCameraNode->setIgnoreInput(true);
}

void EditorGameInstance::onBeforeNewFrame(float timeSincePrevCallInSec) {
    if (pStatsTextNode == nullptr) {
        return;
    }

    static float timeBeforeStatsUpdate = 0.0F;
    timeBeforeStatsUpdate -= timeSincePrevCallInSec;

    if (timeBeforeStatsUpdate <= 0.0F) {
        // Prepare stats to display.
        std::string sStatsText;

        // RAM.
        hwinfo::Memory memory;
        const auto iRamUsedMb = memory.available_Bytes() / 1024 / 1024; // NOLINT
        const auto iRamTotalMb = memory.total_Bytes() / 1024 / 1024;    // NOLINT
        const auto ratio = static_cast<float>(iRamUsedMb) / static_cast<float>(iRamTotalMb);

        sStatsText += std::format("RAM (MB): {}/{}", iRamUsedMb, iRamTotalMb);
        if (ratio >= 0.9F) { // NOLINT
            pStatsTextNode->setTextColor(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
        } else if (ratio >= 0.75F) { // NOLINT
            pStatsTextNode->setTextColor(glm::vec4(1.0F, 1.0F, 0.0F, 1.0F));
        } else {
            pStatsTextNode->setTextColor(glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
        }

        // Render.
        sStatsText += std::format(
            "\nFPS: {} (limit: {})",
            getRenderer()->getRenderStatistics().getFramesPerSecond(),
            getRenderer()->getFpsLimit());

        // Done.
        pStatsTextNode->setText(sStatsText);
        timeBeforeStatsUpdate = 1.0F;
    }
}

void EditorGameInstance::registerEditorInputEvents() {
    // Prepare a handy lambda.
    const auto showErrorIfNotEmpty = [](const std::optional<Error>& optionalError) {
        if (optionalError.has_value()) [[unlikely]] {
            auto error = *optionalError;
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
    };

    // Register action events.
    {
        // Close editor.
        showErrorIfNotEmpty(
            getInputManager()->addActionEvent(
                static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE_EDITOR),
                {KeyboardButton::ESCAPE, GamepadButton::BACK}));

        // Capture mouse.
        showErrorIfNotEmpty(
            getInputManager()->addActionEvent(
                static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR),
                {MouseButton::RIGHT}));

        // Increase camera speed.
        showErrorIfNotEmpty(
            getInputManager()->addActionEvent(
                static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_SPEED),
                {KeyboardButton::LEFT_SHIFT}));

        // Decrease camera speed.
        showErrorIfNotEmpty(
            getInputManager()->addActionEvent(
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
                if (isGamepadConnected()) {
                    return;
                }

                const auto pEditorCameraNode = getEditorCameraNode();

                getWindow()->setCursorVisibility(!bIsPressed);
                pEditorCameraNode->setIgnoreInput(!bIsPressed);
            };
    }

    // Register axis events.
    {
        // Move forward.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD),
                {{KeyboardButton::W, KeyboardButton::S}},
                {}));

        // Move right.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT),
                {{KeyboardButton::D, KeyboardButton::A}},
                {}));

        // Gamepad move forward.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_FORWARD),
                {},
                {GamepadAxis::LEFT_STICK_Y}));

        // Gamepad move right.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_RIGHT),
                {},
                {GamepadAxis::LEFT_STICK_X}));

        // Move up.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP),
                {{KeyboardButton::E, KeyboardButton::Q}},
                {}));

        // Gamepad look right.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_RIGHT),
                {},
                {GamepadAxis::RIGHT_STICK_X}));

        // Gamepad look up.
        showErrorIfNotEmpty(
            getInputManager()->addAxisEvent(
                static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_UP),
                {},
                {GamepadAxis::RIGHT_STICK_Y}));
    }
}

void EditorGameInstance::addEditorNodesToCurrentWorld() {
    // Editor camera.
    const auto pEditorCameraNode = getWorldRootNode()->addChildNode(std::make_unique<EditorCameraNode>());
    pEditorCameraNode->setRelativeLocation(glm::vec3(-2.0F, 0.0F, 2.0F)); // NOLINT
    pEditorCameraNode->makeActive();
    if (isGamepadConnected()) {
        pEditorCameraNode->setIgnoreInput(false);
    }

    // Stats.
    pStatsTextNode = getWorldRootNode()->addChildNode(std::make_unique<TextNode>());
    pStatsTextNode->setTextSize(0.03F);                                                          // NOLINT
    pStatsTextNode->setPosition(glm::vec2(0.01F, 1.0F - 0.01F - pStatsTextNode->getTextSize())); // NOLINT

    // Stuff for testing.
    {
        auto pFloor = std::make_unique<MeshNode>();
        pFloor->setRelativeScale(glm::vec3(50.0F, 50.0F, 1.0F));            // NOLINT
        pFloor->getMaterial().setDiffuseColor(glm::vec3(0.1F, 0.0F, 0.1F)); // NOLINT
        getWorldRootNode()->addChildNode(std::move(pFloor));

        auto pCube = std::make_unique<MeshNode>();
        pCube->setRelativeLocation(glm::vec3(2.0F, 0.0F, 1.0F));           // NOLINT
        pCube->getMaterial().setDiffuseColor(glm::vec3(0.9F, 0.3F, 0.0F)); // NOLINT
        getWorldRootNode()->addChildNode(std::move(pCube));

        auto pSun = std::make_unique<DirectionalLightNode>();
        pSun->setLightIntensity(0.2F); // NOLINT
        pSun->setRelativeRotation(
            MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(1.0F, 1.0F, -1.0F)))); // NOLINT
        getWorldRootNode()->addChildNode(std::move(pSun));

        auto pSpotlight = std::make_unique<SpotlightNode>();
        pSpotlight->setRelativeLocation(glm::vec3(5.0F, 4.0F, 4.0F)); // NOLINT
        pSpotlight->setRelativeRotation(
            MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(-1.0F, -1.0F, -2.0F)))); // NOLINT
        getWorldRootNode()->addChildNode(std::move(pSpotlight));

        auto pPointLight = std::make_unique<PointLightNode>();
        pPointLight->setRelativeLocation(glm::vec3(2.0F, -5.0F, 2.0F)); // NOLINT
        getWorldRootNode()->addChildNode(std::move(pPointLight));
    }
}

EditorCameraNode* EditorGameInstance::getEditorCameraNode() {
    // Get active camera.
    auto& mtxActiveCamera = getCameraManager()->getActiveCamera();
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second == nullptr) {
        Error::showErrorAndThrowException("expected a camera to be active");
    }

    // Cast to editor camera.
    const auto pEditorCameraNode = dynamic_cast<EditorCameraNode*>(mtxActiveCamera.second);
    if (pEditorCameraNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "expected the active camera to be an editor camera (node \"{}\")",
                mtxActiveCamera.second->getNodeName()));
    }

    return pEditorCameraNode;
}
