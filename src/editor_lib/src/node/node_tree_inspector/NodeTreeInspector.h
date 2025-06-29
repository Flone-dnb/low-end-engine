#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;
class ButtonUiNode;
class NodeTreeInspectorItem;

/** Allows viewing and editing a node tree. */
class NodeTreeInspector : public RectUiNode {
public:
    NodeTreeInspector();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    NodeTreeInspector(const std::string& sNodeName);

    virtual ~NodeTreeInspector() override = default;

    /**
     * Returns prefix to node names that won't be visible in the node tree inspector.
     *
     * @return Node name prefix.
     */
    static std::string_view getHiddenNodeNamePrefix() { return "Editor"; };

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Called after a game's node tree was loaded.
     *
     * @param pGameRootNode Root node of game's node tree.
     */
    void onGameNodeTreeLoaded(Node* pGameRootNode);

    /**
     * Shows a menu to create a new child node to attach to the displayed node tree.
     *
     * @param pParent Item that should have a new child.
     */
    void showChildNodeCreationMenu(NodeTreeInspectorItem* pParent);

private:
    /**
     * Adds a node to be displayed.
     *
     * @param pNode Node to display.
     */
    void addGameNodeRecursive(Node* pNode);

    /**
     * Adds a new child node to the displayed node tree and a new inspector item to be displayed.
     *
     * @param pParent   Parent node to attach a new node.
     * @param sTypeGuid GUID of the new node.
     */
    void addChildNodeToNodeTree(NodeTreeInspectorItem* pParent, const std::string& sTypeGuid);

    /** Layout node. */
    LayoutUiNode* pLayoutNode = nullptr;

    /** Root node of game's world. */
    Node* pGameRootNode = nullptr;
};
