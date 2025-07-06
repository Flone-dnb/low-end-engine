#pragma once

// Standard.
#include <string>
#include <functional>

// Custom.
#include "game/node/ui/RectUiNode.h"

class TextEditUiNode;

/**
 * Requires a text input from the user.
 * When spawned puts itself under the mouse cursor.
 * Automatically detaches and despawns when closed.
 */
class SetNameMenu : public RectUiNode {
public:
    SetNameMenu();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SetNameMenu(const std::string& sNodeName);

    virtual ~SetNameMenu() override = default;

    /**
     * Sets initial text to be displayed.
     *
     * @param sText Text to display.
     */
    void setInitialText(std::u16string_view sText);

    /**
     * Sets callbacks to call when operation is finished.
     *
     * @remark Automatically detaches itself and despawns after the operation is finished.
     *
     * @param onNameChanged Called after the name was changed.
     */
    void setOnNameChanged(const std::function<void(std::u16string)>& onNameChanged);

protected:
    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

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

private:
    /** Called after the name was changed. */
    std::function<void(std::u16string)> onNameChanged;

    /** Text input node. */
    TextEditUiNode* pTextEditNode = nullptr;

    /** `true` if detach and despawn is already handled. */
    bool bIsDestroyHandled = false;
};
