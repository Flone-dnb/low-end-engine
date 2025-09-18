#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"
#include "sound/SoundChannel.hpp"

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
     * Selects a node by a node ID (if such node exists).
     *
     * @param iNodeId Node ID of the node to select.
     */
    void selectNodeById(size_t iNodeId);

    /**
     * Updates displayed node name.
     *
     * @param pGameNode Node that changed its name.
     */
    void refreshGameNodeName(Node* pGameNode);

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

    /**
     * Shows a menu to change node name.
     *
     * @param pItem Item that holds the node to change the name.
     */
    void showChangeNodeNameMenu(NodeTreeInspectorItem* pItem);

    /**
     * Shows menu to change node's type.
     *
     * @param pItem Item that holds the node to change the type.
     */
    void showNodeTypeChangeMenu(NodeTreeInspectorItem* pItem);

    /**
     * Shows menu to add external node tree.
     *
     * @param pItem Item that holds the parent node.
     */
    void showAddExternalNodeTreeMenu(NodeTreeInspectorItem* pItem);

    /**
     * Moves a game node in the array of child nodes.
     *
     * @param pItem   Item that holds the parent node.
     * @param bMoveUp `true` to move node to the left (in the array of child nodes), up in the UI.
     */
    void moveGameNodeInChildArray(NodeTreeInspectorItem* pItem, bool bMoveUp);

    /**
     * Duplicates game node that is displayed by the specified node tree item.
     *
     * @param pItem Item that displays the node to duplicate.
     */
    void duplicateGameNode(NodeTreeInspectorItem* pItem);

    /**
     * Deletes game node that is displayed by the specified node tree item.
     *
     * @param pItem Item that displays the node to delete.
     */
    void deleteGameNode(NodeTreeInspectorItem* pItem);

    /**
     * Displays reflected fields in property inspector.
     *
     * @param pItem Item that displays the game node.
     */
    void inspectGameNode(NodeTreeInspectorItem* pItem);

    /**
     * Returns root node of game's world.
     *
     * @return Node.
     */
    Node* getGameRootNode() const { return pGameRootNode; }

    /**
     * Node that is currently being displayed in the inspector (if exists).
     *
     * @return Item.
     */
    NodeTreeInspectorItem* getInspectedItem() const { return pInspectedItem; }

    /**
     * Tells if the specified node is a root node of external node tree.
     *
     * @param pNode Node.
     *
     * @return `true` if root of external tree.
     */
    bool isNodeExternalTreeRootNode(Node* pNode);

    /** Removes display of the reflected fields of a game node (if were displayed). */
    void clearInspection();

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
     * @param pParent      Parent node to attach a new node.
     * @param sTypeGuid    GUID of the new node.
     * @param soundChannel If the node is a sound node specify a sound channel for it to use.
     */
    void addChildNodeToNodeTree(
        NodeTreeInspectorItem* pParent,
        const std::string& sTypeGuid,
        SoundChannel soundChannel = SoundChannel::COUNT);

    /**
     * Changes game node's type.
     *
     * @param pItem        Item that holds the node to change the type.
     * @param sTypeGuid    GUID of the new node.
     * @param soundChannel If the node is a sound node specify a sound channel for it to use.
     */
    void changeNodeType(
        NodeTreeInspectorItem* pItem,
        const std::string& sTypeGuid,
        SoundChannel soundChannel = SoundChannel::COUNT);

    /** Layout node. */
    LayoutUiNode* pLayoutNode = nullptr;

    /** Root node of game's world. */
    Node* pGameRootNode = nullptr;

    /** Node that is currently being displayed in the inspector. */
    NodeTreeInspectorItem* pInspectedItem = nullptr;
};
