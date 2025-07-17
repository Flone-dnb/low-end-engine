#include "NodeTreeInspectorItem.h"

// Custom.
#include "node/node_tree_inspector/NodeTreeInspector.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "EditorTheme.h"
#include "EditorGameInstance.h"

// External.
#include "utf/utf.hpp"

NodeTreeInspectorItem::NodeTreeInspectorItem() : NodeTreeInspectorItem("Node Tree Inspector Item") {}
NodeTreeInspectorItem::NodeTreeInspectorItem(const std::string& sNodeName) : ButtonUiNode(sNodeName) {
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
        getParentNodeOfType<NodeTreeInspector>()->inspectGameNode(this);
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
            // Add child node.
            if (pUiNode == nullptr || pUiNode->getMaxChildCount() > 0) {
                vOptions.push_back({u"Add child node", [this]() {
                                        getParentNodeOfType<NodeTreeInspector>()->showChildNodeCreationMenu(
                                            this);
                                    }});
            }

            // Change name.
            vOptions.push_back({u"Change name", [this]() {
                                    getParentNodeOfType<NodeTreeInspector>()->showChangeNodeNameMenu(this);
                                }});

            // Change type.
            vOptions.push_back({u"Change type", [this]() {
                                    getParentNodeOfType<NodeTreeInspector>()->showNodeTypeChangeMenu(this);
                                }});

            // Delete node.
            if (pGameNode->getWorldRootNodeWhileSpawned() != pGameNode) {
                vOptions.push_back({u"Delete node", [this]() {
                                        getParentNodeOfType<NodeTreeInspector>()->deleteGameNode(this);
                                    }});
            }
        }

        dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())->openContextMenu(vOptions);
    }

    return true;
}
