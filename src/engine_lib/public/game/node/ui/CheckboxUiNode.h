#pragma once

// Standard.
#include <functional>

// Custom.
#include "game/node/ui/UiNode.h"

// External.
#include "math/GLMath.hpp"

/** Button with 2 states: unchecked or checked. */
class CheckboxUiNode : public UiNode {
    // Has access to foreground render color.
    friend class UiNodeManager;

public:
    CheckboxUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    CheckboxUiNode(const std::string& sNodeName);

    virtual ~CheckboxUiNode() override = default;

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
     * Sets fill color when unchecked.
     *
     * @param color RGBA color.
     */
    void setBackgroundColor(const glm::vec4& color);

    /**
     * Sets bounds color and color of the element in the middle when checked.
     *
     * @param color RGBA color.
     */
    void setForegroundColor(const glm::vec4& color);

    /**
     * Sets checkbox state.
     *
     * @param bIsChecked New state.
     * @param bTriggerOnChangedCallback Specify `false` if you don't want @ref setOnStateChanged to
     * be called.
     */
    void setIsChecked(bool bIsChecked, bool bTriggerOnChangedCallback = true);

    /**
     * Sets a callback that will be called after the state of the checkbox (checked, unchecked) is changed.
     *
     * @param callback Callback.
     */
    void setOnStateChanged(const std::function<void(bool)>& callback);

    /**
     * Returns fill color when unchecked.
     *
     * @return RGBA color.
     */
    glm::vec4 getBackgroundColor() const { return backgroundColor; }

    /**
     * Returns bounds color and color of the element in the middle when checked.
     *
     * @return RGBA color.
     */
    glm::vec4 getForegroundColor() const { return foregroundColor; }

    /**
     * Returns `true` if checkbox is enabled.
     *
     * @return State.
     */
    bool isChecked() const { return bIsChecked; }

    /**
     * Returns the maximum number of child nodes this type allows. This is generally 0, 1, or +inf.
     *
     * @return Number of child nodes.
     */
    virtual size_t getMaxChildCount() const override { return 0; }

protected:
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
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

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
     *
     * @return `true` if the event was handled.
     */
    virtual bool
    onMouseClickOnUiNode(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) override;

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() override;

private:
    /** User specified callback to call when the state is changed. */
    std::function<void(bool)> onStateChanged;

    /** Fill color when unchecked. */
    glm::vec4 backgroundColor = glm::vec4(0.25F, 0.25F, 0.25F, 1.0F);

    /** Bounds color and color of the element in the middle when checked. */
    glm::vec4 foregroundColor = glm::vec4(0.5F, 0.5F, 0.5F, 1.0F);

    /** `true` if checkbox is enabeld. */
    bool bIsChecked = false;

    /** Used to avoid recursion. */
    bool bIsAdjustingSize = false;
};
