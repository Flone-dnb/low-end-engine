#pragma once

// Standard.
#include <vector>
#include <functional>
#include <string>

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;
class TextEditUiNode;

/** Customizable context menu. */
class SelectNodeTypeMenu : public RectUiNode {
public:
    SelectNodeTypeMenu() = delete;

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     * @param pParent   Parent node that will have new child node.
     */
    SelectNodeTypeMenu(const std::string& sNodeName, Node* pParent);

    virtual ~SelectNodeTypeMenu() override = default;

    /**
     * Called after the type was selected with type's GUID as the only argument.
     *
     * @remark Automatically detaches and despawns itself after the callback is finished or if the operation
     * is canceled.
     *
     * @param onSelected Callback.
     */
    void setOnTypeSelected(const std::function<void(const std::string&)>& onSelected);

private:
    /**
     * Called after @ref onSpawning when this node and all of node's child nodes (at the moment
     * of spawning) were spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark Generally you might want to prefer to use @ref onSpawning, this function
     * is mostly used to do some logic related to child nodes after all child nodes were spawned
     * (for example if you have a camera child node you can make it active in this function).=
     */
    virtual void onChildNodesSpawned() override;

    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

private:
    /**
     * Adds node type with the specified GUID then adds all node types that derive from this type and etc.
     *
     * @param sTypeGuid GUID of the type to add.
     * @param pLayout   Layout to add new nodes to.
     * @param iNesting  Recursion nesting.
     */
    void addTypesForGuidRecursive(const std::string& sTypeGuid, LayoutUiNode* pLayout, size_t iNesting = 0);

    /** Filter fot types. */
    TextEditUiNode* pSearchTextEdit = nullptr;

    /** Layout that displays available types to select. */
    LayoutUiNode* pTypesLayout = nullptr;

    /** Callback to trigger with type's GUID once the type is selected. */
    std::function<void(const std::string&)> onTypeSelected;

    /** `true` if an option was clicked and we are currently processing it. */
    bool bIsProcessingButtonClick = false;
};
