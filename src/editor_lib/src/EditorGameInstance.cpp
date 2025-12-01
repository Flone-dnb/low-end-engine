#include "EditorGameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/camera/CameraManager.h"
#include "game/node/MeshNode.h"
#include "game/node/light/PointLightNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "input/GamepadButton.hpp"
#include "node/GizmoNode.h"
#include "input/EditorInputEventIds.hpp"
#include "input/InputManager.h"
#include "render/MeshRenderer.h"
#include "node/EditorCameraNode.h"
#include "render/FontManager.h"
#include "misc/MemoryUsage.hpp"
#include "node/node_tree_inspector/NodeTreeInspector.h"
#include "node/node_tree_inspector/NodeTreeInspectorItem.h"
#include "node/property_inspector/PropertyInspector.h"
#include "node/content_browser/ContentBrowser.h"
#include "node/LogViewNode.h"
#include "render/UiNodeManager.h"
#include "node/menu/ContextMenuNode.h"
#include "game/DebugConsole.h"
#include "render/wrapper/Buffer.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/wrapper/Texture.h"
#include "render/GpuResourceManager.h"
#include "render/PhysicsDebugDrawer.hpp"
#include "game/physics/PhysicsManager.h"
#include "render/LightSourceManager.h"
#include "EditorResourcePaths.hpp"
#include "EditorTheme.h"
#include "EditorConstants.hpp"

#if defined(GAME_LIB_INCLUDED)
#include "MyGameInstance.h"
#endif

// External.
#include "utf/utf.hpp"
#include "glad/glad.h"

EditorGameInstance::GpuPickingData::~GpuPickingData() {}

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
#if defined(GAME_LIB_INCLUDED)
    MyGameInstance::registerGameTypes();
    editorAmbientLight = MyGameInstance::getAmbientLightForEditor();
#endif

    getRenderer()->getFontManager().loadFont(
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "font.ttf", 0.05f);

    gpuPickingData.pPickingProgram = getRenderer()->getShaderManager().getShaderProgram(
        EditorResourcePaths::getPathToShadersRelativeRes() + "Picking.comp.glsl");
    gpuPickingData.pClearTextureProgram = getRenderer()->getShaderManager().getShaderProgram(
        EditorResourcePaths::getPathToShadersRelativeRes() + "ClearNodeIdTexture.comp.glsl");
    gpuPickingData.pClickedNodeIdValueBuffer = GpuResourceManager::createStorageBuffer(sizeof(unsigned int));

    registerEditorInputEvents();

    // Create editor's world.
    createWorld(
        [this](Node* pRootNode) {
            editorWorldNodes.pRoot = pRootNode;
            attachEditorNodes(pRootNode);
        },
        true,
        "editor world");
}

void EditorGameInstance::onMouseButtonPressed(MouseButton button, KeyboardModifiers modifiers) {
    if (button != MouseButton::LEFT) {
        return;
    }

    if (gameWorldNodes.pRoot == nullptr) {
        return;
    }

    const auto optCursorInViewport =
        gameWorldNodes.pRoot->getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorInViewport.has_value()) {
        return;
    }

    gpuPickingData.bMouseClickedThisTick = true;
    gpuPickingData.bLeftMouseButtonReleased = false;
}

void EditorGameInstance::onMouseButtonReleased(MouseButton button, KeyboardModifiers modifiers) {
    if (button == MouseButton::LEFT) {
        gpuPickingData.bLeftMouseButtonReleased = true;
        if (gameWorldNodes.pGizmoNode != nullptr) {
            gameWorldNodes.pGizmoNode->stopTrackingMouseMovement();
        }
    }
}

