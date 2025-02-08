#pragma once

// Standard.
#include <memory>
#include <mutex>
#include <functional>
#include <queue>
#include <unordered_set>

class Node;
class GameManager;

/** Represents a game world. Owns world's root node. */
class World {
    // Nodes notify the world about being spawned/despawned.
    friend class Node;

public:
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /**
     * Creates a new world that contains only one node - root node.
     *
     * @param pGameManager Object that owns this world.
     *
     * @return Pointer to the new world instance.
     */
    static std::unique_ptr<World> createWorld(GameManager* pGameManager);

    /** Clears pointer to the root node which causes the world to recursively be despawned and destroyed. */
    void destroyWorld();

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
     * Initializes world.
     *
     * @param pGameManager Object that owns this world.
     */
    World(GameManager* pGameManager);

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
     * Must be called after finishing iterating over all "tickable" nodes of one node group to finish
     * possible node spawn/despawn logic.
     */
    void executeTasksAfterNodeTick();

    /** Nodes that should be called every frame. */
    std::pair<std::mutex, TickableNodes> mtxTickableNodes;

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

    /** Do not delete (free) this pointer. Always valid pointer to game manager. */
    GameManager* const pGameManager = nullptr;
};
