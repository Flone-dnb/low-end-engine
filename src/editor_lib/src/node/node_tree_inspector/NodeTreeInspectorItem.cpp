#include "NodeTreeInspectorItem.h"

// Custom.
#include "node/node_tree_inspector/NodeTreeInspector.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "EditorColorTheme.h"
#include "EditorGameInstance.h"

// External.
#include "utf/utf.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "7cf78258-31f9-489e-b1ea-8e048b002edc";
}

std::string NodeTreeInspectorItem::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string NodeTreeInspectorItem::getTypeGuid() const { return sTypeGuid.data(); }

NodeTreeInspectorItem::NodeTreeInspectorItem() : NodeTreeInspectorItem("Node Tree Inspector Item") {}
NodeTreeInspectorItem::NodeTreeInspectorItem(const std::string& sNodeName) : ButtonUiNode(sNodeName) {
    setSize(glm::vec2(getSize().x, EditorColorTheme::getButtonSizeY()));
    setPadding(EditorColorTheme::getPadding());
    setColor(EditorColorTheme::getButtonColor());
    setColorWhileHovered(EditorColorTheme::getButtonHoverColor());
    setColorWhilePressed(EditorColorTheme::getButtonPressedColor());

    pTextNode = addChildNode(std::make_unique<TextUiNode>());
    pTextNode->setTextHeight(EditorColorTheme::getTextHeight());
}

void NodeTreeInspectorItem::setNodeToDisplay(Node* pNode) {
    pGameNode = pNode;

    size_t iNesting = 0;
    getNodeParentCount(pNode, iNesting);

    // Prepare node name.
    std::string sText;
    for (size_t i = 0; i < iNesting; i++) {
        sText += "    ";
    }
    sText += pNode->getNodeName();

    pTextNode->setText(utf::as_u16(sText));
}

void NodeTreeInspectorItem::getNodeParentCount(Node* pNode, size_t& iParentCount) {
    const auto mtxParent = pNode->getParentNode();

    std::scoped_lock guard(*mtxParent.first);

    if (mtxParent.second == nullptr) {
        return;
    }

    iParentCount += 1;

    getNodeParentCount(mtxParent.second, iParentCount);
}

void NodeTreeInspectorItem::onMouseClickOnUiNode(
    MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    ButtonUiNode::onMouseClickOnUiNode(button, modifiers, bIsPressedDown);

    if (button == MouseButton::RIGHT) {
        const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
        if (pGameInstance == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected editor game instance to be valid");
        }

        pGameInstance->openContextMenu(
            {{u"Add child node",
              [this]() { getParentNodeOfType<NodeTreeInspector>()->showChildNodeCreationMenu(this); }},
             {u"Change name",
              []() {
                  // TODO: show name change menu
              }},
             {u"Change type", []() {
                  // TODO: show type change menu
              }}});
    }
}
