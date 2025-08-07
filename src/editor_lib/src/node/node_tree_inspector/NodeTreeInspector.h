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
     * Returns prefix to node names that won't be visible in the node tree inspector.
     *
     * @return Node name prefix.
     */
    static std::string_view getHiddenNodeNamePrefix() { return "Editor"; };

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
