#include "game/World.h"

// Custom.
#include "game/node/Node.h"
#include "io/Logger.h"
#include "misc/Error.h"

World::~World() {
    std::scoped_lock gaurd(mtxRootNode.first);

    if (mtxRootNode.second != nullptr) {
        destroyWorld();
    }
}

World::World(GameManager* pGameManager) : pGameManager(pGameManager) {
    Logger::get().info(std::format("new world is created"));
    Logger::get().flushToDisk();

    mtxRootNode.second = std::make_unique<Node>("Root Node");
}

void World::destroyWorld() {
    Logger::get().info("world is being destroyed, despawning world's root node...");
    Logger::get().flushToDisk();

    std::scoped_lock guard(mtxRootNode.first);
    mtxRootNode.second->despawn();
    mtxRootNode.second = nullptr;
}

void World::tickTickableNodes(float timeSincePrevCallInSec) {
    executeTasksAfterNodeTick();

    const auto callTickOnGroup = [&](std::unordered_set<Node*>* pTickGroup) {
        for (auto it = pTickGroup->begin(); it != pTickGroup->end(); ++it) {
            (*it)->onBeforeNewFrame(timeSincePrevCallInSec);
        }
    };

    {
        std::scoped_lock guard(mtxTickableNodes.first);
        callTickOnGroup(&mtxTickableNodes.second.firstTickGroup);
    }
    executeTasksAfterNodeTick();

    {
        std::scoped_lock guard(mtxTickableNodes.first);
        callTickOnGroup(&mtxTickableNodes.second.secondTickGroup);
    }
    executeTasksAfterNodeTick();
}

void World::executeTasksAfterNodeTick() {
    bool bExecutedAtLeastOneTask = false;
    do {
        bExecutedAtLeastOneTask = false;
        do {
            std::function<void()> task;

            {
                std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first);

                if (mtxTasksToExecuteAfterNodeTick.second.empty()) {
                    break;
                }

                task = std::move(mtxTasksToExecuteAfterNodeTick.second.front());
                mtxTasksToExecuteAfterNodeTick.second
                    .pop(); // remove first and only then execute to avoid recursion
            }

            // Execute the task while the mutex is not locked to avoid a deadlock,
            // because inside of this task the user might want to interact with some other thread
            // which also might want to add a new task.
            task();
            bExecutedAtLeastOneTask = true;
        } while (true); // TODO: rework this

        // Check if tasks that we executed added more tasks.
    } while (bExecutedAtLeastOneTask);
}

bool World::isNodeSpawned(size_t iNodeId) {
    std::scoped_lock guard(mtxSpawnedNodes.first);

    const auto it = mtxSpawnedNodes.second.find(iNodeId);

    return it != mtxSpawnedNodes.second.end();
}

void World::onNodeSpawned(Node* pNode) {
    {
        // Get node ID.
        const auto optionalNodeId = pNode->getNodeId();

        // Make sure ID is valid.
        if (!optionalNodeId.has_value()) [[unlikely]] {
            Error error(std::format(
                "node \"{}\" notified the world about being spawned but its ID is invalid",
                pNode->getNodeName()));
            error.showError();
            throw std::runtime_error(error.getFullErrorMessage());
        }

        const auto iNodeId = optionalNodeId.value();

        std::scoped_lock guard(mtxSpawnedNodes.first);

        // See if we already have a node with this ID.
        const auto it = mtxSpawnedNodes.second.find(iNodeId);
        if (it != mtxSpawnedNodes.second.end()) [[unlikely]] {
            Error error(std::format(
                "node \"{}\" with ID \"{}\" notified the world about being spawned but there is "
                "already a spawned node with this ID",
                pNode->getNodeName(),
                iNodeId));
            error.showError();
            throw std::runtime_error(error.getFullErrorMessage());
        }

        // Save node.
        mtxSpawnedNodes.second[iNodeId] = pNode;
    }

    std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first);

    // Add to our arrays as deferred task. Why I've decided to do it as a deferred task:
    // if we are right now iterating over an array of nodes that world stores (for ex. ticking, where
    // one node inside of its tick function decided to spawn another node would cause us to get here)
    // without deferred task we will modify array that we are iterating over which will cause
    // bad things.
    mtxTasksToExecuteAfterNodeTick.second.push([this, pNode]() {
        if (pNode->isCalledEveryFrame()) {
            addTickableNode(pNode);
        }

        if (pNode->isReceivingInput()) {
            addNodeToReceivingInputArray(pNode);
        }
    });
}

