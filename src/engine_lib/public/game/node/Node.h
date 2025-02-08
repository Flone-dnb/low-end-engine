#pragma once

// Standard.
#include <string>
#include <mutex>
#include <vector>
#include <optional>

class GameInstance;

/**
 * Describes the order of ticking. Every object of the first tick group will tick first
 * then objects of the second tick group will tick and etc. This allows defining a special
 * order of ticking if some tick functions depend on another or should be executed first/last.
 *
 * Here, "ticking" means calling a function that should be called every frame.
 */
enum class TickGroup { FIRST, SECOND };

/**
 * Base class for game entities, allows being spawned in the world, attaching child nodes
 * or being attached to some parent node.
 */
class Node {
    // World is able to spawn root node.
    friend class World;

public:
    /** Creates a new node with a default name. */
    Node();

    /**
     * Creates a new node with the specified name.
     *
     * @param sName Name of this node.
     */
    Node(std::string_view sName);

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    virtual ~Node() = default;

    /**
     * Sets node's name.
     *
     * @param sName New name of this node.
     */
    void setNodeName(const std::string& sName);

    /**
     * Goes up the parent node chain (up to the world's root node if needed) to find
     * a first node that matches the specified node type and optionally node name.
     *
     * Template parameter NodeType specifies node type to look for. Note that
     * this means that we will use dynamic_cast to determine whether the node matches
     * the specified type or not. So if you are looking for a node with the type `Node`
     * this means that every node will match the type.
     *
     * @param sParentNodeName If not empty, nodes that match the specified node type will
     * also be checked to see if their name exactly matches the specified name.
     *
     * @return nullptr if not found, otherwise a valid pointer to the node.
     */
    template <typename NodeType>
        requires std::derived_from<NodeType, Node>
    NodeType* getParentNodeOfType(const std::string& sParentNodeName = "");

    /**
     * Goes down the child node chain to find a first node that matches the specified node type and
     * optionally node name.
     *
     * Template parameter NodeType specifies node type to look for. Note that
     * this means that we will use dynamic_cast to determine whether the node matches
     * the specified type or not. So if you are looking for a node with the type `Node`
     * this means that every node will match the type.
     *
     * @param sChildNodeName If not empty, nodes that match the specified node type will
     * also be checked to see if their name exactly matches the specified name.
     *
     * @return nullptr if not found, otherwise a valid pointer to the node.
     */
    template <typename NodeType>
        requires std::derived_from<NodeType, Node>
    NodeType* getChildNodeOfType(const std::string& sChildNodeName = "");

    /**
     * Returns whether the @ref onBeforeNewFrame should be called each frame or not.
     *
     * @return Whether the @ref onBeforeNewFrame should be called each frame or not.
     */
    bool isCalledEveryFrame();

    /**
     * Returns whether this node is spawned in the world or not.
     *
     * @return Whether this node is spawned in the world or not.
     */
    bool isSpawned();

    /**
     * Checks if the specified node is a child of this node (somewhere in the child hierarchy,
     * not only as a direct child node).
     *
     * @param pNode Node to check.
     *
     * @return `true` if the specified node was found as a child of this node, `false` otherwise.
     */
    bool isParentOf(Node* pNode);

    /**
     * Checks if the specified node is a parent of this node (somewhere in the parent hierarchy,
     * not only as a direct parent node).
     *
     * @param pNode Node to check.
     *
     * @return `true` if the specified node was found as a parent of this node, `false` otherwise.
     */
    bool isChildOf(Node* pNode);

    /**
     * Returns node's name.
     *
     * @return Node name.
     */
    std::string_view getNodeName() const;

    /**
     * Returns a unique ID of the node.
     *
     * @remark Each spawn gives the node a new ID.
     *
     * @return Empty if this node was never spawned, otherwise unique ID of this node.
     */
    std::optional<size_t> getNodeId() const;

    /**
     * Returns the tick group this node resides in.
     *
     * @return Tick group the node is using.
     */
    TickGroup getTickGroup() const;

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     */
    virtual void onSpawning() {}

    /**
     * Called after @ref onSpawning when this node and all of node's child nodes (at the moment
     * of spawning) were spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     *
     * @remark Generally you might want to prefer to use @ref onSpawning, this function
     * is mostly used to do some logic related to child nodes after all child nodes were spawned
     * (for example if you have a camera child node you can make it active in this function).=
     */
    virtual void onChildNodesSpawned() {}

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() {}

    /**
     * Called before a new frame is rendered.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic (if there is any).
     *
     * @remark This function is disabled by default, use @ref setIsCalledEveryFrame to enable it.
     * @remark This function will only be called while this node is spawned.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) {}

    /**
     * Determines if the @ref onBeforeNewFrame should be called each frame or not
     * (disabled by default).
     *
     * @remark Safe to call any time (while spawned/despawned).
     *
     * @param bEnable `true` to enable @ref onBeforeNewFrame, `false` to disable.
     */
    void setIsCalledEveryFrame(bool bEnable);

