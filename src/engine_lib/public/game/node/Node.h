#pragma once

// Standard.
#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>
#include <memory>
#include <variant>

// Custom.
#include "io/Serializable.h"
#include "input/KeyboardButton.hpp"
#include "game/node/NodeTickGroup.hpp"
#include "math/GLMath.hpp"
#include "misc/Error.h"
#include "misc/ReflectedTypeDatabase.h"
#include "misc/Profiler.hpp"

class GameInstance;
class World;

/**
 * Base class for game entities, allows being spawned in the world, attaching child nodes
 * or being attached to some parent node.
 */
class Node : public Serializable {
    // World is able to spawn root node.
    friend class World;

    // GameManager will propagate functions to all nodes in the world such as `onBeforeNewFrame`.
    friend class GameManager;

    // Importer can take ownership of child nodes.
    friend class GltfImporter;

public:
    /**
     * Defines how location, rotation or scale of a node being attached as a child node
     * should change after the attachment process (after `onAfterAttachedToNewParent` was called).
     */
    enum class AttachmentRule : uint8_t {
        KEEP_RELATIVE, //< After the new child node was attached, its relative location/rotation/scale will
                       // stay the same, but world location/rotation/scale might change.
        KEEP_WORLD,    //< After the new child node was attached, its relative location/rotation/scale will be
                       // recalculated so that its world location/rotation/scale will stay the same (as before
                       // attached).
    };

    /** Callbacks for a bound input action event. */
    struct ActionEventCallbacks {
        /** Optional.  Called when the action event is triggered because one of the bound buttons is pressed.
         */
        std::function<void(KeyboardModifiers)> onPressed;

        /** Optional. Called when the action event is stopped because all bound buttons are released (after
         * some was pressed). */
        std::function<void(KeyboardModifiers)> onReleased;
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

    virtual ~Node() override;

    /**
     * Returns the total amount of currently alive (allocated) nodes.
     *
     * @return Number of alive nodes right now.
     */
    static size_t getAliveNodeCount();

    /**
     * Returns value that the next spawned node will use as its ID.
     * Returned value is not "reserved" to you, this function only exists to view the next node ID.
     *
     * @return Node ID.
     */
    static size_t peekNextNodeId();

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Deserializes a node and all its child nodes (hierarchy information) from a file.
     *
     * @param pathToFile File to read a node tree from. The ".toml" extension will be added
     * automatically if not specified in the path.
     *
     * @return Error if something went wrong, otherwise pointer to the root node of the deserialized node
     * tree.
     */
    static std::variant<std::unique_ptr<Node>, Error>
    deserializeNodeTree(const std::filesystem::path& pathToFile);

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Serializes the node and all child nodes (hierarchy information will also be saved) into a file.
     * Node tree can later be deserialized using @ref deserializeNodeTree.
     *
     * @param pathToFile    File to write the node tree to. The ".toml" extension will be added
     * automatically if not specified in the path. If the specified file already exists it will be
     * overwritten.
     * @param bEnableBackup If 'true' will also use a backup (copy) file. @ref deserializeNodeTree can use
     * backup file if the original file does not exist. Generally you want to use
     * a backup file if you are saving important information, such as player progress,
     * other cases such as player game settings and etc. usually do not need a backup but
     * you can use it if you want.
     *
     * @remark Custom attributes, like in Serializable::serialize, are not available here
     * because they are used internally to store hierarchy and other information.
     *
     * @return Error if something went wrong, for example when found an unsupported for
     * serialization reflected field.
     */
    [[nodiscard]] std::optional<Error>
    serializeNodeTree(std::filesystem::path pathToFile, bool bEnableBackup);

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
     *
     * @param bDontLogMessage Specify `true` to not log a message about node being destroyed.
     */
    void unsafeDetachFromParentAndDespawn(bool bDontLogMessage = false);

    /**
     * Sets if this node (and node's child nodes) should be serialized as part of a node tree or not.
     *
     * @param bSerialize `true` to serialize, `false` ignore when serializing as part of a node tree.
     */
    void setSerialize(bool bSerialize);