void EditorGameInstance::onKeyboardButtonReleased(KeyboardButton key, KeyboardModifiers modifiers) {
    if (gameWorldNodes.pRoot == nullptr) {
        return;
    }

    // Checking if not moving the camera by checking the cursor visibility.
    if (getWindow()->isMouseCursorVisible() && modifiers.isControlPressed() && key == KeyboardButton::S) {
        // Save node tree.

        if (!lastOpenedNodeTree.has_value()) {
            return;
        }

        if (!std::filesystem::exists(*lastOpenedNodeTree)) {
            return;
        }

        const auto optionalError = gameWorldNodes.pRoot->serializeNodeTree(*lastOpenedNodeTree, false);
        if (optionalError.has_value()) [[unlikely]] {
            Log::error(std::format(
                "failed to save node tree to \"{}\", error: {}",
                lastOpenedNodeTree->filename().string(),
                optionalError->getInitialMessage()));
        } else {
            Log::info(std::format("node tree saved to \"{}\"", lastOpenedNodeTree->filename().string()));
            if (editorWorldNodes.pContentBrowser != nullptr) {
                editorWorldNodes.pContentBrowser->rebuildFileTree();
            }
        }

        return;
    }

    if (gameWorldNodes.pGizmoNode != nullptr && getWindow()->isMouseCursorVisible() &&
        (key == KeyboardButton::W || key == KeyboardButton::E || key == KeyboardButton::R) &&
        !editorWorldNodes.pRoot->getWorldWhileSpawned()->getUiNodeManager().hasFocusedNode() &&
        !editorWorldNodes.pRoot->getWorldWhileSpawned()->getUiNodeManager().hasModalUiNodeTree()) {
        // Change gizmo mode.

        GizmoMode newMode = GizmoMode::MOVE;
        if (key == KeyboardButton::E) {
            newMode = GizmoMode::ROTATE;
        } else if (key == KeyboardButton::R) {
            newMode = GizmoMode::SCALE;
        }

        if (gameWorldNodes.pGizmoNode->getMode() != newMode) {
            const auto pNode = gameWorldNodes.pGizmoNode->getControlledNode();
            showGizmoToControlNode(pNode, newMode);
        }
    }

    if (modifiers.isControlPressed() && key == KeyboardButton::D) {
        // Duplicate the current node.
        const auto pCurrentItem = editorWorldNodes.pNodeTreeInspector->getInspectedItem();
        if (pCurrentItem == nullptr) {
            return;
        }
        const auto sNodeName = std::string(pCurrentItem->getDisplayedGameNode()->getNodeName());

        editorWorldNodes.pNodeTreeInspector->duplicateGameNode(pCurrentItem);
    }
}

