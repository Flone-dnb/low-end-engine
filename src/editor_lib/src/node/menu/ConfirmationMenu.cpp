#include "node/menu/ConfirmationMenu.h"

// Custom.
#include "EditorTheme.h"
#include "game/camera/CameraManager.h"
#include "game/World.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/ButtonUiNode.h"

// External.
#include "utf/utf.hpp"

ConfirmationMenu::ConfirmationMenu(const std::string& sText, const std::function<void()>& onConfirmed)
    : RectUiNode("Confirmation Menu"), onConfirmed(onConfirmed) {
    setIsReceivingInput(true);
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorTheme::getPadding());
    setColor(EditorTheme::getContainerBackgroundColor());
    setSize(glm::vec2(0.1f, EditorTheme::getTextHeight() * 3.0f));
    setModal();

    const auto pLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    pLayout->setPadding(EditorTheme::getPadding());
    pLayout->setChildNodeSpacing(EditorTheme::getSpacing());
    {
        const auto pTitle = pLayout->addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorTheme::getTextHeight());
        pTitle->setText(utf::as_u16(sText));

        const auto pHorizontalLayout = pLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pHorizontalLayout->setIsHorizontal(true);
        pHorizontalLayout->setPadding(EditorTheme::getPadding());
        pHorizontalLayout->setChildNodeSpacing(EditorTheme::getSpacing() * 4.0f);
        pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        {
            const auto pNoButton = pHorizontalLayout->addChildNode(std::make_unique<ButtonUiNode>());
            pNoButton->setPadding(EditorTheme::getPadding());
            pNoButton->setColor(EditorTheme::getButtonColor());
            pNoButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
            pNoButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
            pNoButton->setOnClicked([this]() {
                bIsDestroyHandled = true;
                unsafeDetachFromParentAndDespawn(true);
            });
            {
                const auto pText = pNoButton->addChildNode(std::make_unique<TextUiNode>());
                pText->setTextHeight(EditorTheme::getTextHeight());
                pText->setText(u"no");
            }

            const auto pYesButton = pHorizontalLayout->addChildNode(std::make_unique<ButtonUiNode>());
            pYesButton->setPadding(EditorTheme::getPadding());
            pYesButton->setColor(EditorTheme::getButtonColor());
            pYesButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
            pYesButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
            {
                const auto pText = pYesButton->addChildNode(std::make_unique<TextUiNode>());
                pText->setTextHeight(EditorTheme::getTextHeight());
                pText->setText(u"yes");
            }
            pYesButton->setOnClicked([this]() {
                bIsDestroyHandled = true;
                if (!this->onConfirmed) [[unlikely]] {
                    Error::showErrorAndThrowException("expected the callback to be valid");
                }
                this->onConfirmed();
                unsafeDetachFromParentAndDespawn(true);
            });
        }
    }
}

void ConfirmationMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    // Get cursor position.
    const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        Error::showErrorAndThrowException("expected the cursor to be in the viewport");
    }
    const auto cursorPos = *optCursorPos;

    setPosition(cursorPos - 0.01f); // move slightly to be hovered
}

void ConfirmationMenu::onMouseLeft() {
    RectUiNode::onMouseLeft();

    if (!bIsDestroyHandled) {
        unsafeDetachFromParentAndDespawn(true);
    }
}