    /**
     * Sets node's name.
     *
     * @param sName New name of this node.
     */
    void setNodeName(const std::string& sName);

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
     * @param pNode        Node to attach as a child. That node must node have a parent because
     * `this` node will take the ownership of the unique_ptr.
     *
     * @return Raw pointer to the node you passed in this function.
     */
    template <typename NodeType>
        requires std::derived_from<NodeType, Node>
    NodeType* addChildNode(std::unique_ptr<NodeType> pNode);

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
     * @param pNode        Node to attach as a child. That node must have a parent so that `this` node can
     * transfer the ownership of the node, otherwise an error message will be shown.
     *
     * @return Raw pointer to the node you passed in this function.
     */
    template <typename NodeType>
        requires std::derived_from<NodeType, Node>
    NodeType* addChildNode(NodeType* pNode);

    /**
     * Changes the position of a child node in the array of child nodes.
     *
     * @param iIndexFrom Index of the node to move.
     * @param iIndexTo   Index to insert the node to.
     */
    void changeChildNodePositionIndex(size_t iIndexFrom, size_t iIndexTo);

    /**
     * Returns world's root node.
     *
     * @warning If used while not spawned an error will be shown.
     *
     * @return Valid pointer to root node.
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
     * @ref isReceivingInput but without locking the mutex. Should be slightly faster than the original
     * version of the function if used multiple times in a single frame but you need to guarantee that there
     * won't be threading problems.
     *
     * @return Whether this node receives input or not.
     */
    bool isReceivingInputUnsafe() const { return mtxIsReceivingInput.second; }

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
    std::optional<size_t> getNodeId() const { return iNodeId; }

    /**
     * Tells whether or not this node (and node's child nodes) will be serialized as part of a node tree.
     *
     * @return `false` if this node and its child nodes will be ignored when being serialized as part
     * of a node tree, `true` otherwise.
     */
    bool isSerialized() const { return bSerialize; }

    /**
     * Returns the tick group this node resides in.
     *
     * @return Tick group the node is using.
     */
    TickGroup getTickGroup() const;

    /**
     * Returns world that this node is spawned in.
     *
     * @warning Do not delete (free) returned pointer.
     * @warning Shows an error if not spawned.
     *
     * @return Always valid pointer to world.
     */
    World* getWorldWhileSpawned() const;

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
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
     * (before executing your logic) to execute parent's logic.
     *
     * @remark Generally you might want to prefer to use @ref onSpawning, this function
     * is mostly used to do some logic related to child nodes after all child nodes were spawned
     * (for example if you have a camera child node you can make it active in this function).
     */
    virtual void onChildNodesSpawned() {}

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
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
     * (before executing your logic) to execute parent's logic.
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
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    virtual void onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {}

    /**
     * Called after some child node was attached to this node.
     *
     * @remark Called after @ref onAfterAttachedToNewParent is called on child nodes.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) {}

    /**
     * Called after some child node was detached from this node.
     *
     * @param pDetachedDirectChild Direct child node that was detached (child of this node, not a child of
     * some child node).
     */
    virtual void onAfterDirectChildDetached(Node* pDetachedDirectChild) {}

    /**
     * Called before a new frame is rendered.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic (if there is any).
     *
     * @remark This function is disabled by default, use @ref setIsCalledEveryFrame to enable it.
     * @remark This function will only be called while this node is spawned.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) {}

    /**
     * Called after a child node changed its position in the array of child nodes.
     *
     * @param iIndexFrom Moved from.
     * @param iIndexTo   Moved to.
     */
    virtual void onAfterChildNodePositionChanged(size_t iIndexFrom, size_t iIndexTo) {}

    /**
     * Called after the node changed its "receiving input" state (while spawned).
     *
     * @param bEnabledNow `true` if node enabled input, `false` if disabled.
     */
    virtual void onChangedReceivingInputWhileSpawned(bool bEnabledNow) {}

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
     * Returns map of action events that this node is bound to.
     * Bound callbacks will be automatically called when an action event is triggered.
     *
     * @remark Input events will be only triggered if the node is spawned.
     * @remark Input events will not be called if @ref setIsReceivingInput was not enabled.
     * @remark Only events from GameInstance's InputManager (@ref GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     *
     * Example:
     * @code
     * const auto iJumpActionId = 0;
     * getActionEventBindings()[iJumpActionId] =
     *     ActionEventCallbacks{.onPressed = [&](KeyboardModifiers modifiers) { jump(); }};
     * @endcode
     *
     * @return Bound action events.
     */
    std::unordered_map<unsigned int, ActionEventCallbacks>& getActionEventBindings() {
        return boundActionEvents;
    }

