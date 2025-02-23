#pragma once

// Standard.
#include <string>
#include <mutex>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>
#include <memory>
#include <variant>

// Custom.
#include "input/KeyboardButton.hpp"
#include "game/node/NodeTickGroup.hpp"

class GameInstance;
class World;

/**
 * Base class for game entities, allows being spawned in the world, attaching child nodes
 * or being attached to some parent node.
 */
class Node {
    // World is able to spawn root node.
    friend class World;

    // GameManager will propagate functions to all nodes in the world such as `onBeforeNewFrame`.
    friend class GameManager;

public:
    /**
     * Defines how location, rotation or scale of a node being attached as a child node
     * should change after the attachment process (after `onAfterAttachedToNewParent` was called).
     */
    enum class AttachmentRule {
        RESET_RELATIVE, //< After the new child node was attached, resets its relative location or rotation to
                        // 0 and relative scale to 1.
        KEEP_RELATIVE,  //< After the new child node was attached, its relative location/rotation/scale will
                        // stay the same, but world location/rotation/scale might change.
        KEEP_WORLD, //< After the new child node was attached, its relative location/rotation/scale will be
                    // recalculated so that its world location/rotation/scale will stay the same (as before
                    // attached).
    };

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

    virtual ~Node();

    /**
     * Returns the total amount of currently alive (allocated) nodes.
     *
     * @return Number of alive nodes right now.
     */
    static size_t getAliveNodeCount();

    /**
     * Attaches a node as a child of this node.
     *
     * @remark If the specified node already has a parent it will change its parent to be
     * a child of this node. This way you can change to which node you are attached.
     *
     * @remark If the specified node needs to be spawned it will queue a deferred task to be added
     * to the World on next frame so input events and @ref onBeforeNewFrame (if enabled) will be called
     * only starting from the next frame.
     *
     * @param node         Node to attach as a child. If the specified node does not have a parent provide
     * a unique_ptr instead of the raw pointer. If the node does not have a parent but you provide a raw
     * pointer and error will be shown. If the specified node is a parent of `this` node the operation will
     * fail and log an error.
     * @param locationRule Only applied if the child node is a SpatialNode, otherwise ignored.
     * Defines how child node's location should change after the attachment process
     * (after `onAfterAttachedToNewParent` was called)
     * @param rotationRule Only applied if the child node is a SpatialNode, otherwise ignored.
     * Defines how child node's rotation should change after the attachment process
     * (after `onAfterAttachedToNewParent` was called)
     * @param scaleRule    Only applied if the child node is a SpatialNode, otherwise ignored.
     * Defines how child node's scale should change after the attachment process
     * (after `onAfterAttachedToNewParent` was called)
     */
    void addChildNode(
        std::variant<std::unique_ptr<Node>, Node*> node,
        AttachmentRule locationRule = AttachmentRule::KEEP_WORLD,
        AttachmentRule rotationRule = AttachmentRule::KEEP_WORLD,
        AttachmentRule scaleRule = AttachmentRule::KEEP_WORLD);

    /**
     * Detaches this node from the parent and optionally despawns this node and
     * all of its child nodes if the node was spawned.
     *
     * @warning THIS FUNCTION IS UNSAFE. Because we use a strict ownership where parent nodes store
     * unique pointers to child nodes and everything else is raw pointers in the result of this
     * function the node and its child nodes will be deleted (freed) and any entity that stores
     * a raw pointer to this node (or its child nodes) will then store a raw pointer to a deleted memory.
     * We expect that you don't despawn nodes during the gameplay due to this raw pointer risk,
     * moreover we expect you to load the level with (almost) all nodes that you will need
     * during the whole level (just hide/move away the nodes that you don't need to be visible always).
     * If you use this function then you must guarantee that no other game entity is referencing
     * this node or any of its child nodes.
     *
     * @warning After this function is finished `this` should not be used as it will be deleted (freed).
     *
     * @remark This function is usually used to mark node (tree) as "to be destroyed", if you
     * just want to change node's parent consider using @ref addChildNode.
     */
    void unsafeDetachFromParentAndDespawn();

    /**
     * Sets node's name.
     *
     * @param sName New name of this node.
     */
    void setNodeName(const std::string& sName);

    /**
     * Returns world's root node.
     *
     * @return `nullptr` if this node is not spawned or was despawned or world is being destroyed
     * (always check returned pointer before doing something), otherwise valid pointer.
     */
    Node* getWorldRootNodeWhileSpawned();

    /**
     * Returns parent node if this node.
     *
     * @warning Must be used with mutex.
     *
     * @warning Avoid saving returned raw pointer as it points to the node's field and does not
     * guarantee that the node will always live while you hold this pointer.
     *
     * @return `nullptr` as a pointer (second value in the pair) if this node has no parent
     * (could only happen when the node is not spawned), otherwise valid pointer.
     */
    std::pair<std::recursive_mutex*, Node*> getParentNode();

    /**
     * Returns pointer to child nodes array.
     *
     * @warning Must be used with mutex.
     *
     * @warning Avoid saving returned raw pointer as it points to the node's field and does not
     * guarantee that the node will always live while you hold this pointer.
     *
     * @return Array of child nodes.
     */
    std::pair<std::recursive_mutex*, std::vector<Node*>> getChildNodes();

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
     * Returns whether this node receives input or not.
     *
     * @return Whether this node receives input or not.
     */
    bool isReceivingInput();

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
     * Called before this node or one of the node's parents (in the parent hierarchy)
     * is about to be detached from the current parent node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     *
     * @remark If this node is being detached from its parent @ref getParentNode will return
     * `nullptr` after this function is finished.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingDetached `true` if this node is being detached from its parent,
     * `false` if some node in the parent hierarchy is being detached from its parent.
     */
    virtual void onBeforeDetachedFromParent(bool bThisNodeBeingDetached) {}

