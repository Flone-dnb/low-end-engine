#include "node/menu/ContextMenuNode.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "EditorColorTheme.h"
#include "game/GameInstance.h"
#include "game/Window.h"

// External.
#include "utf/utf.hpp"

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

    if (!bIsProcessingButtonClick) {
        closeMenu();
    }
}

void ContextMenuNode::openMenu(
    const std::vector<std::pair<std::u16string, std::function<void()>>>& vMenuItems,
    const std::u16string& sTitle) {
    if (isVisible()) {
        // Close previous menu.
        closeMenu();
    }

    // Get cursor position.
    const auto pWindow = getGameInstanceWhileSpawned()->getWindow();
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();
    const auto [iCursorX, iCursorY] = pWindow->getCursorPosition();
    const auto cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWindowWidth),
        static_cast<float>(iCursorY) / static_cast<float>(iWindowHeight));

    setPosition(cursorPos - 0.01F); // move slightly to make 1st menu item to be hovered

    float totalSizeY = 0.0F;

    if (!sTitle.empty()) {
        auto pText = std::make_unique<TextUiNode>();
        pText->setTextHeight(EditorColorTheme::getTextHeight());
        pText->setText(sTitle);

        totalSizeY += EditorColorTheme::getTextHeight();

        pButtonsLayout->addChildNode(std::move(pText));
    }

    for (const auto& [sName, callback] : vMenuItems) {
        auto pButton =
            std::make_unique<ButtonUiNode>(std::format("Context menu option \"{}\"", utf::as_str8(sName)));
        pButton->setSize(glm::vec2(pButton->getSize().x, EditorColorTheme::getButtonSizeY()));
        pButton->setPadding(EditorColorTheme::getPadding());
        pButton->setColor(EditorColorTheme::getButtonColor());
        pButton->setColorWhileHovered(EditorColorTheme::getButtonHoverColor());
        pButton->setColorWhilePressed(EditorColorTheme::getButtonPressedColor());
        pButton->setOnClicked([this, callback]() {
            bIsProcessingButtonClick = true;
            callback();
            bIsProcessingButtonClick = false;
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
