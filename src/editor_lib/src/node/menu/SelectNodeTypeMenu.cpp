#include "node/menu/SelectNodeTypeMenu.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorColorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

// External.
#include "utf/utf.hpp"

SelectNodeTypeMenu::SelectNodeTypeMenu(const std::string& sNodeName, Node* pParent) : RectUiNode(sNodeName) {
    setIsReceivingInput(true); // for onMouseLeft to work
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorColorTheme::getPadding());
    setColor(EditorColorTheme::getEditorBackgroundColor());
    setSize(glm::vec2(0.15F, 0.4F));

    const auto pLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    pLayout->setPadding(EditorColorTheme::getPadding());
    pLayout->setChildNodeSpacing(EditorColorTheme::getSpacing());
    {
        const auto pSearchBackground = pLayout->addChildNode(std::make_unique<RectUiNode>());
        pSearchBackground->setColor(EditorColorTheme::getContainerBackgroundColor());
        {
            pSearchTextEdit = pSearchBackground->addChildNode(std::make_unique<TextEditUiNode>());
            pSearchTextEdit->setTextHeight(EditorColorTheme::getTextHeight());
            pSearchTextEdit->setText(u"");
            pSearchTextEdit->setHandleNewLineChars(false);
        }

        pTypesLayout = pLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pTypesLayout->setExpandPortionInLayout(8);
        pTypesLayout->setIsScrollBarEnabled(true);
        pTypesLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
        {
            if (dynamic_cast<UiNode*>(pParent) != nullptr) {
                addTypesForGuidRecursive(UiNode::getTypeGuidStatic(), pTypesLayout);
            } else {
                addTypesForGuidRecursive(Node::getTypeGuidStatic(), pTypesLayout);
            }
        }
    }

    // Rebuild list of types on search.
    pSearchTextEdit->setOnTextChanged([this](std::u16string_view sInputText) {
        const auto sText = utf::as_str8(sInputText);

        const auto mtxChildNodes = pTypesLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pChildNode : mtxChildNodes.second) {
            const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pChildNode->getTypeGuid());

            const auto pUiNode = reinterpret_cast<UiNode*>(pChildNode);
            pUiNode->setIsVisible(typeInfo.sTypeName.find(sText) != std::string::npos);
        }
    });
}

void SelectNodeTypeMenu::setOnTypeSelected(const std::function<void(std::string_view)>& onSelected) {
    onTypeSelected = onSelected;
}

void SelectNodeTypeMenu::onMouseLeft() {
    RectUiNode::onMouseLeft();

    unsafeDetachFromParentAndDespawn(true);
}

void SelectNodeTypeMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    setModal();
    // pSearchTextEdit->setFocused();
}

void SelectNodeTypeMenu::addTypesForGuidRecursive(
    const std::string& sTypeGuid, LayoutUiNode* pLayout, size_t iNesting) {
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

    const auto pButton = pLayout->addChildNode(std::make_unique<ButtonUiNode>(sTypeGuid));
    pButton->setOnClicked([this, pButton]() {
        if (onTypeSelected == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the \"type selected\" callback to be set");
        }
        onTypeSelected(pButton->getNodeName());
        unsafeDetachFromParentAndDespawn(true);
    });
    pButton->setSize(glm::vec2(pButton->getSize().x, EditorColorTheme::getButtonSizeY()));
    pButton->setPadding(EditorColorTheme::getPadding());
    pButton->setColor(EditorColorTheme::getButtonColor());
    pButton->setColorWhileHovered(EditorColorTheme::getButtonHoverColor());
    pButton->setColorWhilePressed(EditorColorTheme::getButtonPressedColor());
    {
        const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());

        // Prepare text.
        std::string sText;
        for (size_t i = 0; i < iNesting; i++) {
            sText += "    ";
        }
        sText += typeInfo.sTypeName;

        pText->setText(utf::as_u16(sText));
        pText->setTextHeight(EditorColorTheme::getTextHeight());
    }

    // Find all types that derive from this GUID.
    const auto& reflectedTypes = ReflectedTypeDatabase::getReflectedTypes();
    for (const auto& [sGuid, typeInfo] : reflectedTypes) {
        if (typeInfo.sParentTypeGuid != sTypeGuid) {
            continue;
        }
        addTypesForGuidRecursive(sGuid, pLayout, iNesting + 1);
    }
}
