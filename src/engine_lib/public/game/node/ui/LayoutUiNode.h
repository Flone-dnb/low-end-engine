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
    friend class UiManager;

    // When node's expand portion in layout is changed it notifies the parent layout.
    friend class UiNode;

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
     * Sets padding for elements in range [0.0; 0.5] where 1.0 is full size of the layout.
     *
     * @param padding Padding.
     */
    void setPadding(float padding);

    /**
     * Returns layout rule for child nodes.
     *
     * @return `true` for horizontal, `false` for vertical.
     */
    bool isHorizontal() const { return bIsHorizontal; }

    /**
     * Returns spacing between child nodes in range [0.0; 1.0] where 1.0 is full size of the layout.
     *
     * @return Spacing.
     */
    float getChildNodeSpacing() const { return childNodeSpacing; }

    /**
     * Returns padding for elements in range [0.0; 0.5] where 1.0 is full size of the layout.
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

protected:
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

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() override;

    /**
     * Called after a child node changed its position in the array of child nodes.
     *
     * @param iIndexFrom Moved from.
     * @param iIndexTo   Moved to.
     */
    virtual void onAfterChildNodePositionChanged(size_t iIndexFrom, size_t iIndexTo) override;

private:
    /**
     * Recalculates position and size for direct child nodes.
     *
     * @param bNotifyParentLayout If `true` and this node's parent is a layout node it will be notified
     * about our size being changed.
     */
    void recalculatePosAndSizeForDirectChildNodes(bool bNotifyParentLayout);

    /** Expand rule for child nodes. */
    ChildNodeExpandRule childExpandRule = ChildNodeExpandRule::DONT_EXPAND;

    /** Spacing between child nodes in range [0.0; 1.0] where 1.0 is full size of the layout. */
    float childNodeSpacing = 0.0F;

    /** Padding for elements in range [0.0; 0.5] where 1.0 is full size of the layout. */
    float padding = 0.0F;

    /** `false` for vertical layout, `true` for horizontal. */
    bool bIsHorizontal = false;

    /** Used to avoid recursion. */
    bool bIsCurrentlyUpdatingChildNodes = false;
};
