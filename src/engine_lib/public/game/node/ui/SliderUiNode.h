#pragma once

// Standard.
#include <functional>

// Custom.
#include "game/node/ui/UiNode.h"

// External.
#include "math/GLMath.hpp"

/** It's a slider alright. */
class SliderUiNode : public UiNode {
    // Manager calls input functions.
    friend class UiNodeManager;

public:
    SliderUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SliderUiNode(const std::string& sNodeName);

    virtual ~SliderUiNode() override = default;

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
     * Sets color of the base of the slider.
     *
     * @param color RGBA color.
     */
    void setSliderColor(const glm::vec4& color);

    /**
     * Sets color of the handle for the slider.
     *
     * @param color RGBA color.
     */
    void setSliderHandleColor(const glm::vec4& color);

    /**
     * Sets position of the slider's handle.
     *
     * @param position Value in range [0.0; 1.0].
     * @param bTriggerOnChangedCallback Specify `false` if you don't want @ref setOnHandlePositionChanged to
     * be called.
     */
    void setHandlePosition(float position, bool bTriggerOnChangedCallback = true);

    /**
     * Sets the size of a single movement in handle position. 0 if can move freely.
     *
     * @param stepSize Step size.
     */
    void setSliderStep(float stepSize);

    /**
     * Sets a function that will be called after slider's handle position changed.
     *
     * @param onChanged Function to call.
     */
    void setOnHandlePositionChanged(const std::function<void(float handlePosition)>& onChanged);

    /**
     * Returns color of the base of the slider.
     *
     * @return RGBA color.
     */
    glm::vec4 getSliderColor() const { return sliderColor; }

    /**
     * Returns color of the handle for the slider.
     *
     * @return RGBA color.
     */
    glm::vec4 getSliderHandleColor() const { return sliderHandleColor; }

    /**
     * Returns position of the slider's handle.
     *
     * @return Value in range [0.0; 1.0].
     */
    float getHandlePosition() const { return handlePosition; }

    /**
     * Size of a single movement in handle position. 0 if can move freely.
     *
     * @return Step size.
     */
    float getSliderStep() const { return sliderStep; }

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
    virtual bool onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override;

    /**
     * Same as @ref onMouseButtonPressedOnUiNode but for mouse button released event.
     *
     * @param button    Mouse button.
     * @param modifiers Keyboard modifier keys.
     *
     * @return `true` if the event was handled.
     */
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override;

    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

    /**
     * Called when the window received mouse movement.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param xOffset  Mouse X movement delta in pixels (plus if moved to the right,
     * minus if moved to the left).
     * @param yOffset  Mouse Y movement delta in pixels (plus if moved up,
     * minus if moved down).
     */
    virtual void onMouseMove(double xOffset, double yOffset) override;

    /**
     * Called when the window received gamepad input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param button Gamepad button.
     */
    virtual void onGamepadButtonPressedWhileFocused(GamepadButton button) override;

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

private:
    /**
     * Snaps the value to be a multiple of the specified step (i.e. 0.27 with step 0.1 becomes 0.3).
     *
     * @param value Value to change.
     * @param step  Step size.
     *
     * @return Corrected value.
     */
    static float snapToNearest(float value, float step);

    /** RGBA color of the base of the slider. */
    glm::vec4 sliderColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);

    /** RGBA color of the handle for the slider. */
    glm::vec4 sliderHandleColor = glm::vec4(0.35f, 0.35f, 0.35f, 1.0f);

    /** Position of the slider's handle in range [0.0; 1.0]. */
    float handlePosition = 0.5f;

    /** Size of a single movement in handle position. 0 if can move freely. */
    float sliderStep = 0.05f;

    /** Called after slider's handle position changed. */
    std::function<void(float handlePosition)> onHandlePositionChanged;

    /** `true` if LMB was pressed on the handle and the mouse cursor is moving. */
    bool bIsMouseCursorDraggingHandle = false;
};
