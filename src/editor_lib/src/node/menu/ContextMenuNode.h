#pragma once

// Standard.
#include <vector>
#include <functional>
#include <string>

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;
class TextUiNode;

/** Customizable context menu. */
class ContextMenuNode : public RectUiNode {
public:
    ContextMenuNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    ContextMenuNode(const std::string& sNodeName);

    virtual ~ContextMenuNode() override = default;

    /**
     * Shows context menu at the current position of the mouse cursor.
     *
     * @remark Menu will be automatically closed when a menu item is clicked or if mouse is no longer hovering
     * over the context menu.
     *
     * @param vMenuItems Names and callbacks for menu items.
     * @param sTitle     Optional title of the menu to show.
     */
    void openMenu(
        const std::vector<std::pair<std::u16string, std::function<void()>>>& vMenuItems,
        const std::u16string& sTitle = u"");

private:
    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

    /** Hides the menu. */
    void closeMenu();

    /** Layout to add context menu buttons to. */
    LayoutUiNode* pButtonsLayout = nullptr;

    /** `true` if an option was clicked and we are currently processing it. */
    bool bIsProcessingButtonClick = false;
};
