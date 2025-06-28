#pragma once

// Custom.
#include "game/node/ui/ButtonUiNode.h"

class TextUiNode;

/** Displays a single node from a node tree inspector. */
class NodeTreeInspectorItem : public ButtonUiNode {
public:
    NodeTreeInspectorItem();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    NodeTreeInspectorItem(const std::string& sNodeName);

    virtual ~NodeTreeInspectorItem() override = default;

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
     * Initializes node display logic.
     *
     * @param pNode Node to display.
     */
    void setNodeToDisplay(Node* pNode);

private:
    /**
     * Tells how much parents the node has.
     *
     * @param pNode        Node to check parents of.
     * @param iParentCount Total number of parents in the parent chain.
     */
    static void getNodeParentCount(Node* pNode, size_t& iParentCount);

    /** Displayed text. */
    TextUiNode* pTextNode = nullptr;
};
