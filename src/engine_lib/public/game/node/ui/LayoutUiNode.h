#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/UiNode.h"
#include "math/GLMath.hpp"

/** Defines how to "expand" child nodes across the layout. */
enum class ChildNodeExpandRule : uint8_t {
    DONT_EXPAND = 0,
    EXPAND_ALONG_MAIN_AXIS,      //< For vertical layout: child nodes height takes all space vertically, for
                                 // horizontal: child nodes width takes all space horizontally, everything
                                 // according to their "expand portion".
    EXPAND_ALONG_SECONDARY_AXIS, //< For vertical layout: child nodes width takes all space horizontally, for
                                 // horizontal: child nodes height takes all space vertically, everything
                                 // according to their "expand portion".
    EXPAND_ALONG_BOTH_AXIS       //< Nodes take full space of the layout according to their "expand portion".
};

/** Controls position and size of child nodes and arranges them in a horizontal or a vertical order. */
class LayoutUiNode : public UiNode {
    // Needs access to texture handle.
    friend class UiNodeManager;

    // When node's expand portion in layout is changed it notifies the parent layout.
    friend class UiNode;

    // If layout is a child node of a rect node the rect node will notify about changes in size.
    friend class RectUiNode;

public:
    LayoutUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    LayoutUiNode(const std::string& sNodeName);

    virtual ~LayoutUiNode() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

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
     * Sets layout rule for child nodes.
     *
     * @param bIsHorizontal `true` for horizontal, `false` for vertical.
     */
    void setIsHorizontal(bool bIsHorizontal);

    /**
     * Sets spacing between child nodes in range [0.0; 1.0] where 1.0 is full size of the layout.
     *
     * @param spacing Spacing.
     */
    void setChildNodeSpacing(float spacing);

    /**
     * Sets expand rule for child nodes.
     *
     * @param expandRule Expand rule to use.
     */
    void setChildNodeExpandRule(ChildNodeExpandRule expandRule);

    /**
     * Sets padding for child nodes in range [0.0; 0.5] where 1.0 is full size of the layout.
     *
     * @param padding Padding.
     */
    void setPadding(float padding);

    /**
     * Sets whether scroll bar is enabled or not.
     *
     * @param bEnable `true` to enable.
     */
    void setIsScrollBarEnabled(bool bEnable);

    /**
     * If @ref setIsScrollBarEnabled changes the current scroll offset.
     * Specify 0 to scroll to the top.
     *
     * @param iOffset New offset.
     */
    void setScrollBarOffset(size_t iOffset);

    /**
     * If scroll bar is enabled and a new item is added the layout will automatically scroll to bottom to make
     * sure the last item is visible.
     *
     * @param bEnable New state.
     */
    void setAutoScrollToBottom(bool bEnable);

    /**
     * Sets color of the scroll bar.
     *
     * @param color Color in the RGBA format.
     */
    void setScrollBarColor(const glm::vec4& color);

    /**
     * Returns layout rule for child nodes.
     *
     * @return `true` for horizontal, `false` for vertical.
     */
    bool getIsHorizontal() const { return bIsHorizontal; }

    /**
     * Returns spacing between child nodes in range [0.0; 1.0] where 1.0 is full size of the layout.
     *
     * @return Spacing.
     */
    float getChildNodeSpacing() const { return childNodeSpacing; }

    /**
     * Returns padding for child nodes in range [0.0; 0.5] where 1.0 is full size of the layout.
     *
     * @return Padding.
     */
    float getPadding() const { return padding; }

    /**
     * Returns expand rule used for child nodes.
     *
     * @return Expand rule.
     */
    ChildNodeExpandRule getChildNodeExpandRule() const { return childExpandRule; }

    /**
     * Tells if scroll bar is enabled.
     *
     * @return `true` if enabled.
     */
    bool getIsScrollBarEnabled() const { return bIsScrollBarEnabled; }

    /**
     * If scroll bar is enabled and a new item is added the layout will automatically scroll to bottom to make
     * sure the last item is visible.
     *
     * @return State.
     */
    bool getAutoScrollToBottom() const { return bAutoScrollToBottom; }

    /**
     * Returns color of the scroll bar.
     *
     * @return RGBA color.
     */
    glm::vec4 getScrollBarColor() const { return scrollBarColor; }

protected:
    /** Called after this object was finished deserializing from file. */
    virtual void onAfterDeserialized() override;

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

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
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

    /**
     * Called after some child node was detached from this node.
     *
     * @param pDetachedDirectChild Direct child node that was detached (child of this node, not a child of
     * some child node).
     */
    virtual void onAfterDirectChildDetached(Node* pDetachedDirectChild) override;

    /**
     * Called after this node or one of the node's parents (in the parent hierarchy)
     * was attached to a new parent node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    virtual void onAfterAttachedToNewParent(bool bThisNodeBeingAttached) override;

    /** Called after position of this UI node was changed. */
    virtual void onAfterPositionChanged() override;

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() override;

    /**
     * Called after a child node changed its position in the array of child nodes.
     *
     * @param iIndexFrom Moved from.
     * @param iIndexTo   Moved to.
     */
    virtual void onAfterChildNodePositionChanged(size_t iIndexFrom, size_t iIndexTo) override;

    /**
     * Called when the window receives mouse scroll movement while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iOffset Movement offset.
     *
     * @return `true` if the event was handled or `false` if the event needs to be passed to a parent UI node.
     */
    virtual bool onMouseScrollMoveWhileHovered(int iOffset) override;

private:
    /** Called by direct child nodes if the user changed their visibility. */
    void onDirectChildNodeVisibilityChanged();

    /** Recalculates position and size for direct child nodes. */
    void recalculatePosAndSizeForDirectChildNodes();

    /** First (most closer to this node) layout node in the parent chain. */
    std::pair<std::recursive_mutex, LayoutUiNode*> mtxLayoutParent;

    /** Color of the scroll bar. */
    glm::vec4 scrollBarColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);

    /** Offset of the scroll bar in lines of text. */
    size_t iCurrentScrollOffset = 0;

    /**
     * Total height of the layout including all child nodes, height is relative to the size of the layout
     * but can be bigger than 1.0.
     */
    float totalScrollHeight = 0.0f;

    /** Expand rule for child nodes. */
    ChildNodeExpandRule childExpandRule = ChildNodeExpandRule::DONT_EXPAND;

    /** Spacing between child nodes in range [0.0; 1.0] where 1.0 is full size of the layout. */
    float childNodeSpacing = 0.0f;

    /** Padding for child nodes in range [0.0; 0.5] where 1.0 is full size of the layout. */
    float padding = 0.0f;

    /** `false` for vertical layout, `true` for horizontal. */
    bool bIsHorizontal = false;

    /** Used to avoid recursion. */
    bool bIsCurrentlyUpdatingChildNodes = false;

    /** Scroll bar state. */
    bool bIsScrollBarEnabled = false;

    /** Automatically scroll to bottom if scroll bar is enabled and a new item is added. */
    bool bAutoScrollToBottom = false;

    /** Scroll step in range [0.0; 1.0] where 1.0 means full size of the layout. */
    static constexpr float scrollBarStepLocal = 0.05f;
};