    /**
     * Returns map of axis events that this node is bound to.
     * Bound callbacks will be automatically called when an axis event is triggered.
     *
     * @remark Input events will be only triggered if the node is spawned.
     * @remark Input events will not be called if @ref setIsReceivingInput was not enabled.
     * @remark Only events from GameInstance's InputManager (@ref GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     * @remark Input parameter is a value in range [-1.0f; 1.0f] that describes input.
     *
     * Example:
     * @code
     * const auto iForwardAxisEventId = 0;
     * getAxisEventBindings()[iForwardAxisEventId] =
     *     [&](KeyboardModifiers modifiers, float input) { moveForward(input); };
     * @endcode
     *
     * @return Bound action events.
     */
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>& getAxisEventBindings() {
        return boundAxisEvents;
    }

    /**
     * Returns mutex that is generally used to protect/prevent spawning/despawning.
     *
     * @return Mutex.
     */
    std::recursive_mutex& getSpawnDespawnMutex();

private:
    /** Information about an object to be serialized. */
    struct SerializableObjectInformationWithUniquePtr {
        /**
         * Initialized object information for serialization.
         *
         * @param info                    Info used for serialization.
         * @param pOptionalOriginalObject If @ref info has a valid (deserialized) original object then this
         * unique_ptr owns the memory for that original object.
         */
        SerializableObjectInformationWithUniquePtr(
            SerializableObjectInformation&& info, std::unique_ptr<Node>&& pOptionalOriginalObject)
            : info(info), pOptionalOriginalObject(std::move(pOptionalOriginalObject)) {}

        /** Info used for serialization. */
        SerializableObjectInformation info;

        /**
         * If @ref info has a valid (deserialized) original object then this unique_ptr owns the memory
         * for that original object.
         */
        std::unique_ptr<Node> pOptionalOriginalObject;
    };

    /**
     * Returns node's world location, rotation and scale.
     *
     * @remark This function exists because @ref addChildNode is implemented in the header but
     * we can't include spatial node's header in this header as it would create a recursive include.
     *
     * @param pNode         If spacial node (or derived) returns its world location, rotation and scale,
     * otherwise default parameters.
     * @param worldLocation Node's world location.
     * @param worldRotation Node's world rotation.
     * @param worldScale    Node's world scale.
     */
    static void getNodeWorldLocationRotationScale(
        Node* pNode, glm::vec3& worldLocation, glm::vec3& worldRotation, glm::vec3& worldScale);

    /**
     * Called by `Node` class after we have attached to a new parent node and
     * now need to apply attachment rules based on this new parent node.
     *
     * @remark This function exists because @ref addChildNode is implemented in the header but
     * we can't include spatial node's header in this header as it would create a recursive include.
     *
     * @param pNode                         If spatial node that applies the rule, otherwise does nothing.
     * @param locationRule                  Defines how location should change.
     * @param worldLocationBeforeAttachment World location of this node before being attached.
     * @param rotationRule                  Defines how rotation should change.
     * @param worldRotationBeforeAttachment World rotation of this node before being attached.
     * @param scaleRule                     Defines how scale should change.
     * @param worldScaleBeforeAttachment    World scale of this node before being attached.
     */
    static void applyAttachmentRuleForNode(
        Node* pNode,
        Node::AttachmentRule locationRule,
        const glm::vec3& worldLocationBeforeAttachment,
        Node::AttachmentRule rotationRule,
        const glm::vec3& worldRotationBeforeAttachment,
        Node::AttachmentRule scaleRule,
        const glm::vec3& worldScaleBeforeAttachment);

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
     *
     * @return Raw pointer to the node you passed in this function.
     */
    template <typename NodeType>
        requires std::derived_from<NodeType, Node>
    NodeType* addChildNode(
        std::variant<std::unique_ptr<NodeType>, NodeType*> node,
        AttachmentRule locationRule,
        AttachmentRule rotationRule,
        AttachmentRule scaleRule);

    /**
     * Locks @ref mtxChildNodes mutex for self and recursively for all children.
     * After a node with children was locked this makes the whole node tree to be
     * frozen (hierarchy can't be changed).
     * Use @ref unlockChildren for unlocking the tree.
     */
    void lockChildren();

    /**
     * Unlocks @ref mtxChildNodes mutex for self and recursively for all children.
     * After a node with children was unlocked this makes the whole node tree to be
     * unfrozen (hierarchy can be changed as usual).
     */
    void unlockChildren();

