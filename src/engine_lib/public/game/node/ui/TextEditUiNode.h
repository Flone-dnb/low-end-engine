#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "math/GLMath.hpp"

/** Single-line or multi-line text editing. */
class TextEditUiNode : public TextUiNode {
    // Renderer uses cursor pos and selection.
    friend class UiNodeManager;

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
     * Sets a callback that will be triggered after the text is changed by user input.
     *
     * @param onTextChanged Callback to trigger.
     */
    void setOnTextChanged(const std::function<void(std::u16string_view)>& onTextChanged);

    /**
     * Sets a callback that will be triggered after the Enter key is pressed.
     *
     * @param onEnterPressed Callback.
     */
    void setOnEnterPressed(const std::function<void(std::u16string_view)>& onEnterPressed);

    /**
     * Sets color used for regions of selected text.
     *
     * @param textSelectionColor RGBA color.
     */
    void setTextSelectionColor(const glm::vec4& textSelectionColor);

    /**
     * Sets color of the scroll bar.
     *
     * @param color Color in the RGBA format.
     */
    void setScrollBarColor(const glm::vec4& color);

    /**
     * Sets whether scroll bar is enabled or not.
     *
     * @param bEnable `true` to enable.
     */
    void setIsScrollBarEnabled(bool bEnable);

    /**
     * If scroll is enabled moves scroll to make sure the character with the specified offset is visible.
     *
     * @param iTextCharOffset Offset in @ref getText.
     */
    void moveScrollToTextCharacter(size_t iTextCharOffset);

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

    /**
     * Returns color of the scroll bar.
     *
     * @return RGBA color.
     */
    glm::vec4 getScrollBarColor() const { return scrollBarColor; }

    /**
     * Tells if scroll bar is enabled.
     *
     * @return `true` if enabled.
     */
    bool getIsScrollBarEnabled() const { return bIsScrollBarEnabled; }

    /**
     * Returns current offset of the scroll bar in lines of text.
     *
     * @return Offset.
     */
    size_t getCurrentScrollOffset() const { return iCurrentScrollOffset; }

protected:
    /** Called after this object was finished deserializing from file. */
    virtual void onAfterDeserialized() override;

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
     * Called when the window receives keyboard input.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if this UI node has focus.
     *
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void
    onKeyboardButtonPressedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) override;

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

    /** Called after text is changed. */
    virtual void onAfterTextChanged() override;

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

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

private:
    /**
     * Converts mouse cursor position to offset (in characters) in the text.
     *
     * @return Offset in text in range [0; textSize].
     */
    size_t convertCursorPosToTextOffset();

    /** Called in cases when we should consider the current mouse position as text selection end. */
    void endTextSelection();

    /**
     * Must be used instead of parent's `setText`.
     *
     * @param sNewText Text to set.
     */
    void changeText(std::u16string_view sNewText);

    /** User specified callback to trigger when text is changed. */
    std::function<void(std::u16string_view)> onTextChanged;

    /** User specified callback to trigger after the Enter button is pressed. */
    std::function<void(std::u16string_view)> onEnterPressed;

    /** Empty if text edit is read only or not focused, otherwise value in range [0; textSize]. */
    std::optional<size_t> optionalCursorOffset;

    /** Empty if no text selected, otherwise pair of "start offset" and "count" in range [0; textSize]. */
    std::optional<std::pair<size_t, size_t>> optionalSelection;

    /** Color of the selected region of the text. */
    glm::vec4 textSelectionColor = glm::vec4(0.5F, 0.5F, 0.5F, 0.5F);

    /** Color of the scroll bar. */
    glm::vec4 scrollBarColor = glm::vec4(1.0F, 1.0F, 1.0F, 0.4F);

    /** Offset of the scroll bar in lines of text. */
    size_t iCurrentScrollOffset = 0;

    /** The total number of `\n` chars in @ref sText. */
    size_t iNewLineCharCountInText = 0;

    /** Scroll bar state. */
    bool bIsScrollBarEnabled = false;

    /** `true` if editing text from the user input is not possible. */
    bool bIsReadOnly = false;

    /** `true` if LMB was pressed on some text and was not released yet. */
    bool bIsTextSelectionStarted = false;

    /** `true` if inside @ref changeText. */
    bool bIsChangingText = false;

    /** `true` if we enabled text input for SDL. */
    bool bEnableSdlTextInput = false;
};
