#pragma once

// Custom.
#include "game/node/ui/LayoutUiNode.h"

/** Allows viewing and editing a node tree. */
class NodeTreeInspector : public LayoutUiNode {
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

private:
    /**
     * Adds a node to be displayed.
     *
     * @param pNode Node to display.
     */
    void addGameNodeRecursive(Node* pNode);
};
