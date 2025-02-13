#pragma once

// Standard.
#include <memory>
#include <mutex>
#include <functional>
#include <queue>
#include <unordered_set>

class Node;
class GameManager;

/** Tiny RAII-like class that locks a mutex and changes a boolean while alive. */
class ReceivingInputNodesGuard {
public:
    ReceivingInputNodesGuard() = delete;
    ReceivingInputNodesGuard(const ReceivingInputNodesGuard&) = delete;
    ReceivingInputNodesGuard& operator=(const ReceivingInputNodesGuard&) = delete;

    /**
     * Initializes the object.
     *
     * @param pMutexToLock Mutex with boolean to change.
     * @param pNodes       Nodes that receive input.
     */
    ReceivingInputNodesGuard(
        std::pair<std::recursive_mutex, bool>* pMutexToLock,
        std::pair<std::recursive_mutex, std::unordered_set<Node*>>* pNodes)
        : pMutexToLock(pMutexToLock), pNodes(pNodes) {
        pMutexToLock->first.lock();
        pMutexToLock->second = true;
        pNodes->first.lock();
    }

    /**
     * Returns all spawned nodes that receive input.
     *
     * @return Nodes.
     */
    std::unordered_set<Node*>* getNodes() const { return &pNodes->second; }

    ~ReceivingInputNodesGuard() {
        pMutexToLock->second = false;
        pMutexToLock->first.unlock();
        pNodes->first.unlock();
    }

private:
    /** Mutex with boolean to change. */
    std::pair<std::recursive_mutex, bool>* pMutexToLock = nullptr;

    /** Input receiving nodes. */
    std::pair<std::recursive_mutex, std::unordered_set<Node*>>* const pNodes = nullptr;
};

/** Represents a game world. Owns world's root node. */
class World {
    // Nodes notify the world about being spawned/despawned.
    friend class Node;

public:
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /**
     * Initializes world.
     *
     * @param pGameManager Object that owns this world.
     */
    World(GameManager* pGameManager);

    /** Clears pointer to the root node which causes the world to recursively be despawned and destroyed. */
    void destroyWorld();

    /**
     * Returns spawned nodes that receiving input.
     *
     * @return Nodes.
     */
    ReceivingInputNodesGuard getReceivingInputNodes();

    /**
     * Called before a new frame is rendered.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    void tickTickableNodes(float timeSincePrevCallInSec);

    /**
     * Quickly tests if a node with the specified ID is currently spawned or not.
     *
     * @param iNodeId ID of the node to check.
     *
     * @return `true` if the node is spawned, `false` otherwise.
     */
    bool isNodeSpawned(size_t iNodeId);

private:
    /** Represents arrays of nodes that are marked as "should be called every frame". */
    struct TickableNodes {
        TickableNodes() = default;

        TickableNodes(const TickableNodes&) = delete;
        TickableNodes& operator=(const TickableNodes&) = delete;

        /** Nodes of the first tick group. */
        std::unordered_set<Node*> firstTickGroup;

        /** Nodes of the second tick group. */
        std::unordered_set<Node*> secondTickGroup;
    };

    /**
     * Called from Node to notify the World about a new node being spawned.
     *
     * @param pNode Node that is being spawned.
     */
    void onNodeSpawned(Node* pNode);

    /**
     * Called from Node to notify the World about a node being despawned.
     *
     * @param pNode Node that is being despawned.
     */
    void onNodeDespawned(Node* pNode);

    /**
     * Adds the specified node to @ref mtxTickableNodes.
     *
     * @param pNode Node to add.
     */
    void addTickableNode(Node* pNode);

    /**
     * Looks if the specified node exists in @ref mtxTickableNodes and removes the node.
     *
     * @param pNode Node to remove.
     */
    void removeTickableNode(Node* pNode);

    /**
     * Called from Node to notify the World about a spawned node changed its "is called every frame"
     * setting.
     *
     * @warning Should be called AFTER the node has changed its setting and the new state should not
     * be changed while this function is running.
     *
     * @param pNode Node that is changing its setting.
     */
    void onSpawnedNodeChangedIsCalledEveryFrame(Node* pNode);

    /**
     * Adds the specified node to the array of "receiving input" nodes
     * (see @ref getReceivingInputNodes).
     *
     * @param pNode Node to add.
     */
    void addNodeToReceivingInputArray(Node* pNode);

    /**
     * Looks if the specified node exists in the array of "receiving input" nodes
     * and removes the node from the array (see @ref mtxReceivingInputNodes).
     *
     * @param pNode Node to remove.
     */
    void removeNodeFromReceivingInputArray(Node* pNode);

    /**
     * Called from Node to notify the World about a spawned node changed its "is receiving input"
     * setting.
     *
     * @warning Should be called AFTER the node has changed its setting and the new state should not
     * be changed while this function is running.
     *
     * @param pNode Node that is changing its setting.
     */
    void onSpawnedNodeChangedIsReceivingInput(Node* pNode);

    /**
     * Must be called after finishing iterating over all "tickable" nodes of one node group to finish
     * possible node spawn/despawn logic.
     */
    void executeTasksAfterNodeTick();

    /** Nodes that should be called every frame. */
    std::pair<std::recursive_mutex, TickableNodes> mtxTickableNodes;

    /** Array of currently spawned nodes that receive input. */
    std::pair<std::recursive_mutex, std::unordered_set<Node*>> mtxReceivingInputNodes;

    /**
     * Functions to execute after nodes did their per-frame logic.
     *
     * Used in order to avoid modifying the array that we are currently iterating over.
     *
     * Imagine a case when we iterate over @ref mtxTickableNodes and some node's tick function spawns
     * or despawns some node(s) but in this case we can't modify the array of "tickable" nodes because we
     * are currently iterating over it. Thus we push a "deferred task" in this array and execute this task
     * after we finished iterating over those nodes.
     */
    std::pair<std::mutex, std::queue<std::function<void()>>> mtxTasksToExecuteAfterNodeTick;

    /** World's root node. */
    std::pair<std::mutex, std::unique_ptr<Node>> mtxRootNode;

    /** Stores pairs of "Node ID" - "Spawned Node". */
    std::pair<std::mutex, std::unordered_map<size_t, Node*>> mtxSpawnedNodes;

    /**
     * True if we are currently in a loop where we call every "ticking" node or a node that receiving input.
     */
    std::pair<std::recursive_mutex, bool> mtxIsIteratingOverNodes;

    /** Do not delete (free) this pointer. Always valid pointer to game manager. */
    GameManager* const pGameManager = nullptr;
};