void EditorGameInstance::processGpuPickingResult() {
    if (!gpuPickingData.bIsWaitingForGpuResult) [[unlikely]] {
        Error::showErrorAndThrowException("expected GPU request to be made at this point");
    }

    // Not the best approach but simple and it's not a perf-critical place.
    glFinish();

    // Map buffer for reading.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuPickingData.pClickedNodeIdValueBuffer->getBufferId());
    {
        const auto pResult = reinterpret_cast<unsigned int*>(glMapBufferRange(
            GL_SHADER_STORAGE_BUFFER,
            0,                    // offset
            sizeof(unsigned int), // size
            GL_MAP_READ_BIT));
        if (pResult != nullptr) {
            const auto iNodeIdUnderCursor = *pResult;
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            if (editorWorldNodes.pNodeTreeInspector != nullptr) {
                if (iNodeIdUnderCursor == 0) {
                    editorWorldNodes.pNodeTreeInspector->clearInspection();
                    if (gameWorldNodes.pGizmoNode != nullptr) {
                        gameWorldNodes.pGizmoNode->unsafeDetachFromParentAndDespawn(true);
                        gameWorldNodes.pGizmoNode = nullptr;
                    }
                } else {
                    bool bSelectedGizmo = false;
                    if (!gpuPickingData.bLeftMouseButtonReleased && gameWorldNodes.pGizmoNode != nullptr) {
                        if (iNodeIdUnderCursor == gameWorldNodes.pGizmoNode->getAxisNodeId(GizmoAxis::X)) {
                            bSelectedGizmo = true;
                            gameWorldNodes.pGizmoNode->trackMouseMovement(GizmoAxis::X);
                        } else if (
                            iNodeIdUnderCursor == gameWorldNodes.pGizmoNode->getAxisNodeId(GizmoAxis::Y)) {
                            bSelectedGizmo = true;
                            gameWorldNodes.pGizmoNode->trackMouseMovement(GizmoAxis::Y);
                        } else if (
                            iNodeIdUnderCursor == gameWorldNodes.pGizmoNode->getAxisNodeId(GizmoAxis::Z)) {
                            bSelectedGizmo = true;
                            gameWorldNodes.pGizmoNode->trackMouseMovement(GizmoAxis::Z);
                        }
                    }
                    if (!bSelectedGizmo) {
                        editorWorldNodes.pNodeTreeInspector->selectNodeById(iNodeIdUnderCursor);
                    }
                }
            }
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    gpuPickingData.bIsWaitingForGpuResult = false;
}

void EditorGameInstance::onBeforeNewFrame(float timeSincePrevCallInSec) {
    if (gpuPickingData.bIsWaitingForGpuResult) {
        processGpuPickingResult();
    }
    dispatchGpuPicking();

    // Run compute shader to clear node ID texture.
    if (gpuPickingData.pNodeIdTexture != nullptr) {
        GL_CHECK_ERROR(glUseProgram(gpuPickingData.pClearTextureProgram->getShaderProgramId()));

        const auto [iTexWidth, iTexHeight] = gpuPickingData.pNodeIdTexture->getSize();
        GL_CHECK_ERROR(glBindImageTexture(
            0,
            gpuPickingData.pNodeIdTexture->getTextureId(),
            0,
            GL_FALSE,
            0,
            GL_WRITE_ONLY,
            gpuPickingData.pNodeIdTexture->getGlFormat()));

        gpuPickingData.pClearTextureProgram->setUvector2ToActiveProgram(
            "textureSize", glm::uvec2(iTexWidth, iTexHeight));

        // Calculate thread group count.
        constexpr size_t iThreadGroupSizeOneDim = 16; // same as in shaders

        size_t iGroupsX = iTexWidth / iThreadGroupSizeOneDim;
        if (iTexWidth % iThreadGroupSizeOneDim != 0) {
            iGroupsX += 1;
        }
        size_t iGroupsY = iTexHeight / iThreadGroupSizeOneDim;
        if (iTexHeight % iThreadGroupSizeOneDim != 0) {
            iGroupsY += 1;
        }

        GL_CHECK_ERROR(
            glDispatchCompute(static_cast<unsigned int>(iGroupsX), static_cast<unsigned int>(iGroupsY), 1u));
    }

    updateFrameStatsText(timeSincePrevCallInSec);
}

void EditorGameInstance::updateFrameStatsText(float timeSincePrevCallInSec) {
    if (gameWorldNodes.pStatsText == nullptr) {
        return;
    }

    static float timeBeforeStatsUpdate = 0.0f;
    timeBeforeStatsUpdate -= timeSincePrevCallInSec;

    if (timeBeforeStatsUpdate <= 0.0f) {
        // Prepare stats to display.
        std::string sStatsText;

        // RAM.
        const auto iRamTotalMb = MemoryUsage::getTotalMemorySize() / 1024 / 1024;
        const auto iRamUsedMb = MemoryUsage::getTotalMemorySizeUsed() / 1024 / 1024;
        const auto ratio = static_cast<float>(iRamUsedMb) / static_cast<float>(iRamTotalMb);
        const auto iAppRamMb = MemoryUsage::getMemorySizeUsedByProcess() / 1024 / 1024;

        sStatsText += std::format("RAM used (MB): {} ({}/{})", iAppRamMb, iRamUsedMb, iRamTotalMb);
#if defined(ENGINE_ASAN_ENABLED)
        sStatsText += " (big RAM usage due to ASan)";
#endif
        if (ratio >= 0.9f) {
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        } else if (ratio >= 0.75f) {
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        } else {
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        // Render.
        sStatsText += std::format(
            "\nFPS: {} (limit: {})",
            getRenderer()->getRenderStatistics().getFramesPerSecond(),
            getRenderer()->getFpsLimit());

        // Done.
        gameWorldNodes.pStatsText->setText(utf::as_u16(sStatsText));
        timeBeforeStatsUpdate = 1.0f;
    }
}

void EditorGameInstance::onBeforeWorldDestroyed(Node* pRootNode) {
    if (pRootNode == gameWorldNodes.pRoot) {
        gameWorldNodes = {};
    } else if (pRootNode == editorWorldNodes.pRoot) {
        editorWorldNodes = {};
    }
}

void EditorGameInstance::onWindowFocusChanged(bool bIsFocused) {
    if (!bIsFocused) {
        gpuPickingData.bLeftMouseButtonReleased = true;
        if (gameWorldNodes.pGizmoNode != nullptr) {
            gameWorldNodes.pGizmoNode->stopTrackingMouseMovement();
        }
        return;
    }

    if (editorWorldNodes.pContentBrowser == nullptr) {
        return;
    }

    editorWorldNodes.pContentBrowser->rebuildFileTree();
}

void EditorGameInstance::onWindowSizeChanged() {
    if (gameWorldNodes.pViewportCamera == nullptr) {
        return;
    }
    recreateNodeIdTextureWithNewSize();
    gpuPickingData.bLeftMouseButtonReleased = true;
    if (gameWorldNodes.pGizmoNode != nullptr) {
        gameWorldNodes.pGizmoNode->stopTrackingMouseMovement();
    }
}

void EditorGameInstance::onWindowClose() {
    gpuPickingData.pPickingProgram = nullptr;
    gpuPickingData.pClearTextureProgram = nullptr;
    gpuPickingData.pClickedNodeIdValueBuffer = nullptr;
    gpuPickingData.pNodeIdTexture = nullptr;
}

void EditorGameInstance::dispatchGpuPicking() {
    PROFILE_FUNC

    if (!gpuPickingData.bMouseClickedThisTick) {
        return;
    }

    if (gameWorldNodes.pRoot == nullptr) {
        return;
    }

    gpuPickingData.bMouseClickedThisTick = false;

    const auto pEditorWorld = editorWorldNodes.pRoot->getWorldWhileSpawned();
    if (pEditorWorld->getUiNodeManager().hasModalUiNodeTree()) {
        return;
    }

    const auto optCursorInViewport =
        gameWorldNodes.pRoot->getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorInViewport.has_value()) {
        return;
    }

    GL_CHECK_ERROR(glUseProgram(gpuPickingData.pPickingProgram->getShaderProgramId()));

    // Because we render to window's framebuffer:
    const auto [iFramebufferWidth, iFramebufferHeight] = getWindow()->getWindowSize();

    // Self checks:
    if (gpuPickingData.pNodeIdTexture == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected node ID texture to be created at this point");
    }
    const auto [iTexWidth, iTexHeight] = gpuPickingData.pNodeIdTexture->getSize();
    if (iTexWidth != iFramebufferWidth || iTexHeight != iFramebufferHeight) [[unlikely]] {
        Error::showErrorAndThrowException("framebuffer size and node ID texture sizes don't match");
    }
    if (Node::peekNextNodeId() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("node IDs reached type limit for node ID texture");
    }

    // Bind shader resources.
    {
        // Node IDs must be written at this point because we will read them.
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        GL_CHECK_ERROR(glBindImageTexture(
            0,
            gpuPickingData.pNodeIdTexture->getTextureId(),
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            gpuPickingData.pNodeIdTexture->getGlFormat()));
        GL_CHECK_ERROR(glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER, 0, gpuPickingData.pClickedNodeIdValueBuffer->getBufferId()));
    }

    // Get cursor pos in fullscreen (editor camera) because viewport's framebuffer has fullscreen size.
    const auto optCursorEditor = pEditorWorld->getCameraManager().getCursorPosOnViewport();
    if (!optCursorEditor.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException("expected the cursor to be inside of the editor's camera");
    }
    const glm::uvec2 cursorPosInPix = glm::uvec2(
        static_cast<unsigned int>(optCursorEditor->x * static_cast<float>(iFramebufferWidth)),
        static_cast<unsigned int>((1.0f - optCursorEditor->y) * static_cast<float>(iFramebufferHeight)));

    gpuPickingData.pPickingProgram->setUvector2ToActiveProgram(
        "textureSize", glm::uvec2(iFramebufferWidth, iFramebufferHeight));
    gpuPickingData.pPickingProgram->setUvector2ToActiveProgram("cursorPosInPix", cursorPosInPix);

    // Calculate thread group count.
    constexpr size_t iThreadGroupSizeOneDim = 16; // same as in shaders

    size_t iGroupsX = iFramebufferWidth / iThreadGroupSizeOneDim;
    if (iFramebufferWidth % iThreadGroupSizeOneDim != 0) {
        iGroupsX += 1;
    }
    size_t iGroupsY = iFramebufferHeight / iThreadGroupSizeOneDim;
    if (iFramebufferHeight % iThreadGroupSizeOneDim != 0) {
        iGroupsY += 1;
    }

    GL_CHECK_ERROR(
        glDispatchCompute(static_cast<unsigned int>(iGroupsX), static_cast<unsigned int>(iGroupsY), 1u));
    gpuPickingData.bIsWaitingForGpuResult = true;
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
        // Capture mouse.
        showErrorIfNotEmpty(getInputManager().addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR),
            {MouseButton::RIGHT}));

        // Toggle stats.
        showErrorIfNotEmpty(getInputManager().addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::GAMEPAD_TOGGLE_STATS),
            {GamepadButton::BACK}));

        // Close editor.
        showErrorIfNotEmpty(getInputManager().addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::GAMEPAD_CLOSE_EDITOR),
            {GamepadButton::START}));

        // Increase camera movement speed.
        showErrorIfNotEmpty(getInputManager().addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::INCREASE_CAMERA_MOVEMENT_SPEED),
            {KeyboardButton::LEFT_SHIFT}));

        // Decrease camera movement speed.
        showErrorIfNotEmpty(getInputManager().addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::DECREASE_CAMERA_MOVEMENT_SPEED),
            {KeyboardButton::LEFT_CONTROL}));
    }

    // Bind to action events.
    {
        // Capture mouse.
        getActionEventBindings()[static_cast<unsigned int>(
            EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR)] = ActionEventCallbacks{
            .onPressed =
                [this](KeyboardModifiers modifiers) {
                    if (gameWorldNodes.pRoot == nullptr) {
                        return;
                    }

                    auto optCursorInGameViewport = gameWorldNodes.pRoot->getWorldWhileSpawned()
                                                       ->getCameraManager()
                                                       .getCursorPosOnViewport();
                    if (!optCursorInGameViewport.has_value()) {
                        return;
                    }
                    getWindow()->setIsMouseCursorVisible(false);
                    gameWorldNodes.pViewportCamera->setIsMouseCaptured(true);
                },
            .onReleased =
                [this](KeyboardModifiers modifiers) {
                    if (gameWorldNodes.pRoot == nullptr) {
                        return;
                    }

                    getWindow()->setIsMouseCursorVisible(true);
                    gameWorldNodes.pViewportCamera->setIsMouseCaptured(false);
                }};

        // Toggle stats.
        getActionEventBindings()[static_cast<unsigned int>(
            EditorInputEventIds::Action::GAMEPAD_TOGGLE_STATS)] =
            ActionEventCallbacks{.onPressed = {}, .onReleased = [this](KeyboardModifiers modifiers) {
                                     getRenderer()->setFpsLimit(0);
                                     DebugConsole::toggleStats();
                                 }};

        // Close editor.
        getActionEventBindings()[static_cast<unsigned int>(
            EditorInputEventIds::Action::GAMEPAD_CLOSE_EDITOR)] = ActionEventCallbacks{
            .onPressed = {}, .onReleased = [this](KeyboardModifiers modifiers) { getWindow()->close(); }};
    }

    // Register axis events.
    {
        // Move forward.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_FORWARD),
            {{KeyboardButton::W, KeyboardButton::S}},
            {}));

        // Move right.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_RIGHT),
            {{KeyboardButton::D, KeyboardButton::A}},
            {}));

        // We separate keyboard and gamepad input events because our editor camera node needs it
        // (it's not required to separate them).

        // Gamepad move forward.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_FORWARD),
            {},
            {GamepadAxis::LEFT_STICK_Y}));

        // Gamepad move right.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_RIGHT),
            {},
            {GamepadAxis::LEFT_STICK_X}));

        // Gamepad move up/down.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_UP),
            {},
            {GamepadAxis::RIGHT_TRIGGER}));
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_MOVE_CAMERA_DOWN),
            {},
            {GamepadAxis::LEFT_TRIGGER}));

        // Move up.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::MOVE_CAMERA_UP),
            {{KeyboardButton::E, KeyboardButton::Q}},
            {}));

        // Gamepad look right.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_RIGHT),
            {},
            {GamepadAxis::RIGHT_STICK_X}));

        // Gamepad look up.
        showErrorIfNotEmpty(getInputManager().addAxisEvent(
            static_cast<unsigned int>(EditorInputEventIds::Axis::GAMEPAD_LOOK_UP),
            {},
            {GamepadAxis::RIGHT_STICK_Y}));
    }
}

