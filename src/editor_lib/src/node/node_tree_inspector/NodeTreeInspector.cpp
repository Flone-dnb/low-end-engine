#include "NodeTreeInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "node/node_tree_inspector/NodeTreeInspectorItem.h"
#include "EditorColorTheme.h"

namespace {
    constexpr std::string_view sTypeGuid = "9f6c749e-1b8e-4c53-b870-8cdf7730ec65";
}

std::string NodeTreeInspector::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string NodeTreeInspector::getTypeGuid() const { return sTypeGuid.data(); }

NodeTreeInspector::NodeTreeInspector() : NodeTreeInspector("Node Tree Inspector") {}
NodeTreeInspector::NodeTreeInspector(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(glm::vec4(0.0F));
    setPadding(EditorColorTheme::getPadding() / 2.0F);

    const auto pBackground = addChildNode(std::make_unique<RectUiNode>());
    pBackground->setColor(EditorColorTheme::getContainerBackgroundColor());

    pLayoutNode = pBackground->addChildNode(std::make_unique<LayoutUiNode>("Node Tree Inspector Layout"));
    pLayoutNode->setIsScrollBarEnabled(true);
    pLayoutNode->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pLayoutNode->setPadding(EditorColorTheme::getPadding());
    pLayoutNode->setChildNodeSpacing(EditorColorTheme::getSpacing());
}

void NodeTreeInspector::onGameNodeTreeLoaded(Node* pGameRootNode) { addGameNodeRecursive(pGameRootNode); }

void NodeTreeInspector::addGameNodeRecursive(Node* pNode) {
    if (pNode->getNodeName().starts_with(getHiddenNodeNamePrefix())) {
        return;
    }

    const auto pItem = pLayoutNode->addChildNode(std::make_unique<NodeTreeInspectorItem>());
    pItem->setNodeToDisplay(pNode);

    const auto mtxChildNodes = pNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    for (const auto& pChildNode : mtxChildNodes.second) {
        addGameNodeRecursive(pChildNode);
    }
}
