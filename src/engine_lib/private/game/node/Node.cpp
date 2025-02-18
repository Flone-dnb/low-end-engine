#include "game/node/Node.h"

// Custom.
#include "io/Logger.h"
#include "misc/Error.h"
#include "game/World.h"
#include "game/GameManager.h"
#include "game/node/SpatialNode.h"

/** Total amount of alive nodes. */
static std::atomic<size_t> iTotalAliveNodeCount{0};

/**
 * Stores the next node ID that can be used.
 *
 * @warning Don't reset (zero) this value even if no node exists as we will never hit type limit
 * but resetting this value might cause unwanted behavior.
 *
 * @remark Used in World to quickly and safely check if some node is spawned or not
 * (for example it's used in callbacks).
 */
static std::atomic<size_t> iAvailableNodeId{0};

size_t Node::getAliveNodeCount() { return iTotalAliveNodeCount.load(); }

void Node::addChildNode(
    std::variant<std::unique_ptr<Node>, Node*> node,
    AttachmentRule locationRule,
    AttachmentRule rotationRule,
    AttachmentRule scaleRule) {
    // Save raw pointer for now.
    Node* pNode = nullptr;
    if (std::holds_alternative<Node*>(node)) {
        pNode = std::get<Node*>(node);
    } else {
        pNode = std::get<std::unique_ptr<Node>>(node).get();
    }

    // Convert to spatial node for later use.
    SpatialNode* pSpatialNode = dynamic_cast<SpatialNode*>(pNode);

    // Save world rotation/location/scale for later use.
    glm::vec3 worldLocation = glm::vec3(0.0F, 0.0F, 0.0F);
    glm::vec3 worldRotation = glm::vec3(0.0F, 0.0F, 0.0F);
    glm::vec3 worldScale = glm::vec3(1.0F, 1.0F, 1.0F);
    if (pSpatialNode != nullptr) {
        worldLocation = pSpatialNode->getWorldLocation();
        worldRotation = pSpatialNode->getWorldRotation();
        worldScale = pSpatialNode->getWorldScale();
    }

    std::scoped_lock spawnGuard(mtxIsSpawned.first);

    // Make sure the specified node is valid.
    if (pNode == nullptr) [[unlikely]] {
        Logger::get().warn(std::format(
            "an attempt was made to attach a nullptr node to the \"{}\" node, aborting this "
            "operation",
            getNodeName()));
        return;
    }

    // Make sure the specified node is not `this`.
    if (pNode == this) [[unlikely]] {
        Logger::get().warn(std::format(
            "an attempt was made to attach the \"{}\" node to itself, aborting this "
            "operation",
            getNodeName()));
        return;
    }

    std::scoped_lock guard(mtxChildNodes.first);

    // Make sure the specified node is not our direct child.
    for (const auto& pChildNode : mtxChildNodes.second) {
        if (pChildNode.get() == pNode) [[unlikely]] {
            Logger::get().warn(std::format(
                "an attempt was made to attach the \"{}\" node to the \"{}\" node but it's already "
                "a direct child node of \"{}\", aborting this operation",
                pNode->getNodeName(),
                getNodeName(),
                getNodeName()));
            return;
        }
    }

    // Make sure the specified node is not our parent.
    if (pNode->isParentOf(this)) {
        Logger::get().error(std::format(
            "an attempt was made to attach the \"{}\" node to the node \"{}\", "
            "but the first node is a parent of the second node, "
            "aborting this operation",
            pNode->getNodeName(),
            getNodeName()));
        return;
    }

    // Prepare unique_ptr to add to our "child nodes" array.
    std::unique_ptr<Node> pNodeToAttachToThis = nullptr;

    // Check if this node is already attached to some node.
    std::scoped_lock parentGuard(pNode->mtxParentNode.first);
    if (pNode->mtxParentNode.second != nullptr) {
        // Make sure we were given a raw pointer.
        if (!std::holds_alternative<Node*>(node)) [[unlikely]] {
            Logger::get().error(
                std::format("expected a raw pointer for the node \"{}\"", pNode->getNodeName()));
            return;
        }

        // Check if we are already this node's parent.
        if (pNode->mtxParentNode.second == this) {
            Logger::get().warn(std::format(
                "an attempt was made to attach the \"{}\" node to its parent again, "
                "aborting this operation",
                pNode->getNodeName()));
            return;
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
            Logger::get().error(std::format(
                "the node \"{}\" has parent node \"{}\" but parent node does not have this node in its array "
                "of child nodes",
                pNode->getNodeName(),
                pNode->mtxParentNode.second->getNodeName()));
            return;
        }
    } else {
        if (std::holds_alternative<Node*>(node)) [[unlikely]] {
            // We need a unique_ptr.
            Logger::get().error(std::format(
                "expected a unique pointer for the node \"{}\" because it does not have a parent",
                pNode->getNodeName()));
            return;
        }

        pNodeToAttachToThis = std::move(std::get<std::unique_ptr<Node>>(node));
        if (pNodeToAttachToThis == nullptr) [[unlikely]] {
            Logger::get().error("unexpected nullptr");
            return;
        }
    }

    // Add node to our children array.
    pNode->mtxParentNode.second = this;
    mtxChildNodes.second.push_back(std::move(pNodeToAttachToThis));

    // The specified node is not attached.

    // Notify the node (here, SpatialNode will save a pointer to the first SpatialNode in the parent
    // chain and will use it in `setWorld...` operations).
    pNode->notifyAboutAttachedToNewParent(true);

    // Now after the SpatialNode did its `onAfterAttached` logic `setWorld...` calls when applying
    // attachment rule will work.
    // Apply attachment rule (if possible).
    if (pSpatialNode != nullptr) {
        pSpatialNode->applyAttachmentRule(
            locationRule, worldLocation, rotationRule, worldRotation, scaleRule, worldScale);
    }

    // don't unlock node's parent lock here yet, still doing some logic based on the new parent

    // Spawn/despawn node if needed.
    if (isSpawned() && !pNode->isSpawned()) {
        // Spawn node.
        pNode->spawn();
    } else if (!isSpawned() && pNode->isSpawned()) {
        // Despawn node.
        pNode->despawn();
    }
}

