#include "EditorGameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/camera/CameraManager.h"
#include "game/node/MeshNode.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/PointLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "input/EditorInputEventIds.hpp"
#include "input/InputManager.h"
#include "math/MathHelpers.hpp"
#include "node/EditorCameraNode.h"
#include "render/FontManager.h"
#include "misc/MemoryUsage.hpp"
#include "node/node_tree_inspector/NodeTreeInspector.h"
#include "node/property_inspector/PropertyInspector.h"
#include "node/content_browser/ContentBrowser.h"
#include "node/LogViewNode.h"
#include "node/menu/ContextMenuNode.h"
#include "EditorTheme.h"

#if defined(GAME_LIB_INCLUDED)
#include "MyGameInstance.h"
#endif

// External.
#include "hwinfo/hwinfo.h"
#include "utf/utf.hpp"

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
#if defined(GAME_LIB_INCLUDED)
    MyGameInstance::registerGameTypes();
#endif

    getRenderer()->getFontManager().loadFont(
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "font.ttf", 0.05F);

    registerEditorInputEvents();

    // Create editor's world.
    createWorld([this](Node* pRootNode) {
        editorWorldNodes.pRoot = pRootNode;
        attachEditorNodes(pRootNode);
    });
}

void EditorGameInstance::onGamepadConnected(std::string_view sGamepadName) {
    if (gameWorldNodes.pViewportCamera == nullptr) {
        return;
    }
    gameWorldNodes.pViewportCamera->onGamepadConnected();
}

void EditorGameInstance::onGamepadDisconnected() {
    if (gameWorldNodes.pViewportCamera == nullptr) {
        return;
    }

    gameWorldNodes.pViewportCamera->onGamepadDisconnected();
}

void EditorGameInstance::onKeyboardButtonReleased(KeyboardButton key, KeyboardModifiers modifiers) {
    if (gameWorldNodes.pRoot == nullptr || editorWorldNodes.pContentBrowser == nullptr) {
        return;
    }

    if (!lastOpenedNodeTree.has_value()) {
        return;
    }

    if (!std::filesystem::exists(*lastOpenedNodeTree)) {
        return;
    }

    if (modifiers.isControlPressed() && key == KeyboardButton::S) {
        const auto optionalError = gameWorldNodes.pRoot->serializeNodeTree(*lastOpenedNodeTree, false);
        if (optionalError.has_value()) [[unlikely]] {
            Logger::get().error(std::format(
                "failed to save node tree to \"{}\", error: {}",
                lastOpenedNodeTree->filename().string(),
                optionalError->getInitialMessage()));
        } else {
            Logger::get().info(
                std::format("node tree saved to \"{}\"", lastOpenedNodeTree->filename().string()));
        }
    }
}

void EditorGameInstance::onBeforeNewFrame(float timeSincePrevCallInSec) {
    if (gameWorldNodes.pStatsText == nullptr) {
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
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
        } else if (ratio >= 0.75F) { // NOLINT
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0F, 1.0F, 0.0F, 1.0F));
        } else {
            gameWorldNodes.pStatsText->setTextColor(glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
        }

        // Render.
        sStatsText += std::format(
            "\nFPS: {} (limit: {})",
            getRenderer()->getRenderStatistics().getFramesPerSecond(),
            getRenderer()->getFpsLimit());

        // Done.
        gameWorldNodes.pStatsText->setText(utf::as_u16(sStatsText));
        timeBeforeStatsUpdate = 1.0F;
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
        return;
    }

    if (editorWorldNodes.pContentBrowser == nullptr) {
        return;
    }

    editorWorldNodes.pContentBrowser->rebuildFileTree();
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
    }

    // Bind to action events.
    {
        auto& mtxActionEvents = getActionEventBindings();
        std::scoped_lock guard(mtxActionEvents.first);

        // Capture mouse.
        mtxActionEvents.second[static_cast<unsigned int>(EditorInputEventIds::Action::CAPTURE_MOUSE_CURSOR)] =
            [this](KeyboardModifiers modifiers, bool bIsPressed) {
                if (gameWorldNodes.pRoot == nullptr) {
                    return;
                }
                if (bIsPressed) {
                    auto optCursorInGameViewport = gameWorldNodes.pRoot->getWorldWhileSpawned()
                                                       ->getCameraManager()
                                                       .getCursorPosOnViewport();
                    if (!optCursorInGameViewport.has_value()) {
                        return;
                    }
                    getWindow()->setCursorVisibility(false);
                    gameWorldNodes.pViewportCamera->setIsMouseCaptured(true);
                } else if (!getWindow()->isCursorVisible()) {
                    getWindow()->setCursorVisibility(true);
                    gameWorldNodes.pViewportCamera->setIsMouseCaptured(false);
                }
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

        // We separate keyboard and gamepad input events because our editor camera node needs it
        // (it's not required to separate them).

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

void EditorGameInstance::attachEditorNodes(Node* pRootNode) {
    // Spawn some camera to view editor's UI.
    const auto pCamera = pRootNode->addChildNode(std::make_unique<CameraNode>());
    pCamera->makeActive(false);

    editorWorldNodes.pContextMenu = pRootNode->addChildNode(std::make_unique<ContextMenuNode>());

    const auto pHorizontalLayout = pRootNode->addChildNode(std::make_unique<LayoutUiNode>());
    pHorizontalLayout->setPosition(glm::vec2(0.0F, 0.0F));
    pHorizontalLayout->setSize(glm::vec2(1.0F, 1.0F));
    pHorizontalLayout->setIsHorizontal(true);
    pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    {
        // Left panel: node tree and content browser.
        const auto pLeftRect = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
        pLeftRect->setColor(EditorTheme::getEditorBackgroundColor());
        {
            const auto pLayout = pLeftRect->addChildNode(std::make_unique<LayoutUiNode>());
            pLayout->setPadding(EditorTheme::getPadding() / 2.0F);
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
            pFloor->setRelativeScale(glm::vec3(200.0F, 200.0F, 1.0F));
            pFloor->getMaterial().setDiffuseColor(glm::vec3(0.0F, 0.5F, 0.0F));
            pGameRootNode->addChildNode(std::move(pFloor));

            auto pCube = std::make_unique<MeshNode>("Cube");
            pCube->setRelativeLocation(glm::vec3(2.0F, 0.0F, 1.0F));
            pCube->getMaterial().setDiffuseColor(glm::vec3(0.5F, 0.0F, 0.0F));
            pGameRootNode->addChildNode(std::move(pCube));

            auto pSun = std::make_unique<DirectionalLightNode>();
            pSun->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(1.0F, 1.0F, -1.0F))));
            pGameRootNode->addChildNode(std::move(pSun));

            auto pSpotlight = std::make_unique<SpotlightNode>();
            pSpotlight->setRelativeLocation(glm::vec3(5.0F, 4.0F, 4.0F));
            pSpotlight->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(-1.0F, -1.0F, -2.0F))));
            pGameRootNode->addChildNode(std::move(pSpotlight));

            auto pPointLight = std::make_unique<PointLightNode>();
            pPointLight->setRelativeLocation(glm::vec3(2.0F, -5.0F, 2.0F));
            pGameRootNode->addChildNode(std::move(pPointLight));

            onAfterGameWorldCreated(pGameRootNode);
        },
        false);
}

