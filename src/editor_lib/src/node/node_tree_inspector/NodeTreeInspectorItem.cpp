#include "NodeTreeInspectorItem.h"

// Custom.
#include "node/node_tree_inspector/NodeTreeInspector.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "EditorTheme.h"
#include "EditorGameInstance.h"

// External.
#include "utf/utf.hpp"

NodeTreeInspectorItem::NodeTreeInspectorItem(NodeTreeInspector* pInspector)
    : ButtonUiNode("Node Tree Inspector Item"), pInspector(pInspector) {
    setSize(glm::vec2(getSize().x, EditorTheme::getButtonSizeY()));
    setPadding(EditorTheme::getPadding());
    setColor(EditorTheme::getButtonColor());
    setColorWhileHovered(EditorTheme::getButtonHoverColor());
    setColorWhilePressed(EditorTheme::getButtonPressedColor());

    pTextNode = addChildNode(std::make_unique<TextUiNode>());
    pTextNode->setTextHeight(EditorTheme::getTextHeight());
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

    // Check if this is external tree root node.
    if (pInspector->isNodeExternalTreeRootNode(pNode)) {
        sText += " [ext tree]";
    }

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

bool NodeTreeInspectorItem::onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    ButtonUiNode::onMouseButtonReleasedOnUiNode(button, modifiers);

    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance to be valid");
    }

    if (button == MouseButton::LEFT) {
        pInspector->inspectGameNode(this);
        return true;
    }

    if (button == MouseButton::RIGHT) {
        if (pGameInstance->isContextMenuOpened()) {
            // Already opened.
            return true;
        }

        // Prepare context menu.
        std::vector<std::pair<std::u16string, std::function<void()>>> vOptions;
        vOptions.reserve(3);

        const auto pUiNode = dynamic_cast<UiNode*>(pGameNode);

        // Fill context menu options.
        {
            if ((pUiNode == nullptr || pUiNode->getMaxChildCount() > 0) &&
                !pInspector->isNodeExternalTreeRootNode(pGameNode)) {
                vOptions.push_back(
                    {u"Add child node", [this]() { pInspector->showChildNodeCreationMenu(this); }});
            }

            if (!pInspector->isNodeExternalTreeRootNode(pGameNode)) {
                vOptions.push_back({u"Change type", [this]() { pInspector->showNodeTypeChangeMenu(this); }});
            }

            if (!pInspector->isNodeExternalTreeRootNode(pGameNode)) {
                vOptions.push_back(
                    {u"Add external node tree", [this]() { pInspector->showAddExternalNodeTreeMenu(this); }});
            }

            if (pGameNode->getWorldRootNodeWhileSpawned() != pGameNode) {
                vOptions.push_back(
                    {u"Duplicate node (Ctrl + D)", [this]() { pInspector->duplicateGameNode(this); }});
            }

            if (pInspector->getGameRootNode() != pGameNode) {
                vOptions.push_back(
                    {u"Move up", [this]() { pInspector->moveGameNodeInChildArray(this, true); }});
                vOptions.push_back(
                    {u"Move down", [this]() { pInspector->moveGameNodeInChildArray(this, false); }});
            }

            if (pGameNode->getWorldRootNodeWhileSpawned() != pGameNode) {
                vOptions.push_back({u"Delete node", [this]() { pInspector->deleteGameNode(this); }});
            }
        }

        dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())->openContextMenu(vOptions);
    }

    return true;
}
