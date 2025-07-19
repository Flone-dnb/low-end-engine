#pragma once

// Standard.
#include <string>
#include <functional>

// Custom.
#include "game/node/ui/RectUiNode.h"

class TextEditUiNode;

/**
 * Asks "yes" or "no" to confirm an operation.
 * When spawned puts itself under the mouse cursor.
 * Automatically detaches and despawns when closed.
 */
class ConfirmationMenu : public RectUiNode {
public:
    ConfirmationMenu() = delete;

    /**
     * Creates a new node with the specified name.
     *
     * @param sText       Text to display.
     * @param onConfirmed Callback that will be called after the user confirmed the operation.
     */
    ConfirmationMenu(const std::string& sText, const std::function<void()>& onConfirmed);

    virtual ~ConfirmationMenu() override = default;

protected:
    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

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
    /** Called after the operation was confirmed. */
    const std::function<void()> onConfirmed;

    /** `true` if detach and despawn is already handled. */
    bool bIsDestroyHandled = false;
};