void EditorGameInstance::attachEditorNodes(Node* pRootNode) {
    // Spawn some camera to view editor's UI.
    const auto pCamera = pRootNode->addChildNode(std::make_unique<CameraNode>(
        "editor UI camera")); // name must contain "editor" to make it active in the camera manager
    pCamera->makeActive(false);

    editorWorldNodes.pContextMenu = pRootNode->addChildNode(std::make_unique<ContextMenuNode>());

    const auto pHorizontalLayout = pRootNode->addChildNode(std::make_unique<LayoutUiNode>());
    pHorizontalLayout->setPosition(glm::vec2(0.0f, 0.0f));
    pHorizontalLayout->setSize(glm::vec2(1.0f, 1.0f));
    pHorizontalLayout->setIsHorizontal(true);
    pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    {
        // Left panel: node tree and content browser.
        const auto pLeftRect = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
        pLeftRect->setColor(EditorTheme::getEditorBackgroundColor());
        {
            const auto pLayout = pLeftRect->addChildNode(std::make_unique<LayoutUiNode>());
            pLayout->setPadding(EditorTheme::getPadding() / 2.0f);
            pLayout->setChildNodeSpacing(EditorTheme::getSpacing());
            pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            {
                editorWorldNodes.pNodeTreeInspector =
                    pLayout->addChildNode(std::make_unique<NodeTreeInspector>());
                editorWorldNodes.pNodeTreeInspector->setExpandPortionInLayout(3);

                editorWorldNodes.pContentBrowser = pLayout->addChildNode(std::make_unique<ContentBrowser>());
                editorWorldNodes.pContentBrowser->setExpandPortionInLayout(2);
            }
        }

        // Middle panel: logger and viewport.
        const auto pMiddleVerticalLayout = pHorizontalLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pMiddleVerticalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        pMiddleVerticalLayout->setExpandPortionInLayout(4);
        {
            pMiddleVerticalLayout->addChildNode(std::make_unique<LogViewNode>());

            editorWorldNodes.pViewportUiPlaceholder =
                pMiddleVerticalLayout->addChildNode(std::make_unique<UiNode>());
            editorWorldNodes.pViewportUiPlaceholder->setExpandPortionInLayout(4);
        }

        // Right panel: property inspector.
        editorWorldNodes.pPropertyInspector =
            pHorizontalLayout->addChildNode(std::make_unique<PropertyInspector>());
    }

    // Create game world.
    createWorld(
        [this](Node* pGameRootNode) {
            auto pFloor = std::make_unique<MeshNode>("Floor");
            pFloor->setRelativeScale(glm::vec3(10.0f, 1.0f, 10.0f));
            pFloor->getMaterial().setDiffuseColor(glm::vec3(1.0f, 0.5f, 0.0f));
            pGameRootNode->addChildNode(std::move(pFloor));

            auto pCube = std::make_unique<MeshNode>("Cube");
            pCube->setRelativeLocation(glm::vec3(0.0f, 1.0f, -2.0f));
            pCube->getMaterial().setDiffuseColor(glm::vec3(1.0f, 1.0f, 1.0f));
            pGameRootNode->addChildNode(std::move(pCube));

            auto pPointLight = std::make_unique<PointLightNode>("Point Light");
            pPointLight->setRelativeLocation(glm::vec3(1.0f, 5.0f, -1.0f));
            pGameRootNode->addChildNode(std::move(pPointLight));

            onAfterGameWorldCreated(pGameRootNode);
        },
        false,
        "game"); // name needed for debug drawer to find game world
}

