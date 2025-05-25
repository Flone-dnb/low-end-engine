#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "math/GLMath.hpp"

/** Single-line or multi-line text editing. */
class TextEditUiNode : public TextUiNode {
    // Renderer uses cursor pos and selection.
    friend class UiManager;

public:
    TextEditUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    TextEditUiNode(const std::string& sNodeName);

    virtual ~TextEditUiNode() override = default;

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
     * Sets if editing text from the user input is not possible.
     *
     * @param bIsReadOnly `true` to disable user input editing.
     */
    void setIsReadOnly(bool bIsReadOnly);

    /**
     * Sets color used for regions of selected text.
     *
     * @param textSelectionColor RGBA color.
     */
    void setTextSelectionColor(const glm::vec4& textSelectionColor);

    /**
     * Tells if editing text from the user input is not possible.
     *
     * @return Read only state.
     */
    bool getIsReadOnly() const { return bIsReadOnly; }

    /**
     * Returns color used for regions of selected text.
     *
     * @return RGBA color.
     */
    glm::vec4 getTextSelectionColor() const { return textSelectionColor; }

protected:
    /** Called after this object was finished deserializing from file. */
    virtual void onAfterDeserialized() override;

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
    virtual void
    onMouseClickOnUiNode(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) override;

    /**
     * Called when the window receives keyboard input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    virtual void onKeyboardInputWhileFocused(
        KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown) override;

    /**
     * Called by game manager when window received an event about text character being inputted.
     *
     * @param sTextCharacter Character that was typed.
     */
    virtual void onKeyboardInputTextCharacterWhileFocused(const std::string& sTextCharacter) override;

    /** Called after the node gained keyboard/gamepad focus. */
    virtual void onGainedFocus() override;

    /** Called after the node lost keyboard/gamepad focus. */
    virtual void onLostFocus() override;

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

private:
    /**
     * Converts the specified position on the screen to offset (in characters) in the text.
     *
     * @param screenPos Position on screen in range [0; iWindowWidth][0; iWindowHeight].
     *
     * @return Offset in text in range [0; textSize].
     */
    size_t convertScreenPosToTextOffset(const glm::vec2& screenPos);

    /** Called in cases when we should consider the current mouse position as text selection end. */
    void endTextSelection();

    /** Empty if text edit is read only or not focused, otherwise value in range [0; textSize]. */
    std::optional<size_t> optionalCursorOffset;

    /** Empty if no selected, otherwise pair of "start offset" and "end offset" in range [0; textSize]. */
    std::optional<std::pair<size_t, size_t>> optionalSelection;

    /** Color of the selected region of the text. */
    glm::vec4 textSelectionColor = glm::vec4(0.5F, 0.5F, 0.5F, 0.5F); // NOLINT

    /** `true` if editing text from the user input is not possible. */
    bool bIsReadOnly = false;

    /** `true` if LMB was pressed on some text and was not released yet. */
    bool bIsTextSelectionStarted = false;
};
