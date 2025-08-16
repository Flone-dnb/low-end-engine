#pragma once

// Custom.
#include "game/node/ui/ButtonUiNode.h"

class TextUiNode;
class NodeTreeInspector;

/** Displays a single node from a node tree inspector. */
class NodeTreeInspectorItem : public ButtonUiNode {
    // Inspector manages data.
    friend class NodeTreeInspector;

public:
    NodeTreeInspectorItem() = delete;

    /**
     * Creates a new node.
     *
     * @param pInspector Inspector.
     */
    NodeTreeInspectorItem(NodeTreeInspector* pInspector);

    virtual ~NodeTreeInspectorItem() override = default;

    /**
     * Initializes node display logic.
     *
     * @param pNode Node to display.
     */
    void setNodeToDisplay(Node* pNode);

    /**
     * Returns game node that this item is displaying.
     *
     * @return Game node.
     */
    Node* getDisplayedGameNode() const { return pGameNode; }

protected:
    /**
     * Called when the window receives mouse button press event while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if the user clicked on this UI node.
     *
     * @param button    Mouse button.
     * @param modifiers Keyboard modifier keys.
     *
     * @return `true` if the event was handled.
     */
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override;

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

    /** Game node this item represents. */
    Node* pGameNode = nullptr;

    /** Inspector that created this node. */
    NodeTreeInspector* const pInspector = nullptr;
};