void EditorGameInstance::onAfterGameWorldCreated(Node* pRootNode) {
    gameWorldNodes.pRoot = pRootNode;

    const auto pGameWorld = pRootNode->getWorldWhileSpawned();
    pGameWorld->getLightSourceManager().setAmbientLightColor(editorAmbientLight);

    if (editorWorldNodes.pViewportUiPlaceholder == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor's viewport UI node to be created at this point");
    }

    // Viewport camera.
    gameWorldNodes.pViewportCamera = pRootNode->addChildNode(std::make_unique<EditorCameraNode>(std::format(
        "{}: editor viewport camera", // name must contain "editor" to make it active in the camera manager
        EditorConstants::getHiddenNodeNamePrefix())));
    gameWorldNodes.pViewportCamera->setSerialize(false);
    gameWorldNodes.pViewportCamera->setRelativeLocation(glm::vec3(0.0f, 3.0f, 1.0f));
    gameWorldNodes.pViewportCamera->makeActive();
    const auto pos = editorWorldNodes.pViewportUiPlaceholder->getPosition();
    const auto size = editorWorldNodes.pViewportUiPlaceholder->getSize();
    gameWorldNodes.pViewportCamera->getCameraProperties()->setViewport(
        glm::vec4(pos.x, pos.y, size.x, size.y));

    recreateNodeIdTextureWithNewSize();

    // Stats.
    gameWorldNodes.pStatsText = pRootNode->addChildNode(
        std::make_unique<TextUiNode>(std::format("{}: stats", EditorConstants::getHiddenNodeNamePrefix())));
    gameWorldNodes.pStatsText->setSerialize(false);
    gameWorldNodes.pStatsText->setTextHeight(0.03f);
    gameWorldNodes.pStatsText->setSize(glm::vec2(1.0f, 1.0f));
    gameWorldNodes.pStatsText->setPosition(glm::vec2(0.005f, 0.0f));

    editorWorldNodes.pNodeTreeInspector->onGameNodeTreeLoaded(gameWorldNodes.pRoot);

    // Collision draw mode.
    {
        const auto pCollisionDrawModeText = pRootNode->addChildNode(std::make_unique<TextUiNode>());
        pCollisionDrawModeText->setTextHeight(0.025f);
        pCollisionDrawModeText->setPosition(glm::vec2(0.79f, 0.005f));
        pCollisionDrawModeText->setSize(glm::vec2(0.2f, 0.05f));
        pCollisionDrawModeText->setText(u"draw collision as wireframe: ");

        auto& debugDrawer =
            pRootNode->getWorldWhileSpawned()->getGameManager().getPhysicsManager().getPhysicsDebugDrawer();

        const auto pCollisionDrawModeButton = pRootNode->addChildNode(std::make_unique<ButtonUiNode>());
        pCollisionDrawModeButton->setPosition(glm::vec2(0.96f, 0.0075f));
        pCollisionDrawModeButton->setSize(glm::vec2(0.03f, 0.03f));
        pCollisionDrawModeButton->setPadding(0.05f);

        const auto pButtonText = pCollisionDrawModeButton->addChildNode(std::make_unique<TextUiNode>());
        pButtonText->setTextHeight(0.02f);
        pButtonText->setText(debugDrawer.getDrawAsWireframe() ? u"ON" : u"OFF");

        pCollisionDrawModeButton->setOnClicked([pButtonText, pRootNode]() {
            auto& debugDrawer = pRootNode->getWorldWhileSpawned()
                                    ->getGameManager()
                                    .getPhysicsManager()
                                    .getPhysicsDebugDrawer();
            debugDrawer.setDrawAsWireframe(!debugDrawer.getDrawAsWireframe());
            pButtonText->setText(debugDrawer.getDrawAsWireframe() ? u"ON" : u"OFF");
        });
    }

    // Bind node ID texture for GPU picking.
    pGameWorld->getMeshRenderer().getGlobalShaderConstantsSetter().addSetterFunction([this](ShaderProgram*
                                                                                                pProgram) {
        if (gpuPickingData.pNodeIdTexture != nullptr) {
            GL_CHECK_ERROR(glBindImageTexture(
                0, gpuPickingData.pNodeIdTexture->getTextureId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI));
        }
    });
}