void World::onNodeDespawned(Node* pNode) {
    {
        // Get node ID.
        const auto optionalNodeId = pNode->getNodeId();

        // Make sure ID is valid.
        if (!optionalNodeId.has_value()) [[unlikely]] {
            Error error(std::format(
                "node \"{}\" notified the world about being despawned but its ID is invalid",
                pNode->getNodeName()));
            error.showError();
            throw std::runtime_error(error.getFullErrorMessage());
        }

        const auto iNodeId = optionalNodeId.value();

        std::scoped_lock guard(mtxSpawnedNodes.first);

        // See if we indeed have a node with this ID.
        const auto it = mtxSpawnedNodes.second.find(iNodeId);
        if (it == mtxSpawnedNodes.second.end()) [[unlikely]] {
            Error error(std::format(
                "node \"{}\" with ID \"{}\" notified the world about being despawned but this node's "
                "ID is not found",
                pNode->getNodeName(),
                iNodeId));
            error.showError();
            throw std::runtime_error(error.getFullErrorMessage());
        }

        // Remove node.
        mtxSpawnedNodes.second.erase(it);
    }

    std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first);

    // Remove to our arrays as deferred task (see onNodeSpawned for the reason).
    // Additionally, engine guarantees that all deferred tasks will be finished before GC runs
    // so it's safe to do so.
    mtxTasksToExecuteAfterNodeTick.second.push([this, pNode]() {
        // Force iterate over all ticking nodes and remove this node if it's marked as ticking in our
        // arrays.
        // We don't check `Node::isCalledEveryFrame` here simply because the node can disable it
        // and despawn right after disabling it. If we check `Node::isCalledEveryFrame` we
        // might not remove the node from our arrays and it will continue ticking (error).
        // We even have a test for this bug.
        removeTickableNode(pNode);

        // Not checking for `Node::isReceivingInput` for the same reason as above.
        removeNodeFromReceivingInputArray(pNode);
    });
}

void World::onSpawnedNodeChangedIsCalledEveryFrame(Node* pNode) {
    // Get node ID.
    const auto optionalNodeId = pNode->getNodeId();

    // Make sure the node ID is valid.
    if (!optionalNodeId.has_value()) [[unlikely]] {
        Error error(std::format("spawned node \"{}\" ID is invalid", pNode->getNodeName()));
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }
    const auto iNodeId = optionalNodeId.value();

    // Save previous state.
    const auto bPreviousIsCalledEveryFrame = !pNode->isCalledEveryFrame();

    std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first);

    // Modify our arrays as deferred task (see onNodeSpawned for the reason).
    mtxTasksToExecuteAfterNodeTick.second.push([this, pNode, iNodeId, bPreviousIsCalledEveryFrame]() {
        // Make sure this node is still spawned.
        const auto bIsSpawned = isNodeSpawned(iNodeId);
        if (!bIsSpawned) {
            // If it had "is called every frame" enabled it was removed from our arrays during despawn.
            return;
        }

        // Get current setting state.
        const auto bIsCalledEveryFrame = pNode->isCalledEveryFrame();

        if (bIsCalledEveryFrame == bPreviousIsCalledEveryFrame) {
            // The node changed the setting back, nothing to do.
            return;
        }

        if (!bPreviousIsCalledEveryFrame && bIsCalledEveryFrame) {
            // Was disabled but now enabled.
            addTickableNode(pNode);
        } else if (bPreviousIsCalledEveryFrame && !bIsCalledEveryFrame) {
            // Was enabled but now disabled.
            removeTickableNode(pNode);
        }
    });
}

void World::addTickableNode(Node* pNode) {
    // Pick the tick group that the node uses.
    std::unordered_set<Node*>* pTickGroup = nullptr;
    if (pNode->getTickGroup() == TickGroup::FIRST) {
        pTickGroup = &mtxTickableNodes.second.firstTickGroup;
    } else {
        pTickGroup = &mtxTickableNodes.second.secondTickGroup;
    }

    std::scoped_lock guard(mtxTickableNodes.first);
    pTickGroup->insert(pNode);
}

void World::removeTickableNode(Node* pNode) {
    // Pick the tick group that the node uses.
    std::unordered_set<Node*>* pTickGroup = nullptr;
    if (pNode->getTickGroup() == TickGroup::FIRST) {
        pTickGroup = &mtxTickableNodes.second.firstTickGroup;
    } else {
        pTickGroup = &mtxTickableNodes.second.secondTickGroup;
    }

    // Find in array.
    std::scoped_lock guard(mtxTickableNodes.first);
    const auto it = pTickGroup->find(pNode);
    if (it == pTickGroup->end()) {
        // Not found.
        // This might happen if the node had the setting disabled and then quickly
        // enabled and disabled it back.
        return;
    }

    // Remove from array.
    pTickGroup->erase(it);
}
