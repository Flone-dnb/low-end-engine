#include "node/menu/SelectNodeTypeMenu.h"

// Standard.
#include <algorithm>

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

// External.
#include "utf/utf.hpp"

SelectNodeTypeMenu::SelectNodeTypeMenu(const std::string& sNodeName, Node* pParent) : RectUiNode(sNodeName) {
    setIsReceivingInput(true); // for onMouseLeft to work
    setUiLayer(UiLayer::LAYER2);
    setPadding(EditorTheme::getPadding());
    setColor(EditorTheme::getEditorBackgroundColor());
    setSize(glm::vec2(0.15f, 0.4f));

    const auto pLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    pLayout->setPadding(EditorTheme::getPadding());
    pLayout->setChildNodeSpacing(EditorTheme::getSpacing() * 2.0f);
    {
        const auto pSearchBackground = pLayout->addChildNode(std::make_unique<RectUiNode>());
        pSearchBackground->setPadding(EditorTheme::getPadding() * 2.0f);
        pSearchBackground->setColor(EditorTheme::getContainerBackgroundColor());
        {
            pSearchTextEdit = pSearchBackground->addChildNode(std::make_unique<TextEditUiNode>());
            pSearchTextEdit->setTextHeight(EditorTheme::getTextHeight());
            pSearchTextEdit->setText(u"");
            pSearchTextEdit->setHandleNewLineChars(false);
        }

        pTypesLayout = pLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pTypesLayout->setExpandPortionInLayout(12);
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
        pTypesLayout->setScrollBarOffset(0);

        // Input to lower case for searching.
        auto sText = utf::as_str8(sInputText);
        std::transform(
            sText.begin(), sText.end(), sText.begin(), [](unsigned char c) { return std::tolower(c); });

        const auto mtxChildNodes = pTypesLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pChildNode : mtxChildNodes.second) {
            const auto pUiNode = reinterpret_cast<UiNode*>(pChildNode);

            // Get type info.
            const auto sTypeGuid =
                std::string(pChildNode->getNodeName()); // we store GUIDs in node names here
            const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

            // Type name to lower case.
            auto sTypeName = typeInfo.sTypeName;
            std::transform(sTypeName.begin(), sTypeName.end(), sTypeName.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

            pUiNode->setIsVisible(sTypeName.find(sText) != std::string::npos);
        }
    });
    pSearchTextEdit->setOnEnterPressed([this](std::u16string_view sText) {
        // If only 1 option (node type) is visible - select it.

        auto mtxChildNodes = pTypesLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        UiNode* pOnlyVisibleUiNode = nullptr;
        for (const auto& pNode : mtxChildNodes.second) {
            const auto pUiNode = dynamic_cast<UiNode*>(pNode);
            if (pUiNode == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected a UI node");
            }
            if (pUiNode->isVisible()) {
                if (pOnlyVisibleUiNode != nullptr) {
                    return;
                }
                pOnlyVisibleUiNode = pUiNode;
            }
        }

        if (onTypeSelected == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the \"type selected\" callback to be set");
        }

        const auto pButton = dynamic_cast<ButtonUiNode*>(pOnlyVisibleUiNode);
        if (pButton == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected types layout to have buttons");
        }

        bIsProcessingButtonClick = true;
        onTypeSelected(std::string(pButton->getNodeName()));
        unsafeDetachFromParentAndDespawn(true);
    });
}

void SelectNodeTypeMenu::setOnTypeSelected(const std::function<void(const std::string&)>& onSelected) {
    onTypeSelected = onSelected;
}

void SelectNodeTypeMenu::onMouseLeft() {
    RectUiNode::onMouseLeft();

    if (!bIsProcessingButtonClick) {
        unsafeDetachFromParentAndDespawn(true);
    }
}

void SelectNodeTypeMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    setModal();
    pSearchTextEdit->setFocused();
}

void SelectNodeTypeMenu::addTypesForGuidRecursive(
    const std::string& sTypeGuid, LayoutUiNode* pLayout, size_t iNesting) {
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

    const auto pButton = pLayout->addChildNode(std::make_unique<ButtonUiNode>(sTypeGuid));
    pButton->setOnClicked([this, pButton]() {
        if (onTypeSelected == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the \"type selected\" callback to be set");
        }
        bIsProcessingButtonClick = true;
        onTypeSelected(std::string(pButton->getNodeName()));
        unsafeDetachFromParentAndDespawn(true);
    });
    pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
    pButton->setPadding(EditorTheme::getPadding());
    pButton->setColor(EditorTheme::getButtonColor());
    pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
    pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
    {
        const auto pText = pButton->addChildNode(
            std::make_unique<TextUiNode>(std::format("select type option \"{}\"", typeInfo.sTypeName)));

        // Prepare text.
        std::string sText;
        for (size_t i = 0; i < iNesting; i++) {
            sText += "    ";
        }
        sText += typeInfo.sTypeName;

        pText->setText(utf::as_u16(sText));
        pText->setTextHeight(EditorTheme::getTextHeight());
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
