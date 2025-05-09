#pragma once

// Custom.
#include "game/node/Node.h"
#include "render/UiLayer.hpp"
#include "input/MouseButton.hpp"

/** Base class for UI nodes. Provides functionality for positioning on the screen. */
class UiNode : public Node {
    // Manager calls input functions.
    friend class UiManager;

public:
    UiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    UiNode(const std::string& sNodeName);

    virtual ~UiNode() override = default;

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
     * Sets position on the screen in range [0.0; 1.0].
     *
     * @param position New position.
     */
    void setPosition(const glm::vec2& position);

    /**
     * Sets width and height in range [0.0; 1.0].
     *
     * @param size New size.
     */
    void setSize(const glm::vec2& size);

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     *
     * @param iPortion Positive value (can be bigger than 1 to fill more space relative to other nodes).
     */
    void setExpandPortionInLayout(unsigned int iPortion);

    /**
     * Sets if this node (and all child nodes) should be included in the rendering or not.
     *
     * @param bIsVisible New visibility.
     */
    void setIsVisible(bool bIsVisible);

    /**
     * Sets UI layer to use.
     *
     * @param layer UI layer.
     */
    void setUiLayer(UiLayer layer);

    /**
     * Returns position on the screen in range [0.0; 1.0].
     *
     * @return Position.
     */
    glm::vec2 getPosition() const { return position; }

    /**
     * Returns width and height in range [0.0; 1.0].
     *
     * @return Width and height.
     */
    glm::vec2 getSize() const { return size; }

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     *
     * @return Fill portion.
     */
    unsigned int getExpandPortionInLayout() const { return iExpandPortionInLayout; }

    /**
     * Tells if this node is included in the rendering or not.
     *
     * @return Visibility.
     */
    bool isVisible() const { return bIsVisible; }

    /**
     * Returns UI layer that this node uses.
     *
     * @return UI layer.
     */
    UiLayer getUiLayer() const { return layer; }

    /**
     * Returns how much nodes from the world's root node to skip to reach this node. Used to determine which
     * UI nodes should be in the front and which behind. Deepest nodes rendered last (in front).
     *
     * @warning If used while despawned an error will be shown.
     *
     * @return Node depth.
     */
    size_t getNodeDepthWhileSpawned();

protected:
    /**
     * Called when the window receives keyboard input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    virtual void
    onKeyboardInputOnUiNode(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown) {};

    /**
     * Called when the window receives mouse input while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if the user clicked on this UI node.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    virtual void onMouseClickOnUiNode(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    };

    /**
     * Called when the window receives mouse scroll movement while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iOffset Movement offset.
     */
    virtual void onMouseScrollMoveWhileHovered(int iOffset) {}

    /**
     * Called when the mouse cursor started floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseEntered() {}

    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() {}

    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() override;

    /**
     * Called after the node changed its "receiving input" state (while spawned).
     *
     * @param bEnabledNow `true` if node enabled input, `false` if disabled.
     */
    virtual void onChangedReceivingInputWhileSpawned(bool bEnabledNow) override;

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

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() {}

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() {}

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

private:
    /** Recalculates @ref iNodeDepth. Must be called only while spawned. */
    void recalculateNodeDepthWhileSpawned();

    /** Width and height in range [0.0; 1.0]. */
    glm::vec2 size = glm::vec2(0.2F, 0.2F); // NOLINT

    /** Position on the screen in range [0.0; 1.0]. */
    glm::vec2 position = glm::vec2(0.2F, 0.5F); // NOLINT

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     */
    unsigned int iExpandPortionInLayout = 1;

    /**
     * How much nodes from the world's root node to skip to reach this node. Used to determine which UI nodes
     * should be in the front and which behind. Deepest nodes rendered last (in front).
     *
     * @remark Only valid while spawned.
     */
    size_t iNodeDepth = 0;

    /** UI layer. */
    UiLayer layer = UiLayer::LAYER1;

    /** Rendered or not. */
    bool bIsVisible = true;

    /**
     * If receiving input is enabled, `true` if the mouse cursor is currently floating over this UI node.
     *
     * @remark UI manager modifies this value and calls @ref onMouseEntered @ref onMouseLeft if needed.
     */
    bool bIsHoveredCurrFrame = false;

    /**
     * @ref bIsHoveredCurrFrame on previous frame.
     *
     * @remark UI manager modifies this value and calls @ref onMouseEntered @ref onMouseLeft if needed.
     */
    bool bIsHoveredPrevFrame = false;
};