void EditorGameInstance::createGameWorld() {
    createWorld(
        [this](Node* pGameRootNode) {
            auto pFloor = std::make_unique<MeshNode>("Floor");
            pFloor->setRelativeScale(glm::vec3(200.0F, 200.0F, 1.0F));
            pFloor->getMaterial().setDiffuseColor(glm::vec3(0.0F, 0.5F, 0.0F));
            pGameRootNode->addChildNode(std::move(pFloor));

            auto pCube = std::make_unique<MeshNode>("Cube");
            pCube->setRelativeLocation(glm::vec3(2.0F, 0.0F, 1.0F));
            pCube->getMaterial().setDiffuseColor(glm::vec3(0.5F, 0.0F, 0.0F));
            pGameRootNode->addChildNode(std::move(pCube));

            auto pSun = std::make_unique<DirectionalLightNode>();
            pSun->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(1.0F, 1.0F, -1.0F))));
            pGameRootNode->addChildNode(std::move(pSun));

            auto pSpotlight = std::make_unique<SpotlightNode>();
            pSpotlight->setRelativeLocation(glm::vec3(5.0F, 4.0F, 4.0F));
            pSpotlight->setRelativeRotation(MathHelpers::convertNormalizedDirectionToRollPitchYaw(
                glm::normalize(glm::vec3(-1.0F, -1.0F, -2.0F))));
            pGameRootNode->addChildNode(std::move(pSpotlight));

            auto pPointLight = std::make_unique<PointLightNode>();
            pPointLight->setRelativeLocation(glm::vec3(2.0F, -5.0F, 2.0F));
            pGameRootNode->addChildNode(std::move(pPointLight));

            onAfterGameWorldCreated(pGameRootNode);
        },
        false);
}

void EditorGameInstance::onAfterGameWorldCreated(Node* pRootNode) {
    gameWorldNodes.pRoot = pRootNode;

    if (editorWorldNodes.pViewportUiPlaceholder == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor's viewport UI node to be created at this point");
    }

    // Viewport camera.
    gameWorldNodes.pViewportCamera = pRootNode->addChildNode(std::make_unique<EditorCameraNode>(
        std::format("{}: camera", NodeTreeInspector::getHiddenNodeNamePrefix())));
    gameWorldNodes.pViewportCamera->setSerialize(false);
    gameWorldNodes.pViewportCamera->setRelativeLocation(glm::vec3(-2.0F, 0.0F, 2.0F));
    gameWorldNodes.pViewportCamera->makeActive();
    if (isGamepadConnected()) {
        gameWorldNodes.pViewportCamera->onGamepadConnected();
    }
    const auto pos = editorWorldNodes.pViewportUiPlaceholder->getPosition();
    const auto size = editorWorldNodes.pViewportUiPlaceholder->getSize();
    gameWorldNodes.pViewportCamera->getCameraProperties()->setViewport(
        glm::vec4(pos.x, pos.y, size.x, size.y));

    // Stats.
    gameWorldNodes.pStatsText = pRootNode->addChildNode(
        std::make_unique<TextUiNode>(std::format("{}: stats", NodeTreeInspector::getHiddenNodeNamePrefix())));
    gameWorldNodes.pStatsText->setSerialize(false);
    gameWorldNodes.pStatsText->setTextHeight(0.035F);
    gameWorldNodes.pStatsText->setSize(glm::vec2(1.0F, 1.0F));
    gameWorldNodes.pStatsText->setPosition(glm::vec2(0.005F, 0.0F));

    editorWorldNodes.pNodeTreeInspector->onGameNodeTreeLoaded(gameWorldNodes.pRoot);
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
            false);
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

bool EditorGameInstance::isContextMenuOpened() const {
    return editorWorldNodes.pContextMenu != nullptr && editorWorldNodes.pContextMenu->isVisible();
}

PropertyInspector* EditorGameInstance::getPropertyInspector() const {
    return editorWorldNodes.pPropertyInspector;
}

NodeTreeInspector* EditorGameInstance::getNodeTreeInspector() const {
    return editorWorldNodes.pNodeTreeInspector;
}
