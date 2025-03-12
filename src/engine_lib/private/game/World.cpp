#include "game/World.h"

// Custom.
#include "game/node/Node.h"
#include "io/Logger.h"
#include "misc/Error.h"
#include "render/Renderer.h"

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
    mtxRootNode.second->pWorldWeSpawnedIn = this;
    mtxRootNode.second->spawn();
}

void World::destroyWorld() {
    Logger::get().info("world is being destroyed, despawning world's root node...");
    Logger::get().flushToDisk();

    // Just in case wait for all GPU work to finish.
    Renderer::waitForGpuToFinishWorkUpToThisPoint();

    std::scoped_lock guard(mtxRootNode.first);
    mtxRootNode.second->despawn();
    mtxRootNode.second = nullptr;

    // Make sure that all nodes were destroyed.
    const auto iAliveNodeCount = Node::getAliveNodeCount();
    if (iAliveNodeCount != 0) [[unlikely]] {
        Logger::get().error(
            std::format("the world was destroyed but there are still {} node(s) alive", iAliveNodeCount));
    }
}

ReceivingInputNodesGuard World::getReceivingInputNodes() {
    return ReceivingInputNodesGuard(&mtxIsIteratingOverNodes, &mtxReceivingInputNodes);
}

void World::tickTickableNodes(float timeSincePrevCallInSec) {
    std::scoped_lock guard(mtxTickableNodes.first, mtxIsIteratingOverNodes.first);

    mtxIsIteratingOverNodes.second = true;
    {
        executeTasksAfterNodeTick();

        const auto callTickOnGroup = [&](std::unordered_set<Node*>* pTickGroup) {
            for (auto it = pTickGroup->begin(); it != pTickGroup->end(); ++it) {
                (*it)->onBeforeNewFrame(timeSincePrevCallInSec);
            }
        };

        callTickOnGroup(&mtxTickableNodes.second.firstTickGroup);
        executeTasksAfterNodeTick();

        callTickOnGroup(&mtxTickableNodes.second.secondTickGroup);
        executeTasksAfterNodeTick();
    }
    mtxIsIteratingOverNodes.second = false;
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

Node* World::getRootNode() {
    std::scoped_lock guard(mtxRootNode.first);
    return mtxRootNode.second.get();
}

size_t World::getTotalSpawnedNodeCount() {
    std::scoped_lock guard(mtxSpawnedNodes.first);

    return mtxSpawnedNodes.second.size();
}

size_t World::getCalledEveryFrameNodeCount() {
    std::scoped_lock guard(mtxTickableNodes.first);

    return mtxTickableNodes.second.getTotalNodeCount();
}

void World::onNodeSpawned(Node* pNode) {
    {
        // Get node ID.
        const auto optionalNodeId = pNode->getNodeId();

        // Make sure ID is valid.
        if (!optionalNodeId.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" notified the world about being spawned but its ID is invalid",
                pNode->getNodeName()));
        }

        const auto iNodeId = optionalNodeId.value();

        std::scoped_lock guard(mtxSpawnedNodes.first);

        // See if we already have a node with this ID.
        const auto it = mtxSpawnedNodes.second.find(iNodeId);
        if (it != mtxSpawnedNodes.second.end()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" with ID \"{}\" notified the world about being spawned but there is "
                "already a spawned node with this ID",
                pNode->getNodeName(),
                iNodeId));
        }

        // Save node.
        mtxSpawnedNodes.second[iNodeId] = pNode;
    }

    {
        std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first, mtxIsIteratingOverNodes.first);

        const auto executeOperation = [this, pNode]() {
            if (pNode->isCalledEveryFrame()) {
                addTickableNode(pNode);
            }

            if (pNode->isReceivingInput()) {
                addNodeToReceivingInputArray(pNode);
            }
        };

        if (mtxIsIteratingOverNodes.second) {
            // Add to our arrays as "deferred" task because we are currently iterating over an array of
            // tickable nodes, without this "deferred" task we will modify array that we are iterating over
            // which will cause bad things to happen.
            mtxTasksToExecuteAfterNodeTick.second.push(executeOperation);
        } else {
            executeOperation();
        }
    }
}

void World::onNodeDespawned(Node* pNodeToBeDeleted) {
    {
        // Get node ID.
        const auto optionalNodeId = pNodeToBeDeleted->getNodeId();

        // Make sure ID is valid.
        if (!optionalNodeId.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" notified the world about being despawned but its ID is invalid",
                pNodeToBeDeleted->getNodeName()));
        }

        const auto iNodeId = optionalNodeId.value();

        std::scoped_lock guard(mtxSpawnedNodes.first);

        // See if we indeed have a node with this ID.
        const auto it = mtxSpawnedNodes.second.find(iNodeId);
        if (it == mtxSpawnedNodes.second.end()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" with ID \"{}\" notified the world about being despawned but this node's "
                "ID is not found",
                pNodeToBeDeleted->getNodeName(),
                iNodeId));
        }

        // Remove node.
        mtxSpawnedNodes.second.erase(it);
    }

    {
        std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first, mtxIsIteratingOverNodes.first);

        const auto executeOperation =
            [this, pDeletedNode = pNodeToBeDeleted, tickGroup = pNodeToBeDeleted->getTickGroup()]() {
                // Force iterate over all ticking nodes and remove this node if it's marked as ticking in our
                // arrays.
                // We don't check `Node::isCalledEveryFrame` here simply because the node can disable it
                // and despawn right after disabling it. If we check `Node::isCalledEveryFrame` we
                // might not remove the node from our arrays and it will continue ticking (error).
                // We even have a test for this bug.
                removeTickableNode(pDeletedNode, tickGroup);

                // Not checking for `Node::isReceivingInput` for the same reason as above.
                removeNodeFromReceivingInputArray(pDeletedNode);
            };

        if (mtxIsIteratingOverNodes.second) {
            // Modify our array as "deferred" task (see onNodeSpawned for the reason).
            mtxTasksToExecuteAfterNodeTick.second.push(executeOperation);
        } else {
            executeOperation();
        }
    }
}

