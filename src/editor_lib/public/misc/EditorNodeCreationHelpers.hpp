#pragma once

// Custom.
#include "game/node/Node.h"

/**
 * Creates an editor-owned node (not a game-owned node) with specific node settings like
 * no node serialization and etc. so that it won't affect the game.
 *
 * @return Created node.
 */
template <typename T>
    requires std::derived_from<T, Node>
static inline std::unique_ptr<T> createEditorNode() {
    // Create node.
    auto pCreatedNode = std::make_unique<T>();

    // Disable serialization so that it won't be serialized as part of the game world.
    pCreatedNode->setSerialize(false);

    return pCreatedNode;
}
