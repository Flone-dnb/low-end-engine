#include "node/menu/SetNameMenu.h"

// Custom.
#include "EditorColorTheme.h"
#include "game/GameInstance.h"
#include "game/camera/CameraManager.h"
#include "game/World.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextEditUiNode.h"

SetNameMenu::SetNameMenu() : SetNameMenu("Set Name Menu Node") {}
SetNameMenu::SetNameMenu(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setIsReceivingInput(true);
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorColorTheme::getPadding());
    setColor(EditorColorTheme::getContainerBackgroundColor());
    setSize(glm::vec2(0.15F, EditorColorTheme::getTextHeight() * 3.0F));
    setModal();

    const auto pLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pLayout->setPadding(EditorColorTheme::getPadding());
    pLayout->setChildNodeSpacing(EditorColorTheme::getSpacing());
    {
        const auto pTitle = pLayout->addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorColorTheme::getTextHeight());
        pTitle->setText(u"New name:");

        const auto pTextEditBackground = pLayout->addChildNode(std::make_unique<RectUiNode>());
        pTextEditBackground->setColor(EditorColorTheme::getButtonColor());
        pTextEditBackground->setSize(glm::vec2(1.0F, EditorColorTheme::getTextHeight() * 1.1F));
        {
            pTextEditNode = pTextEditBackground->addChildNode(std::make_unique<TextEditUiNode>());
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
    }
}

void SetNameMenu::setOnNameChanged(const std::function<void(std::u16string)>& onNameChanged) {
    this->onNameChanged = onNameChanged;
}

void SetNameMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    // Get cursor position.
    const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        Error::showErrorAndThrowException("expected the cursor to be in the viewport");
    }
    const auto cursorPos = *optCursorPos;

    setPosition(cursorPos - 0.01F); // move slightly to be hovered

    pTextEditNode->setFocused();
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