    /**
     * Sets the tick group in which the node will reside.
     *
     * Tick groups determine the order in which the @ref onBeforeNewFrame functions will be called
     * on nodes. Each frame, @ref onBeforeNewFrame will be called first on the nodes that use
     * the first tick group, then on the nodes that use the second group and etc. This allows
     * defining a special order in which @ref onBeforeNewFrame functions will be called on nodes,
     * thus if you want some nodes to execute their @ref onBeforeNewFrame function only after
     * some other nodes do so, you can define this with tick groups.
     *
     * @warning Calling this function while the node is spawned will cause an error to be shown.
     *
     * @remark Tick group is ignored if @ref setIsCalledEveryFrame was not enabled.
     * @remark Typically you should call this function in your node's constructor to determine
     * in which tick group the node will reside.
     * @remark Nodes use the first tick group by default.
     *
     * @param tickGroup Tick group the node will reside in.
     */
    void setTickGroup(TickGroup tickGroup);

    /**
     * Returns game instance from the world in which the node is spawned.
     *
     * @warning Calling this function while the node is not spawned will cause an error to be shown.
     *
     * @return Always valid pointer to the game instance.
     */
    GameInstance* getGameInstanceWhileSpawned();

    /**
     * Returns mutex that is generally used to protect/prevent spawning/despawning.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Mutex.
     */
    std::recursive_mutex* getSpawnDespawnMutex();

private:
    /** Calls @ref onSpawning on this node and all of its child nodes. */
    void spawn();

    /** Calls @ref onDespawning on this node and all of its child nodes. */
    void despawn();

    /**
     * Asks this node's parent and goes up the node hierarchy
     * up to the root node if needed to find a valid pointer to world.
     *
     * @return Valid world pointer.
     */
    World* askParentsAboutWorldPointer();

    /** Attached child nodes. */
    std::pair<std::recursive_mutex, std::vector<std::unique_ptr<Node>>> mtxChildNodes;

    /** Attached parent node. */
    std::pair<std::recursive_mutex, Node*> mtxParentNode;

    /** Determines if the @ref onBeforeNewFrame should be called each frame or not. */
    std::pair<std::recursive_mutex, bool> mtxIsCalledEveryFrame;

    /** Whether this node is spawned in the world or not. */
    std::pair<std::recursive_mutex, bool> mtxIsSpawned;

    /**
     * Initialized after the node is spawned and reset when despawned.
     *
     * @warning Do not delete (free) this pointer.
     */
    World* pWorldWeSpawnedIn = nullptr;

    /** Tick group used by this node. */
    TickGroup tickGroup = TickGroup::FIRST;

    /** Unique ID of the spawned node (initialized after the node is spawned). */
    std::optional<size_t> iNodeId;

    /** Node's name. */
    std::string sNodeName;
};

template <typename NodeType>
    requires std::derived_from<NodeType, Node>
inline NodeType* Node::getParentNodeOfType(const std::string& sParentNodeName) {
    std::scoped_lock guard(mtxParentNode.first);

    // Check if have a parent.
    if (mtxParentNode.second == nullptr) {
        return nullptr;
    }

    // Check parent's type and optionally name.
    const auto pCastedParentNode = dynamic_cast<NodeType*>(mtxParentNode.second);
    if (pCastedParentNode != nullptr &&
        (sParentNodeName.empty() || mtxParentNode.second->getNodeName() == sParentNodeName)) {
        // Found the node.
        return pCastedParentNode;
    }

    // Ask parent nodes of that node.
    return mtxParentNode.second->getParentNodeOfType<NodeType>(sParentNodeName);
}

template <typename NodeType>
    requires std::derived_from<NodeType, Node>
inline NodeType* Node::getChildNodeOfType(const std::string& sChildNodeName) {
    std::scoped_lock guard(mtxChildNodes.first);

    // Iterate over child nodes.
    for (auto& pChildNode : mtxChildNodes.second) {
        // Check if this is the node we are looking for.
        const auto pCastedChildNode = dynamic_cast<NodeType*>(pChildNode.get());
        if (pCastedChildNode != nullptr &&
            (sChildNodeName.empty() || pChildNode->getNodeName() == sChildNodeName)) {
            // Found the node.
            return pCastedChildNode;
        }

        // Ask child nodes of that node.
        const auto pNode = pChildNode->getChildNodeOfType<NodeType>(sChildNodeName);
        if (pNode == nullptr) {
            // Check the next child node.
            continue;
        }

        // Found the node.
        return pNode;
    }

    return nullptr;
}