    /**
     * Collects and returns information for serialization for self and all child nodes.
     *
     * @param pathToSerializeTo Path that this node will be serialized to.
     * @param iId       ID for serialization to use (will be incremented).
     * @param iParentId Parent's serialization ID (if this node has a parent and it will also
     * be serialized).
     *
     * @return Error if something went wrong, otherwise an array of collected information
     * that can be serialized.
     */
    std::variant<std::vector<SerializableObjectInformationWithUniquePtr>, Error>
    getInformationForSerialization(
        const std::filesystem::path& pathToSerializeTo, size_t& iId, std::optional<size_t> iParentId);

    /**
     * Checks if this node and all child nodes were deserialized from the same file
     * (i.e. checks if this node tree is located in one file).
     *
     * @param sPathRelativeToRes Path relative to the `res` directory to the file to check,
     * example: `game/test.toml`.
     *
     * @return `false` if this node or some child node(s) were deserialized from other file
     * or if some nodes we not deserialized previously, otherwise `true`.
     */
    bool isTreeDeserializedFromOneFile(const std::string& sPathRelativeToRes);

    /** Map of action events that this node is bound to. */
    std::unordered_map<unsigned int, ActionEventCallbacks> boundActionEvents;

    /** Map of axis events that this node is bound to. */
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>> boundAxisEvents;

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

    /**
     * Defines whether or not this node (and node's child nodes) should be serialized
     * as part of a node tree.
     */
    bool bSerialize = true;

    /** Name of the TOML key we use to serialize information about parent node. */
    static constexpr std::string_view sTomlKeyParentNodeId = "parent_node_id";

    /** Name of the TOML key we use to serialize information about position in parent's child node array. */
    static constexpr std::string_view sTomlKeyChildNodeArrayIndex = "child_node_index";

