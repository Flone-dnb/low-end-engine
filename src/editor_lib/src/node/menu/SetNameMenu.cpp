#include "node/menu/SetNameMenu.h"

// Custom.
#include "EditorColorTheme.h"
#include "game/GameInstance.h"
#include "game/Window.h"
#include "game/node/ui/TextEditUiNode.h"

SetNameMenu::SetNameMenu() : SetNameMenu("Set Name Menu Node") {}
SetNameMenu::SetNameMenu(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setIsReceivingInput(true);
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorColorTheme::getPadding());
    setColor(EditorColorTheme::getContainerBackgroundColor());
    setSize(glm::vec2(0.15F, EditorColorTheme::getTextHeight() * 1.2F));
    setModal();

    pTextEditNode = addChildNode(std::make_unique<TextEditUiNode>());
    pTextEditNode->setTextHeight(EditorColorTheme::getTextHeight());
    pTextEditNode->setText(u"");
    pTextEditNode->setHandleNewLineChars(false);
    pTextEditNode->setOnEnterPressed([this](std::u16string_view sText) {
        if (!onNameChanged) [[unlikely]] {
            Error::showErrorAndThrowException("expected the callback to be set");
        }
        bIsDestroyHandled = true;
        onNameChanged(std::u16string(sText));
        unsafeDetachFromParentAndDespawn(true);
    });
}

void SetNameMenu::setInitialText(std::u16string_view sText) { pTextEditNode->setText(sText); }

void SetNameMenu::setOnNameChanged(const std::function<void(std::u16string)>& onNameChanged) {
    this->onNameChanged = onNameChanged;
}

void SetNameMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    // Get cursor position.
    const auto pWindow = getGameInstanceWhileSpawned()->getWindow();
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();
    const auto [iCursorX, iCursorY] = pWindow->getCursorPosition();
    const auto cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWindowWidth),
        static_cast<float>(iCursorY) / static_cast<float>(iWindowHeight));

    setPosition(cursorPos - 0.01F); // move slightly to be hovered
}

void SetNameMenu::onMouseLeft() {
    RectUiNode::onMouseLeft();

    if (!bIsDestroyHandled) {
        unsafeDetachFromParentAndDespawn(true);
    }
}

void SetNameMenu::onKeyboardInputWhileFocused(
    KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    RectUiNode::onKeyboardInputWhileFocused(button, modifiers, bIsPressedDown);

    if (!bIsPressedDown) {
        return;
    }

    if (button == KeyboardButton::ESCAPE) {
        bIsDestroyHandled = true;
        unsafeDetachFromParentAndDespawn(true);
    }
}