void EditorGameInstance::recreateNodeIdTextureWithNewSize() {
    // Because we render to window's framebuffer.
    const auto [iFramebufferWidth, iFramebufferHeight] = getWindow()->getWindowSize();

    gpuPickingData.pNodeIdTexture =
        GpuResourceManager::createStorageTexture(iFramebufferWidth, iFramebufferHeight, GL_R32UI);
}

void EditorGameInstance::openContextMenu(
    const std::vector<std::pair<std::u16string, std::function<void()>>>& vMenuItems,
    const std::u16string& sTitle) {
    if (editorWorldNodes.pContextMenu == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("unable to show context menu as editor world is not created");
    }
    editorWorldNodes.pContextMenu->openMenu(vMenuItems, sTitle);
}

void EditorGameInstance::openNodeTreeAsGameWorld(const std::filesystem::path& pathToNodeTree) {
    if (gameWorldNodes.pRoot == nullptr) {
        return;
    }
    destroyWorld(gameWorldNodes.pRoot->getWorldWhileSpawned(), [this, pathToNodeTree]() {
        loadNodeTreeAsWorld(
            pathToNodeTree,
            [this, pathToNodeTree](Node* pRoot) {
                onAfterGameWorldCreated(pRoot);
                lastOpenedNodeTree = pathToNodeTree;
            },
            false,
            "game"); // name needed for debug drawer to find game world
    });
}

