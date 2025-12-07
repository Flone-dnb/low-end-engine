#pragma once

// Standard.
#include <memory>
#include <mutex>
#include <functional>
#include <queue>
#include <unordered_set>
#include <optional>

// Custom.
#include "game/node/NodeTickGroup.hpp"
#include "render/Renderer.h"

class Node;
class GameManager;
class CameraManager;
class UiNodeManager;
class MeshRenderer;
class LightSourceManager;
class ParticleRenderer;

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

// ------------------------------------------------------------------------------------------------

/** Represents a game world. Owns world's root node. */
class World {
    // Nodes notify the world about being spawned/despawned.
    friend class Node;

    // Ticks world nodes and destroys worlds.
    friend class GameManager;

public:
    /** GL GPU time queries. */
    struct FrameQueries {
        /** GL query ID for measuring GPU time that we spent drawing shadow pass. */
        unsigned int iGlQueryToDrawShadowPass = 0;

        /** GL query ID for measuring GPU time that we spent drawing depth prepass. */
        unsigned int iGlQueryToDrawDepthPrepass = 0;

        /** GL query ID for measuring GPU time that we spent drawing meshes. */
        unsigned int iGlQueryToDrawMeshes = 0;
    };

    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /**
     * Initializes world.
     *
     * @param pGameManager   Object that owns this world.
     * @param sName          Name of the world, used for logging.
     * @param pRootNodeToUse Optionally specify a root node to use, if `nullptr` a new root node will be
     * created.
     */
    World(
        GameManager* pGameManager, const std::string& sName, std::unique_ptr<Node> pRootNodeToUse = nullptr);

    /**
     * Despawns all nodes and attaches old root node's child nodes to the specified node then spawns root
     * node.
     *
     * @param pNewRoot New root node.
     */
    void changeRootNode(std::unique_ptr<Node> pNewRoot);

    /**
     * Returns spawned nodes that receive input.
     *
     * @return Nodes.
     */
    ReceivingInputNodesGuard getReceivingInputNodes();

    /**
     * Quickly tests if a node with the specified ID is currently spawned or not.
     *
     * @param iNodeId ID of the node to check.
     *
     * @return `true` if the node is spawned, `false` otherwise.
     */
    bool isNodeSpawned(size_t iNodeId);

    /**
     * Returns a pointer to world's root node.
     *
     * @return `nullptr` if world is being destroyed, otherwise pointer to world's root node.
     */
    Node* getRootNode();

    /**
     * Returns spawned node by the specified node ID (if found).
     *
     * @param iNodeId ID of the node to get.
     *
     * @return `nullptr` if not found.
     */
    Node* getSpawnedNodeById(size_t iNodeId);

    /**
     * Returns camera manager.
     *
     * @return Manager.
     */
    CameraManager& getCameraManager() const;

    /**
     * Returns UI node manager.
     *
     * @return Manager.
     */
    UiNodeManager& getUiNodeManager() const;

    /**
     * Returns mesh renderer.
     *
     * @return Renderer.
     */
    MeshRenderer& getMeshRenderer() const;

    /**
     * Returns particle renderer.
     *
     * @return Renderer.
     */
    ParticleRenderer& getParticleRenderer() const;

    /**
     * Returns manager used to add/remove light sources to/from rendering.
     *
     * @return Manager.
     */
    LightSourceManager& getLightSourceManager() const;

    /**
     * Returns game manager.
     *
     * @return Game manager.
     */
    GameManager& getGameManager() const { return *pGameManager; }

    /**
     * Returns total amount of currently spawned nodes.
     *
     * @return Total nodes spawned right now.
     */
    size_t getTotalSpawnedNodeCount();

    /**
     * Returns the current amount of spawned nodes that are marked as "should be called every frame".
     *
     * @return Amount of spawned nodes that should be called every frame.
     */
    size_t getCalledEveryFrameNodeCount();

    /**
     * Returns name of the world.
     *
     * @return Name.
     */
    std::string_view getName() const { return sName; }

    /**
     * Returns GPU time queries.
     *
     * @return Time queries.
     */
    std::array<FrameQueries, iFramesInFlight>& getFrameQueries() { return vFrameQueries; }

private:
    /** Represents arrays of nodes that are marked as "should be called every frame". */
    struct TickableNodes {
        TickableNodes() = default;

        TickableNodes(const TickableNodes&) = delete;
        TickableNodes& operator=(const TickableNodes&) = delete;

        /**
         * Returns the number of nodes called every frame (includes all tick groups).
         *
         * @return Node count.
         */
        size_t getTotalNodeCount() const { return firstTickGroup.size() + secondTickGroup.size(); }

        /** Nodes of the first tick group. */
        std::unordered_set<Node*> firstTickGroup;

        /** Nodes of the second tick group. */
        std::unordered_set<Node*> secondTickGroup;
    };

    /** GL queries. */
    std::array<FrameQueries, iFramesInFlight> vFrameQueries;

    /**
     * Called before a new frame is rendered.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    void tickTickableNodes(float timeSincePrevCallInSec);

    /** Clears pointer to the root node which causes the world to recursively be despawned and destroyed. */
    void destroyWorld();

    /** Called after window size changed. */
    void onWindowSizeChanged();

    /**
     * Called from Node to notify the World about a new node being spawned.
     *
     * @param pNode Node that is being spawned.
     */
    void onNodeSpawned(Node* pNode);

    /**
     * Called from Node to notify the World about a node being despawned.
     *
     * @warning After this function is finished node pointer can point to deleted memory.
     *
     * @param pNodeToBeDeleted Node that is being despawned.
     */
    void onNodeDespawned(Node* pNodeToBeDeleted);

    /**
     * Adds the specified node to @ref mtxTickableNodes.
     *
     * @param pNode Node to add.
     */
    void addTickableNode(Node* pNode);

    /**
     * Looks if the specified node exists in @ref mtxTickableNodes and removes the node.
     *
     * @param pMaybeDeletedNode      Node to remove.
     * @param tickGroupOfDeletedNode Specify if the node points to deleted memory.
     */
    void removeTickableNode(Node* pMaybeDeletedNode, std::optional<TickGroup> tickGroupOfDeletedNode);

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
     * @param pMaybeDeletedNode Node to remove. Can point to deleted memory.
     */
    void removeNodeFromReceivingInputArray(Node* pMaybeDeletedNode);

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

    /** Stores pairs of "Node ID" - "spawned Node". */
    std::pair<std::mutex, std::unordered_map<size_t, Node*>> mtxSpawnedNodes;

    /**
     * True if we are currently in a loop where we call every "ticking" node or a node that receiving input.
     */
    std::pair<std::recursive_mutex, bool> mtxIsIteratingOverNodes;

    /** Manages all UI nodes. */
    std::unique_ptr<UiNodeManager> pUiNodeManager;

    /** Manages mesh rendering. */
    std::unique_ptr<MeshRenderer> pMeshRenderer;

    /** Manages particle rendering. */
    std::unique_ptr<ParticleRenderer> pParticleRenderer;

    /** Manages light sources (nodes). */
    std::unique_ptr<LightSourceManager> pLightSourceManager;

    /** Determines which camera is used as in-game eyes. */
    std::unique_ptr<CameraManager> pCameraManager;

    /** Name of the world, used for logging. */
    const std::string sName;

    /** Do not delete (free) this pointer. Always valid pointer to game manager. */
    GameManager* const pGameManager = nullptr;
};