void Node::unsafeDetachFromParentAndDespawn() {
    Logger::get().info(std::format("detaching and despawning the node \"{}\"", getNodeName()));
    Logger::get().flushToDisk(); // flush in case if we crash later

    if (getWorldRootNodeWhileSpawned() == this) [[unlikely]] {
        Error error("instead of despawning world's root node, create/replace world using GameInstance "
                    "functions, this would destroy the previous world with all nodes");
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }

    // Detach from parent.
    std::scoped_lock guard(mtxIsSpawned.first, mtxParentNode.first);
    std::unique_ptr<Node> pSelf;

    if (mtxParentNode.second != nullptr) {
        // Notify.
        notifyAboutDetachingFromParent(true);

        // Remove this node from parent's children array.
        std::scoped_lock parentChildGuard(mtxParentNode.second->mtxChildNodes.first);
        auto& parentChildren = mtxParentNode.second->mtxChildNodes.second;
        for (auto it = parentChildren.begin(); it != parentChildren.end(); ++it) {
            if ((*it).get() == this) {
                pSelf = std::move(*it);
                parentChildren.erase(it);
                break;
            }
        }

        if (pSelf == nullptr) [[unlikely]] {
            Logger::get().error(std::format(
                "node \"{}\" has a parent node but parent's children array "
                "does not contain this node.",
                getNodeName()));
        }

        // Clear parent.
        mtxParentNode.second = nullptr;
    }

    // don't unlock mutexes yet

    if (mtxIsSpawned.second) {
        // Despawn.
        despawn();
    }

    // Delete self.
    pSelf = nullptr;
}

Node::Node() : Node("Node") {}

Node::Node(std::string_view sName) : sNodeName(sName) {
    mtxIsCalledEveryFrame.second = false;
    mtxIsReceivingInput.second = false;

    // Increment total node counter.
    const size_t iNodeCount = iTotalAliveNodeCount.fetch_add(1) + 1;
#undef max
    if (iNodeCount + 1 == std::numeric_limits<size_t>::max()) [[unlikely]] {
        Logger::get().warn(std::format(
            "\"total alive nodes\" counter is at its maximum value: {}, another new node will cause "
            "an overflow",
            iNodeCount + 1));
    }
}

Node::~Node() {
    // Decrement total node counter.
    iTotalAliveNodeCount.fetch_sub(1);
}

void Node::setNodeName(const std::string& sName) { this->sNodeName = sName; }

Node* Node::getWorldRootNodeWhileSpawned() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (pWorldWeSpawnedIn == nullptr) {
        return nullptr;
    }

    return pWorldWeSpawnedIn->getRootNode();
}

