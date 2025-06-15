#include "game/GameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/GameManager.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/SliderUiNode.h"
#include "game/node/ui/LayoutUiNode.h"

GameInstance::GameInstance(Window* pWindow) : pWindow(pWindow) {}

void GameInstance::createWorld(const std::function<void()>& onCreated) {
    pWindow->getGameManager()->createWorld(onCreated);
}

void GameInstance::loadNodeTreeAsWorld(
    const std::filesystem::path& pathToNodeTreeFile, const std::function<void()>& onLoaded) {
    pWindow->getGameManager()->loadNodeTreeAsWorld(pathToNodeTreeFile, onLoaded);
}

void GameInstance::addTaskToThreadPool(const std::function<void()>& task) const {
    pWindow->getGameManager()->addTaskToThreadPool(task);
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>&
GameInstance::getActionEventBindings() {
    return mtxBindedActionEvents;
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>&
GameInstance::getAxisEventBindings() {
    return mtxBindedAxisEvents;
}

void GameInstance::onInputActionEvent(
    unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxBindedActionEvents.first);

    // Find this event in the registered events.
    const auto it = mtxBindedActionEvents.second.find(iActionId);
    if (it == mtxBindedActionEvents.second.end()) {
        return;
    }

    // Call user logic.
    it->second(modifiers, bIsPressedDown);
}

void GameInstance::onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input) {
    std::scoped_lock guard(mtxBindedAxisEvents.first);

    // Find this event in the registered events.
    const auto it = mtxBindedAxisEvents.second.find(iAxisEventId);
    if (it == mtxBindedAxisEvents.second.end()) {
        return;
    }

    // Call user logic.
    it->second(modifiers, input);
}

Node* GameInstance::getWorldRootNode() const { return pWindow->getGameManager()->getWorldRootNode(); }

size_t GameInstance::getTotalSpawnedNodeCount() const {
    return pWindow->getGameManager()->getTotalSpawnedNodeCount();
}

size_t GameInstance::getCalledEveryFrameNodeCount() const {
    return pWindow->getGameManager()->getCalledEveryFrameNodeCount();
}

size_t GameInstance::getReceivingInputNodeCount() const {
    return pWindow->getGameManager()->getReceivingInputNodeCount();
}

Window* GameInstance::getWindow() const { return pWindow; }

Renderer* GameInstance::getRenderer() const { return pWindow->getGameManager()->getRenderer(); }

CameraManager* GameInstance::getCameraManager() const {
    return pWindow->getGameManager()->getCameraManager();
}

InputManager* GameInstance::getInputManager() const { return pWindow->getGameManager()->getInputManager(); }

bool GameInstance::isGamepadConnected() const { return pWindow->isGamepadConnected(); }

/**
 * Creates a transient node (not a game-owned node) with specific node settings like
 * no node serialization and etc. so that it won't affect the game.
 *
 * @return Created node.
 */
template <typename T>
    requires std::derived_from<T, Node>
static inline std::unique_ptr<T> createTempNode() {
    // Create node.
    auto pCreatedNode = std::make_unique<T>();

    // Disable serialization so that it won't be serialized as part of the game world.
    pCreatedNode->setSerialize(false);

    return pCreatedNode;
}

