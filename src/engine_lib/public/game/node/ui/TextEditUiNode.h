#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "math/GLMath.hpp"

/** Single-line or multi-line text editing. */
class TextEditUiNode : public TextUiNode {
    // Manager queries cursor pos.
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
     * Tells if editing text from the user input is not possible.
     *
     * @return Read only state.
     */
    bool getIsReadOnly() const { return bIsReadOnly; }

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

private:
    /** Empty if text edit is read only or not focused, otherwise value in range [0; textSize]. */
    std::optional<size_t> optionalCursorOffset;

    /** `true` if editing text from the user input is not possible. */
    bool bIsReadOnly = false;
};