std::pair<std::recursive_mutex*, Node*> Node::getParentNode() {
    return {&mtxParentNode.first, mtxParentNode.second};
}

std::pair<std::recursive_mutex*, std::vector<Node*>> Node::getChildNodes() {
    std::scoped_lock guard(mtxChildNodes.first);

    // Convert array of unique_ptr to raw pointer array.
    std::vector<Node*> vChildNodes;
    vChildNodes.reserve(mtxChildNodes.second.size());

    for (const auto& pNode : mtxChildNodes.second) {
        vChildNodes.push_back(pNode.get());
    }

    return {&mtxChildNodes.first, std::move(vChildNodes)};
}

bool Node::isCalledEveryFrame() {
    std::scoped_lock guard(mtxIsCalledEveryFrame.first);
    return mtxIsCalledEveryFrame.second;
}

bool Node::isReceivingInput() {
    std::scoped_lock guard(mtxIsReceivingInput.first);
    return mtxIsReceivingInput.second;
}

bool Node::isSpawned() {
    std::scoped_lock guard(mtxIsSpawned.first);
    return mtxIsSpawned.second;
}

bool Node::isParentOf(Node* pNode) {
    std::scoped_lock guard(mtxChildNodes.first);

    // See if the specified node is in our child tree.
    for (const auto& pChildNode : mtxChildNodes.second) { // NOLINT
        if (pChildNode.get() == pNode) {
            return true;
        }

        const auto bIsChild = pChildNode->isParentOf(pNode);
        if (!bIsChild) {
            continue;
        }

        return true;
    }

    return false;
}

bool Node::isChildOf(Node* pNode) {
    std::scoped_lock guard(mtxParentNode.first);

    // Check if we have a parent.
    if (mtxParentNode.second == nullptr) {
        return false;
    }

    if (mtxParentNode.second == pNode) {
        return true;
    }

    return mtxParentNode.second->isChildOf(pNode);
}

std::string_view Node::getNodeName() const { return sNodeName; }

std::optional<size_t> Node::getNodeId() const { return iNodeId; }

TickGroup Node::getTickGroup() const { return tickGroup; }

void Node::setIsCalledEveryFrame(bool bEnable) {
    std::scoped_lock guard(mtxIsSpawned.first, mtxIsCalledEveryFrame.first);

    // Make sure the value is indeed changed.
    if (bEnable == mtxIsCalledEveryFrame.second) {
        // Nothing to do.
        return;
    }

    // Change the setting.
    mtxIsCalledEveryFrame.second = bEnable;

    // Check if we are spawned.
    if (!mtxIsSpawned.second) {
        return;
    }

    // Notify the world.
    pWorldWeSpawnedIn->onSpawnedNodeChangedIsCalledEveryFrame(this);
}

void Node::setIsReceivingInput(bool bEnable) {
    std::scoped_lock guard(mtxIsSpawned.first, mtxIsReceivingInput.first);

    // Make sure the value is indeed changed.
    if (bEnable == mtxIsReceivingInput.second) {
        // Nothing to do.
        return;
    }

    // Change the setting.
    mtxIsReceivingInput.second = bEnable;

    // Check if we are spawned.
    if (!mtxIsSpawned.second) {
        return;
    }

    // Notify the world.
    pWorldWeSpawnedIn->onSpawnedNodeChangedIsReceivingInput(this);
}

void Node::setTickGroup(TickGroup tickGroup) {
    // Make sure the node is not spawned.
    std::scoped_lock guard(mtxIsSpawned.first);
    if (mtxIsSpawned.second) [[unlikely]] {
        Error error("this function should not be called while the node is spawned");
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }

    this->tickGroup = tickGroup;
}

