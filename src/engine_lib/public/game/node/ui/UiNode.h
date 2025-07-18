#pragma once

// Custom.
#include "game/node/Node.h"
#include "render/UiLayer.hpp"
#include "input/MouseButton.hpp"
#include "input/GamepadButton.hpp"

/** Base class for UI nodes. Provides functionality for positioning on the screen. */
class UiNode : public Node {
    // Manager calls input functions.
    friend class UiNodeManager;

    // Disables rendering of the node if the node is outside of the visible area of layout.
    friend class LayoutUiNode;

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
     * Generally used when the node is a child node of some container node (for example: layout node), if
     * `true` then even if the node is invisible it will still take space in the container (there would be an
     * empty space with the size of the node).
     *
     * @param bTakeSpace `true` to enable.
     */
    void setOccupiesSpaceEvenIfInvisible(bool bTakeSpace);

    /**
     * Sets UI layer to use.
     *
     * @warning If used while spawned an error will be shown.
     *
     * @remark Also changes UI layer for all child nodes.
     *
     * @param layer UI layer.
     */
    void setUiLayer(UiLayer layer);

    /**
     * Makes this node and its child nodes a modal UI node that takes all input.
     *
     * @remark Replaces old modal node (tree).
     * @remark Automatically becomes non-modal when a node gets despawned, becomes invisible or disables
     * input.
     */
    void setModal();

    /**
     * Sets node that will have focus to receive keyboard/gamepad input.
     *
     * @warning If used while not spawned or invisible an error will be shown.
     */
    void setFocused();

    /**
     * Returns position of the top-left corner of the UI node in range [0.0; 1.0] relative to screen size.
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
     * Returns the current state of @ref setOccupiesSpaceEvenIfInvisible.
     *
     * @return State.
     */
    bool getOccupiesSpaceEvenIfInvisible() const { return bOccupiesSpaceEvenIfInvisible; }

    /**
     * Returns how much nodes from the world's root node to skip to reach this node. Used to determine which
     * UI nodes should be in the front and which behind. Deepest nodes rendered last (in front).
     *
     * @warning If used while despawned an error will be shown.
     *
     * @return Node depth.
     */
    size_t getNodeDepthWhileSpawned();

    /**
     * Returns the maximum number of child nodes this type allows. This is generally 0, 1, or +inf.
     *
     * @return Number of child nodes.
     */
    virtual size_t getMaxChildCount() const;

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     */
    virtual void onSpawning() override;

    /**
     * Called when the window receives keyboard input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void onKeyboardButtonPressedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) {}

    /**
     * Same as @ref onKeyboardButtonPressedWhileFocused but called when button is released.
     *
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void onKeyboardButtonReleasedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) {}

    /**
     * Called when the window received gamepad input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param button Gamepad button.
     */
    virtual void onGamepadButtonPressedWhileFocused(GamepadButton button) {}

    /**
     * Same as @ref onGamepadButtonPressedWhileFocused but called when button is released.
     *
     * @param button Gamepad button.
     */
    virtual void onGamepadButtonReleasedWhileFocused(GamepadButton button) {}

    /**
     * Called by game manager when window received an event about text character being inputted.
     *
     * @param sTextCharacter Character that was typed.
     */
    virtual void onKeyboardInputTextCharacterWhileFocused(const std::string& sTextCharacter) {}

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
    virtual bool onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
        return false;
    }

    /**
     * Same as @ref onMouseButtonPressedOnUiNode but for mouse button released event.
     *
     * @param button    Mouse button.
     * @param modifiers Keyboard modifier keys.
     *
     * @return `true` if the event was handled.
     */
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
        return false;
    }

    /**
     * Called when the window receives mouse scroll movement while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iOffset Movement offset.
     *
     * @return `true` if the event was handled or `false` if the event needs to be passed to a parent UI node.
     * Base class implementation passes the call to the parent node.
     */
    virtual bool onMouseScrollMoveWhileHovered(int iOffset);

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

    /** Called after the node gained keyboard/gamepad focus. */
    virtual void onGainedFocus() {}

    /** Called after the node lost keyboard/gamepad focus. */
    virtual void onLostFocus() {}

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

    /** Called after position of this UI node was changed. */
    virtual void onAfterPositionChanged() {}

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() {}

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

    /**
     * Tells if the UI node is allowed to be rendered or not (has higher priority over visibility).
     *
     * @return `false` if should not be rendered.
     */
    bool isRenderingAllowed() const { return bAllowRendering; }

private:
    /** Recalculates @ref iNodeDepth. Must be called only while spawned. */
    void recalculateNodeDepthWhileSpawned();

    /**
     * Used internally to change @ref bAllowRendering and affect child nodes and trigger nessessary
     * callbacks.
     *
     * @param bAllowRendering `true` to allow rendering.
     */
    void setAllowRendering(bool bAllowRendering);

    /** Called after @ref bIsVisible or @ref bAllowRendering is changed. */
    void processVisibilityChange();

    /** Width and height in range [0.0; 1.0]. */
    glm::vec2 size = glm::vec2(0.2F, 0.2F);

    /** Position on the screen in range [0.0; 1.0]. */
    glm::vec2 position = glm::vec2(0.2F, 0.5F);

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

    /** Setting that allows the user to enable/disable rendering of this node. Affects all child nodes. */
    bool bIsVisible = true;

    /** `true` if this UI node (and children) should be modal (top priority for input over other nodes). */
    bool bShouldBeModal = false;

    /**
     * Has more priority over @ref bIsVisible. Affects all child nodes. Used internally by container nodes
     * that operate on whether a specific child node should be rendered or not (for example the layout node
     * might disable rendering if the node is outside of the visible area).
     *
     * @remark Use @ref setAllowRendering to change this variable.
     */
    bool bAllowRendering = true;

    /**
     * Generally used when the node is a child node of some container node, if `true` then even if the node
     * is invisible it will still take space in the container (there would be an empty space with the size of
     * the node).
     */
    bool bOccupiesSpaceEvenIfInvisible = false;

    /**
     * If receiving input is enabled, `true` if the mouse cursor is currently floating over this UI node.
     *
     * @remark UI manager modifies this value and calls @ref onMouseEntered @ref onMouseLeft if needed.
     */
    bool bIsMouseCursorHovered = false;
};
