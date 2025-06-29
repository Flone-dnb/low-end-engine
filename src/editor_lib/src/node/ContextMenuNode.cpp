#include "node/ContextMenuNode.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "EditorColorTheme.h"
#include "game/GameInstance.h"
#include "game/Window.h"

namespace {
    constexpr std::string_view sTypeGuid = "f50b54c3-322b-49f8-9ff2-2fd1faf03cc9";
}

std::string ContextMenuNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string ContextMenuNode::getTypeGuid() const { return sTypeGuid.data(); }

ContextMenuNode::ContextMenuNode() : ContextMenuNode("Context Menu Node") {}
ContextMenuNode::ContextMenuNode(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setIsReceivingInput(true); // for onMouseLeft to work
    setIsVisible(false);
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorColorTheme::getPadding());
    setColor(EditorColorTheme::getContainerBackgroundColor());
    setModal();

    pButtonsLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pButtonsLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
}

void ContextMenuNode::closeMenu() {
    // Despawn old buttons.
    const auto mtxChildNodes = pButtonsLayout->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    for (const auto& pNode : mtxChildNodes.second) {
        pNode->unsafeDetachFromParentAndDespawn(true);
    }

    setIsVisible(false);
}

void ContextMenuNode::onMouseLeft() {
    RectUiNode::onMouseLeft();

    closeMenu();
}

void ContextMenuNode::openMenu(
    const std::vector<std::pair<std::u16string, std::function<void()>>>& vMenuItems) {
    // Get cursor position.
    const auto pWindow = getGameInstanceWhileSpawned()->getWindow();
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();
    const auto [iCursorX, iCursorY] = pWindow->getCursorPosition();
    const auto cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWindowWidth),
        static_cast<float>(iCursorY) / static_cast<float>(iWindowHeight));

    setPosition(cursorPos - 0.01F); // move slightly to make 1st menu item to be hovered

    float totalSizeY = 0.0F;

    for (const auto& [sName, callback] : vMenuItems) {
        auto pButton = std::make_unique<ButtonUiNode>();
        pButton->setUiLayer(UiLayer::LAYER2);
        pButton->setSize(glm::vec2(pButton->getSize().x, EditorColorTheme::getButtonSizeY()));
        pButton->setPadding(EditorColorTheme::getPadding());
        pButton->setColor(EditorColorTheme::getButtonColor());
        pButton->setColorWhileHovered(EditorColorTheme::getButtonHoverColor());
        pButton->setColorWhilePressed(EditorColorTheme::getButtonPressedColor());
        pButton->setOnClicked([this, callback]() {
            callback();
            closeMenu();
        });
        {
            const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
            pText->setText(sName);
            pText->setTextHeight(EditorColorTheme::getTextHeight());
        }

        totalSizeY += pButton->getSize().y;
        pButtonsLayout->addChildNode(std::move(pButton));
    }

    setSize(glm::vec2(0.1F, totalSizeY));
    setIsVisible(true);
}