GameInstance* Node::getGameInstanceWhileSpawned() {
    std::scoped_lock guard(mtxIsSpawned.first);

    // Make sure the node is spawned.
    if (!mtxIsSpawned.second) [[unlikely]] {
        Error error(std::format(
            "this function should not be called while the node is not spawned (called from node \"{}\")",
            sNodeName));
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }

    if (pWorldWeSpawnedIn == nullptr) [[unlikely]] {
        Error error(std::format(
            "spawned node \"{}\" attempted to request the game instance but world is nullptr", sNodeName));
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }

    return pWorldWeSpawnedIn->pGameManager->getGameInstance();
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>*
Node::getActionEventBindings() {
    return &mtxBindedActionEvents;
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>*
Node::getAxisEventBindings() {
    return &mtxBindedAxisEvents;
}

std::recursive_mutex* Node::getSpawnDespawnMutex() { return &mtxIsSpawned.first; }

void Node::spawn() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (mtxIsSpawned.second) [[unlikely]] {
        Logger::get().warn(std::format(
            "an attempt was made to spawn already spawned node \"{}\", ignoring this operation",
            getNodeName()));
        return;
    }

    // Initialize world.
    pWorldWeSpawnedIn = askParentsAboutWorldPointer();

    // Get unique ID.
    iNodeId = iAvailableNodeId.fetch_add(1);
#undef max
    if (iNodeId.value() + 1 == std::numeric_limits<size_t>::max()) [[unlikely]] {
        Logger::get().warn(std::format(
            "\"next available node ID\" is at its maximum value: {}, another spawned node will "
            "cause an overflow",
            iNodeId.value() + 1));
    }

    // Mark state.
    mtxIsSpawned.second = true;

    // Notify world in order for node ID to be registered before running custom user spawn logic.
    pWorldWeSpawnedIn->onNodeSpawned(this);

    // Do custom user spawn logic.
    onSpawning();

    // We spawn self first and only then child nodes.
    // This spawn order is required for some nodes and engine parts to work correctly.
    // With this spawn order we will not make "holes" in the world's node tree
    // (i.e. when node is spawned, node's parent is not spawned but parent's parent node is spawned).

    // Spawn children.
    {
        std::scoped_lock childGuard(mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            pChildNode->spawn();
        }
    }

    // Notify user code.
    onChildNodesSpawned();
}

void Node::despawn() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (!mtxIsSpawned.second) [[unlikely]] {
        Logger::get().warn(std::format(
            "an attempt was made to despawn already despawned node \"{}\", ignoring this operation",
            getNodeName()));
        return;
    }

    // Despawn children first.
    // This despawn order is required for some nodes and engine parts to work correctly.
    // With this despawn order we will not make "holes" in world's node tree
    // (i.e. when node is spawned, node's parent is not spawned but parent's parent node is spawned).
    {
        std::scoped_lock childGuard(mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            pChildNode->despawn();
        }
    }

    // Despawn self.
    onDespawning();

    // Mark state.
    mtxIsSpawned.second = false;

    // Notify world.
    pWorldWeSpawnedIn->onNodeDespawned(this);

    // Don't allow accessing world at this point.
    pWorldWeSpawnedIn = nullptr;
}

void Node::notifyAboutAttachedToNewParent(bool bThisNodeBeingAttached) {
    std::scoped_lock guard(mtxParentNode.first, mtxChildNodes.first);

    onAfterAttachedToNewParent(bThisNodeBeingAttached);

    for (const auto& pChildNode : mtxChildNodes.second) {
        pChildNode->notifyAboutAttachedToNewParent(false);
    }
}

void Node::notifyAboutDetachingFromParent(bool bThisNodeBeingDetached) {
    std::scoped_lock guard(mtxParentNode.first, mtxChildNodes.first);

    onBeforeDetachedFromParent(bThisNodeBeingDetached);

    for (const auto& pChildNode : mtxChildNodes.second) {
        pChildNode->notifyAboutDetachingFromParent(false);
    }
}

void Node::onInputActionEvent(unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxBindedActionEvents.first);

    // See if this action event is registered.
    const auto it = mtxBindedActionEvents.second.find(iActionId);
    if (it == mtxBindedActionEvents.second.end()) {
        return;
    }

    // Trigger user logic.
    it->second(modifiers, bIsPressedDown);
}

void Node::onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input) {
    std::scoped_lock guard(mtxBindedAxisEvents.first);

    // See if this axis event is registered.
    const auto it = mtxBindedAxisEvents.second.find(iAxisEventId);
    if (it == mtxBindedAxisEvents.second.end()) {
        return;
    }

    // Trigger user logic.
    it->second(modifiers, input);
}

World* Node::askParentsAboutWorldPointer() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (pWorldWeSpawnedIn != nullptr) {
        return pWorldWeSpawnedIn;
    }

    // Ask parent node for the valid world pointer.
    std::scoped_lock parentGuard(mtxParentNode.first);
    if (mtxParentNode.second == nullptr) [[unlikely]] {
        Error err(std::format(
            "node \"{}\" can't find a pointer to a valid world instance because "
            "there is no parent node",
            getNodeName()));
        err.showError();
        throw std::runtime_error(err.getFullErrorMessage());
    }

    return mtxParentNode.second->askParentsAboutWorldPointer();
}