void World::onSpawnedNodeChangedIsCalledEveryFrame(Node* pNode) {
    // Get node ID.
    const auto optionalNodeId = pNode->getNodeId();

    // Make sure the node ID is valid.
    if (!optionalNodeId.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("spawned node \"{}\" ID is invalid", pNode->getNodeName()));
    }
    const auto iNodeId = optionalNodeId.value();

    // Save previous state.
    const auto bPreviousIsCalledEveryFrame = !pNode->isCalledEveryFrame();

    {
        std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first, mtxIsIteratingOverNodes.first);

        const auto executeOperation = [this, pNode, iNodeId, bPreviousIsCalledEveryFrame]() {
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
                removeTickableNode(pNode, {});
            }
        };

        if (mtxIsIteratingOverNodes.second) {
            // Modify our array as "deferred" task (see onNodeSpawned for the reason).
            mtxTasksToExecuteAfterNodeTick.second.push(executeOperation);
        } else {
            executeOperation();
        }
    }
}

void World::addNodeToReceivingInputArray(Node* pNode) {
    std::scoped_lock guard(mtxReceivingInputNodes.first);

    mtxReceivingInputNodes.second.insert(pNode);
}

void World::removeNodeFromReceivingInputArray(Node* pMaybeDeletedNode) {
    std::scoped_lock guard(mtxReceivingInputNodes.first);

    // Find in array.
    const auto it = mtxReceivingInputNodes.second.find(pMaybeDeletedNode);
    if (it == mtxReceivingInputNodes.second.end()) {
        // Not found.
        // This might happen if the node had the setting disabled and then quickly
        // enabled and disabled it back
        return;
    }

    // Remove from array.
    mtxReceivingInputNodes.second.erase(it);
}

void World::onSpawnedNodeChangedIsReceivingInput(Node* pNode) {
    // Get node ID.
    const auto optionalNodeId = pNode->getNodeId();

    // Make sure ID is valid.
    if (!optionalNodeId.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("spawned node \"{}\" ID is invalid", pNode->getNodeName()));
    }

    // Save node ID.
    const auto iNodeId = optionalNodeId.value();

    // Save previous state.
    const auto bPreviousIsReceivingInput = !pNode->isReceivingInput();

    {
        std::scoped_lock guard(mtxTasksToExecuteAfterNodeTick.first, mtxIsIteratingOverNodes.first);

        const auto executeOperation = [this, pNode, iNodeId, bPreviousIsReceivingInput]() {
            // Make sure this node is still spawned.
            const auto bIsSpawned = isNodeSpawned(iNodeId);
            if (!bIsSpawned) {
                // If it had "receiving input" enabled it was removed from our array during despawn.
                return;
            }

            // Get current setting state.
            const auto bIsReceivingInput = pNode->isReceivingInput();

            if (bIsReceivingInput == bPreviousIsReceivingInput) {
                // The node changed the setting back, nothing to do.
                return;
            }

            if (!bPreviousIsReceivingInput && bIsReceivingInput) {
                // Was disabled but now enabled.
                addNodeToReceivingInputArray(pNode);
            } else if (bPreviousIsReceivingInput && !bIsReceivingInput) {
                // Was enabled but now disabled.
                removeNodeFromReceivingInputArray(pNode);
            }
        };

        if (mtxIsIteratingOverNodes.second) {
            // Modify our array as "deferred" task (see onNodeSpawned for the reason).
            mtxTasksToExecuteAfterNodeTick.second.push(executeOperation);
        } else {
            executeOperation();
        }
    }
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

void World::removeTickableNode(Node* pMaybeDeletedNode, std::optional<TickGroup> tickGroupOfDeletedNode) {
    // Pick the tick group that the node used.
    std::unordered_set<Node*>* pTickGroup = nullptr;

    TickGroup tickGroup = TickGroup::FIRST;
    if (tickGroupOfDeletedNode.has_value()) {
        tickGroup = *tickGroupOfDeletedNode;
    } else {
        tickGroup = pMaybeDeletedNode->getTickGroup();
    }

    if (tickGroup == TickGroup::FIRST) {
        pTickGroup = &mtxTickableNodes.second.firstTickGroup;
    } else {
        pTickGroup = &mtxTickableNodes.second.secondTickGroup;
    }

    // Find in array.
    std::scoped_lock guard(mtxTickableNodes.first);
    const auto it = pTickGroup->find(pMaybeDeletedNode);
    if (it == pTickGroup->end()) {
        // Not found.
        // This might happen if the node had the setting disabled and then quickly
        // enabled and disabled it back.
        return;
    }

    // Remove from array.
    pTickGroup->erase(it);
}