void GameInstance::showGammaAdjustmentScreen(
    const std::function<void()>& onAdjusted, const std::u16string& sTextOverride) {
    const auto pWorldRoot = getWorldRootNode();
    if (pWorldRoot == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the world's root node to be valid (is world created?)");
    }

    SliderUiNode* pSlider = nullptr;

    auto pBackground = createTempNode<RectUiNode>();
    pBackground->setNodeName("Gamma correction node");
    pBackground->setPosition(glm::vec2(0.0F, 0.0F));
    pBackground->setSize(glm::vec2(1.0F, 1.0F));
    pBackground->setColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    pBackground->setPadding(0.04F);
    {
        const auto pVerticalLayout = pBackground->addChildNode(createTempNode<LayoutUiNode>());
        pVerticalLayout->setChildNodeSpacing(0.1F);
        pVerticalLayout->setPadding(0.05F);
        pVerticalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        {
            // Insert a spacer.
            const auto pSpacer = pVerticalLayout->addChildNode(createTempNode<UiNode>());

            const auto pHorizontalLayout = pVerticalLayout->addChildNode(createTempNode<LayoutUiNode>());
            pHorizontalLayout->setIsHorizontal(true);
            pHorizontalLayout->setChildNodeSpacing(0.05F);
            pHorizontalLayout->setExpandPortionInLayout(10);
            pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            {
                const auto pBox1 = pHorizontalLayout->addChildNode(createTempNode<RectUiNode>());
                const auto pBox2 = pHorizontalLayout->addChildNode(createTempNode<RectUiNode>());
                const auto pBox3 = pHorizontalLayout->addChildNode(createTempNode<RectUiNode>());

                pBox1->setColor(glm::vec4(0.05F, 0.05F, 0.05F, 1.0F));
                pBox2->setColor(glm::vec4(0.5F, 0.5F, 0.5F, 1.0F));
                pBox3->setColor(glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
            }
        }

        const auto pText = pVerticalLayout->addChildNode(createTempNode<TextUiNode>());
        pText->setTextHeight(0.04F);
        pText->setExpandPortionInLayout(4);

        // Prepare text.
        std::u16string sText = u"Adjust the slider so the left-most image is barely visible.\nThen press ";
        if (isGamepadConnected()) {
            sText += u"any gamepad button to accept.";
        } else {
            sText += u"the Enter button to accept.";
        }
        if (!sTextOverride.empty()) {
            sText = sTextOverride;
        }
        pText->setText(sText);

        // Create a slider that accepts input that we need.
        class CustomSliderUiNode : public SliderUiNode {
        public:
            CustomSliderUiNode() = default;
            virtual ~CustomSliderUiNode() override = default;

            std::function<void()> onClicked;

        protected:
            virtual void onKeyboardInputWhileFocused(
                KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown) override {
                SliderUiNode::onKeyboardInputWhileFocused(button, modifiers, bIsPressedDown);
                if (bIsPressedDown && button == KeyboardButton::ENTER) {
                    onClicked();
                }
            }

            virtual void onGamepadInputWhileFocused(GamepadButton button, bool bIsPressedDown) override {
                SliderUiNode::onGamepadInputWhileFocused(button, bIsPressedDown);
                if (bIsPressedDown && (button == GamepadButton::X || button == GamepadButton::A ||
                                       button == GamepadButton::Y || button == GamepadButton::B)) {
                    onClicked();
                }
            }
        };

        // Add slider.
        const auto pCustomSlider = pVerticalLayout->addChildNode(createTempNode<CustomSliderUiNode>());
        pSlider = pCustomSlider;
        pSlider->setExpandPortionInLayout(2);
        const auto currentGamma = getRenderer()->getGamma();
        pSlider->setHandlePosition((std::clamp(currentGamma, 1.0F, 2.2F) - 1.0F) / 1.2F);
        const auto changeGamma = [this](float gamma) {
            if (pGammaAdjustmentNode == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("gamma adjustment node is invalid");
            }
            getRenderer()->setGamma(gamma);
        };
        pCustomSlider->setOnHandlePositionChanged(
            [changeGamma](float value) { changeGamma(1.0F + value * 1.2F); });
        pCustomSlider->onClicked = [this, changeGamma, pSlider]() {
            const auto gamma = 1.0F + pSlider->getHandlePosition() * 1.2F;
            changeGamma(gamma);
            pGammaAdjustmentNode->unsafeDetachFromParentAndDespawn();
            pGammaAdjustmentNode = nullptr;
        };
    }

    // Set layer for node tree.
    pBackground->setUiLayer(UiLayer::LAYER2);

    // Spawn.
    pGammaAdjustmentNode = pWorldRoot->addChildNode(std::move(pBackground));
    pGammaAdjustmentNode->setModal();
    pSlider->setFocused();
}
