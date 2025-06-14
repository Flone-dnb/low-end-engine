#include "EditorGameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/camera/CameraManager.h"
#include "game/node/MeshNode.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/PointLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/ui/TextUiNode.h"
#include "input/EditorInputEventIds.hpp"
#include "input/InputManager.h"
#include "math/MathHelpers.hpp"
#include "misc/EditorNodeCreationHelpers.hpp"
#include "node/EditorCameraNode.h"
#include "render/FontManager.h"
#include "misc/MemoryUsage.hpp"

#if defined(GAME_LIB_INCLUDED)
#include "MyGameInstance.h"
#endif

// External.
#include "hwinfo/hwinfo.h"
#include "utf/utf.hpp"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
#if defined(GAME_LIB_INCLUDED)
    MyGameInstance::registerGameTypes();
#endif

    getRenderer()->getFontManager().loadFont(
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "RedHatDisplay-Light.ttf");

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
        const auto iRamTotalMb = memory.total_Bytes() / 1024 / 1024;
        const auto iRamFreeMb = memory.available_Bytes() / 1024 / 1024;
        const auto iRamUsedMb = iRamTotalMb - iRamFreeMb;
        const auto ratio = static_cast<float>(iRamUsedMb) / static_cast<float>(iRamTotalMb);
        const auto iAppRamMb = getCurrentRSS() / 1024 / 1024;

        sStatsText += std::format("RAM used (MB): {} ({}/{})", iAppRamMb, iRamUsedMb, iRamTotalMb);
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
        pStatsTextNode->setText(utf::as_u16(sStatsText));
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
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE_EDITOR),
            {KeyboardButton::ESCAPE, GamepadButton::BACK}));

        // Capture mouse.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR),
            {MouseButton::RIGHT}));

        // Increase camera movement speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_MOVEMENT_SPEED),
            {KeyboardButton::LEFT_SHIFT}));

        // Decrease camera movement speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_MOVEMENT_SPEED),
            {KeyboardButton::LEFT_CONTROL}));

        // Increase camera rotation speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_ROTATION_SPEED),
            {GamepadButton::RIGHT_SHOULDER}));

        // Decrease camera rotation speed.
        showErrorIfNotEmpty(getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_ROTATION_SPEED),
            {GamepadButton::LEFT_SHOULDER}));
    }

    // Bind to action events.
    {
        auto& mtxActionEvents = getActionEventBindings();
        std::scoped_lock guard(mtxActionEvents.first);

        // Close editor.
        mtxActionEvents.second[static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE_EDITOR)] =
            [this](KeyboardModifiers modifiers, bool bIsPressedDown) {
                if (!bIsPressedDown) {
                    getWindow()->close();
                }
            };

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
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD),
            {{KeyboardButton::W, KeyboardButton::S}},
            {}));

        // Move right.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT),
            {{KeyboardButton::D, KeyboardButton::A}},
            {}));

        // Gamepad move forward.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_FORWARD),
            {},
            {GamepadAxis::LEFT_STICK_Y}));

        // Gamepad move right.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_RIGHT),
            {},
            {GamepadAxis::LEFT_STICK_X}));

        // Move up.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP),
            {{KeyboardButton::E, KeyboardButton::Q}},
            {}));

        // Gamepad look right.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_RIGHT),
            {},
            {GamepadAxis::RIGHT_STICK_X}));

        // Gamepad look up.
        showErrorIfNotEmpty(getInputManager()->addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_UP),
            {},
            {GamepadAxis::RIGHT_STICK_Y}));
    }
}

void EditorGameInstance::addEditorNodesToCurrentWorld() {
    // Editor camera.
    const auto pEditorCameraNode = getWorldRootNode()->addChildNode(createEditorNode<EditorCameraNode>());
    pEditorCameraNode->setRelativeLocation(glm::vec3(-2.0F, 0.0F, 2.0F));
    pEditorCameraNode->makeActive();
    if (isGamepadConnected()) {
        pEditorCameraNode->setIgnoreInput(false);
    }

    // Stats.
    pStatsTextNode = getWorldRootNode()->addChildNode(createEditorNode<TextUiNode>());
    pStatsTextNode->setSize(glm::vec2(1.0F, 1.0F));
    pStatsTextNode->setPosition(glm::vec2(0.0F, 0.0F));

    // Stuff for testing.
    {
        auto pFloor = createEditorNode<MeshNode>();
        pFloor->setRelativeScale(glm::vec3(200.0F, 200.0F, 1.0F));
        pFloor->getMaterial().setDiffuseColor(glm::vec3(0.0F, 0.5F, 0.0F));
        getWorldRootNode()->addChildNode(std::move(pFloor));

        auto pCube = createEditorNode<MeshNode>();
        pCube->setRelativeLocation(glm::vec3(2.0F, 0.0F, 1.0F));
        pCube->getMaterial().setDiffuseColor(glm::vec3(0.5F, 0.0F, 0.0F));
        getWorldRootNode()->addChildNode(std::move(pCube));

        auto pSun = createEditorNode<DirectionalLightNode>();
        pSun->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
            glm::normalize(glm::vec3(1.0F, 1.0F, -1.0F))));
        getWorldRootNode()->addChildNode(std::move(pSun));

        auto pSpotlight = createEditorNode<SpotlightNode>();
        pSpotlight->setRelativeLocation(glm::vec3(5.0F, 4.0F, 4.0F));
        pSpotlight->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
            glm::normalize(glm::vec3(-1.0F, -1.0F, -2.0F))));
        getWorldRootNode()->addChildNode(std::move(pSpotlight));

        auto pPointLight = createEditorNode<PointLightNode>();
        pPointLight->setRelativeLocation(glm::vec3(2.0F, -5.0F, 2.0F));
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
        Error::showErrorAndThrowException(std::format(
            "expected the active camera to be an editor camera (node \"{}\")",
            mtxActiveCamera.second->getNodeName()));
    }

    return pEditorCameraNode;
}