    /** Name of the TOML key we use to store a path to an external node tree. */
    static constexpr std::string_view sTomlKeyExternalNodeTreePath = "root_node_of_external_node_tree";
};

template <typename NodeType>
    requires std::derived_from<NodeType, Node>
inline NodeType* Node::addChildNode(std::unique_ptr<NodeType> pNode) {
    AttachmentRule locationRule = AttachmentRule::KEEP_RELATIVE;
    AttachmentRule rotationRule = AttachmentRule::KEEP_RELATIVE;
    AttachmentRule scaleRule = AttachmentRule::KEEP_RELATIVE;
    if (pNode->isSpawned()) {
        locationRule = AttachmentRule::KEEP_WORLD;
        rotationRule = AttachmentRule::KEEP_WORLD;
        scaleRule = AttachmentRule::KEEP_WORLD;
    }

    return addChildNode<NodeType>(
        std::variant<std::unique_ptr<NodeType>, NodeType*>(std::move(pNode)),
        locationRule,
        rotationRule,
        scaleRule);
}

template <typename NodeType>
    requires std::derived_from<NodeType, Node>
inline NodeType* Node::addChildNode(NodeType* pNode) {
    AttachmentRule locationRule = AttachmentRule::KEEP_RELATIVE;
    AttachmentRule rotationRule = AttachmentRule::KEEP_RELATIVE;
    AttachmentRule scaleRule = AttachmentRule::KEEP_RELATIVE;
    if (pNode->isSpawned()) {
        locationRule = AttachmentRule::KEEP_WORLD;
        rotationRule = AttachmentRule::KEEP_WORLD;
        scaleRule = AttachmentRule::KEEP_WORLD;
    }

    return addChildNode<NodeType>(
        std::variant<std::unique_ptr<NodeType>, NodeType*>(pNode), locationRule, rotationRule, scaleRule);
}

template <typename NodeType>
    requires std::derived_from<NodeType, Node>
inline NodeType* Node::addChildNode(
    std::variant<std::unique_ptr<NodeType>, NodeType*> node,
    AttachmentRule locationRule,
    AttachmentRule rotationRule,
    AttachmentRule scaleRule) {
    PROFILE_FUNC
    PROFILE_ADD_SCOPE_TEXT(sNodeName.c_str(), sNodeName.size());

    // Save raw pointer for now.
    NodeType* pNode = nullptr;
    if (std::holds_alternative<NodeType*>(node)) {
        pNode = std::get<NodeType*>(node);
    } else {
        pNode = std::get<std::unique_ptr<NodeType>>(node).get();
    }

    // Save world rotation/location/scale for later use.
    glm::vec3 worldLocation = glm::vec3(0.0F, 0.0F, 0.0F);
    glm::vec3 worldRotation = glm::vec3(0.0F, 0.0F, 0.0F);
    glm::vec3 worldScale = glm::vec3(1.0F, 1.0F, 1.0F);
    getNodeWorldLocationRotationScale(pNode, worldLocation, worldRotation, worldScale);

    std::scoped_lock spawnGuard(mtxIsSpawned.first);

    // Make sure the specified node is valid.
    if (pNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an attempt was made to attach a nullptr node to the \"{}\" node", getNodeName()));
    }

    // Make sure the specified node is not `this`.
    if (pNode == this) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an attempt was made to attach the \"{}\" node to itself", getNodeName()));
    }

    std::scoped_lock guard(mtxChildNodes.first);

    // Make sure the specified node is not our direct child.
    for (const auto& pChildNode : mtxChildNodes.second) {
        if (pChildNode.get() == pNode) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "an attempt was made to attach the \"{}\" node to the \"{}\" node but it's already "
                "a direct child node of \"{}\"",
                pNode->getNodeName(),
                getNodeName(),
                getNodeName()));
        }
    }

    // Make sure the specified node is not our parent.
    if (pNode->isParentOf(this)) {
        Error::showErrorAndThrowException(std::format(
            "an attempt was made to attach the \"{}\" node to the node \"{}\", "
            "but the first node is a parent of the second node, "
            "aborting this operation",
            pNode->getNodeName(),
            getNodeName()));
    }

    // Prepare unique_ptr to add to our "child nodes" array.
    std::unique_ptr<Node> pNodeToAttachToThis = nullptr;

    // Check if this node is already attached to some node.
    std::scoped_lock parentGuard(pNode->mtxParentNode.first);
    if (pNode->mtxParentNode.second != nullptr) {
        // Make sure we were given a raw pointer.
        if (!std::holds_alternative<NodeType*>(node)) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected a raw pointer for the node \"{}\"", pNode->getNodeName()));
        }

        // Check if we are already this node's parent.
        if (pNode->mtxParentNode.second == this) {
            Error::showErrorAndThrowException(std::format(
                "an attempt was made to attach the \"{}\" node to its parent again", pNode->getNodeName()));
        }

        // Notify start of detachment.
        pNode->notifyAboutDetachingFromParent(true);

        // Remove node from parent's "children" array.
        std::scoped_lock parentsChildrenGuard(pNode->mtxParentNode.second->mtxChildNodes.first);

        auto& parentsChildren = pNode->mtxParentNode.second->mtxChildNodes.second;
        for (auto it = parentsChildren.begin(); it != parentsChildren.end(); ++it) {
            if ((*it).get() == pNode) {
                pNodeToAttachToThis = std::move(*it);
                parentsChildren.erase(it);
                break;
            }
        }

        if (pNodeToAttachToThis == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "the node \"{}\" has parent node \"{}\" but parent node does not have this node in its "
                "array "
                "of child nodes",
                pNode->getNodeName(),
                pNode->mtxParentNode.second->getNodeName()));
        }
    } else {
        if (std::holds_alternative<NodeType*>(node)) [[unlikely]] {
            // We need a unique_ptr.
            Error::showErrorAndThrowException(std::format(
                "expected a unique pointer for the node \"{}\" because it does not have a parent",
                pNode->getNodeName()));
        }

        pNodeToAttachToThis = std::move(std::get<std::unique_ptr<NodeType>>(node));
        if (pNodeToAttachToThis == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("unexpected nullptr");
        }
    }

    // Add node to our children array.
    pNode->mtxParentNode.second = this;
    mtxChildNodes.second.push_back(std::move(pNodeToAttachToThis));

    // First notify the children (here, SpatialNode will save a pointer to the first SpatialNode in the parent
    // chain and will use it in `setWorld...` operations).
    pNode->notifyAboutAttachedToNewParent(true);

    // Now after the SpatialNode did its `onAfterAttached` logic `setWorld...` calls when applying
    // attachment rule will work.
    // Apply attachment rule (if possible).
    applyAttachmentRuleForNode(
        pNode, locationRule, worldLocation, rotationRule, worldRotation, scaleRule, worldScale);

    // don't unlock node's parent lock here yet, still doing some logic based on the new parent

    // Spawn/despawn node if needed.
    if (isSpawned() && !pNode->isSpawned()) {
        // Spawn node.
        pNode->spawn();
    } else if (!isSpawned() && pNode->isSpawned()) {
        // Despawn node.
        pNode->despawn();
    }

    // After children were notified - notify the parent.
    onAfterNewDirectChildAttached(pNode);

    return pNode;
}

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