    /**
     * Called after this node or one of the node's parents (in the parent hierarchy)
     * was attached to a new parent node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    virtual void onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {}

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
     * Called when the window received mouse movement.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param xOffset  Mouse X movement delta in pixels (plus if moved to the right,
     * minus if moved to the left).
     * @param yOffset  Mouse Y movement delta in pixels (plus if moved up,
     * minus if moved down).
     */
    virtual void onMouseMove(double xOffset, double yOffset) {}

    /**
     * Called when the window receives mouse scroll movement.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iOffset Movement offset.
     */
    virtual void onMouseScrollMove(int iOffset) {}

    /**
     * Determines if the @ref onBeforeNewFrame should be called each frame or not.
     *
     * @remark Safe to call any time (while spawned/despawned).
     * @remark Nodes are not called every frame by default.
     *
     * @param bEnable `true` to enable @ref onBeforeNewFrame, `false` to disable.
     */
    void setIsCalledEveryFrame(bool bEnable);

    /**
     * Determines if the input related functions, such as @ref onMouseMove, @ref onMouseScrollMove,
     * @ref onInputActionEvent and @ref onInputAxisEvent will be called or not.
     *
     * @remark Safe to call any time (while spawned/despawned).
     * @remark Nodes do not receive input by default.
     *
     * @param bEnable Whether the input function should be enabled or not.
     */
    void setIsReceivingInput(bool bEnable);

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
     * Returns map of action events that this node is bound to (must be used with mutex).
     * Bound callbacks will be automatically called when an action event is triggered.
     *
     * @remark Input events will be only triggered if the node is spawned.
     * @remark Input events will not be called if @ref setIsReceivingInput was not enabled.
     * @remark Only events in GameInstance's InputManager (@ref GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     *
     * Example:
     * @code
     * const auto iForwardActionId = 0;
     * const auto pActionEvents = getActionEventBindings();
     *      * std::scoped_lock guard(pActionEvents->first);
     * pActionEvents->second[iForwardActionId] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
     *     moveForward(modifiers, bIsPressedDown);
     * };
     * @endcode
     *
     * @return Bound action events.
     */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>*
    getActionEventBindings();

    /**
     * Returns map of axis events that this node is bound to (must be used with mutex).
     * Bound callbacks will be automatically called when an axis event is triggered.
     *
     * @remark Input events will be only triggered if the node is spawned.
     * @remark Input events will not be called if @ref setIsReceivingInput was not enabled.
     * @remark Only events in GameInstance's InputManager (@ref GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     * @remark Input parameter is a value in range [-1.0f; 1.0f] that describes input.
     *
     * Example:
     * @code
     * const auto iForwardAxisEventId = 0;
     * const auto pAxisEvents = getAxisEventBindings();
     *      * std::scoped_lock guard(pAxisEvents->first);
     * pAxisEvents->second[iForwardAxisEventId] = [&](KeyboardModifiers modifiers, float input) {
     *     moveForward(modifiers, input);
     * };
     * @endcode
     *
     * @return Bound action events.
     */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>*
    getAxisEventBindings();

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
     * Calls @ref onAfterAttachedToNewParent on this node and all of its child nodes.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    void notifyAboutAttachedToNewParent(bool bThisNodeBeingAttached);

    /**
     * Calls @ref onBeforeDetachedFromParent on this node and all of its child nodes.
     *
     * @param bThisNodeBeingDetached `true` if this node is being detached from its parent,
     * `false` if some node in the parent hierarchy is being detached from its parent.
     */
    void notifyAboutDetachingFromParent(bool bThisNodeBeingDetached);

    /**
     * Called when a window that owns this game instance receives user
     * input and the input key exists as an action event in the InputManager.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iActionId      Unique ID of the input action event (from input manager).
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onInputActionEvent(unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called when a window that owns this game instance receives user
     * input and the input key exists as an axis event in the InputManager.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param iAxisEventId  Unique ID of the input axis event (from input manager).
     * @param modifiers     Keyboard modifier keys.
     * @param input         A value in range [-1.0f; 1.0f] that describes input.
     */
    void onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input);

    /**
     * Asks this node's parent and goes up the node hierarchy
     * up to the root node if needed to find a valid pointer to world.
     *
     * @return Valid world pointer.
     */
    World* askParentsAboutWorldPointer();

    /** Map of action events that this node is bound to. Must be used with mutex. */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>
        mtxBindedActionEvents;

    /** Map of axis events that this node is bound to. Must be used with mutex. */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>
        mtxBindedAxisEvents;

    /** Attached child nodes. */
    std::pair<std::recursive_mutex, std::vector<std::unique_ptr<Node>>> mtxChildNodes;

    /** Attached parent node. */
    std::pair<std::recursive_mutex, Node*> mtxParentNode;

    /** Determines if the @ref onBeforeNewFrame should be called each frame or not. */
    std::pair<std::recursive_mutex, bool> mtxIsCalledEveryFrame;

    /**
     * Determines if the input related functions, such as @ref onMouseMove, @ref onMouseScrollMove,
     * @ref onInputActionEvent and @ref onInputAxisEvent will be called or not.
     */
    std::pair<std::recursive_mutex, bool> mtxIsReceivingInput;

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