void EditorGameInstance::changeGameWorldRootNode(std::unique_ptr<Node> pNewGameRootNode) {
    const auto pNewGameRoot = pNewGameRootNode.get();
    gameWorldNodes.pRoot->getWorldWhileSpawned()->changeRootNode(std::move(pNewGameRootNode));

    gameWorldNodes.pRoot = pNewGameRoot;
    gameWorldNodes.pViewportCamera->makeActive();
}

void EditorGameInstance::setEnableViewportCamera(bool bEnable) {
    if (gameWorldNodes.pViewportCamera == nullptr) {
        return;
    }

    if (bEnable) {
        gameWorldNodes.pViewportCamera->makeActive();
    } else {
        gameWorldNodes.pViewportCamera->getWorldWhileSpawned()->getCameraManager().clearActiveCamera();
    }
}

void EditorGameInstance::showGizmoToControlNode(SpatialNode* pNode, GizmoMode mode) {
    if (gameWorldNodes.pGizmoNode != nullptr) {
        gameWorldNodes.pGizmoNode->unsafeDetachFromParentAndDespawn(true);
        gameWorldNodes.pGizmoNode = nullptr;
    }

    if (pNode == nullptr) {
        return;
    }

    gameWorldNodes.pGizmoNode = gameWorldNodes.pRoot->addChildNode(std::make_unique<GizmoNode>(mode, pNode));
}

bool EditorGameInstance::isContextMenuOpened() const {
    return editorWorldNodes.pContextMenu != nullptr && editorWorldNodes.pContextMenu->isVisible();
}

GizmoNode* EditorGameInstance::getGizmoNode() { return gameWorldNodes.pGizmoNode; }

PropertyInspector* EditorGameInstance::getPropertyInspector() const {
    return editorWorldNodes.pPropertyInspector;
}

NodeTreeInspector* EditorGameInstance::getNodeTreeInspector() const {
    return editorWorldNodes.pNodeTreeInspector;
}
