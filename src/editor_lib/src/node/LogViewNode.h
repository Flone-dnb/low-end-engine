#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"
#include "io/Logger.h"

class LayoutUiNode;

/** Displays logger messages. */
class LogViewNode : public RectUiNode {
public:
    LogViewNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    LogViewNode(const std::string& sNodeName);

    virtual ~LogViewNode() override = default;

protected:
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

private:
    /** Node that has all log messages. */
    LayoutUiNode* pLayout = nullptr;

    /** Callback to receive logger messages. */
    std::unique_ptr<LoggerCallbackGuard> pLoggerCallback;
};
