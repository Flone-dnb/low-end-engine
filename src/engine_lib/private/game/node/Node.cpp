#include "game/node/Node.h"

// Custom.
#include "io/Logger.h"
#include "misc/Error.h"
#include "game/World.h"
#include "game/GameManager.h"

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

Node::Node() : Node("Node") {}

Node::Node(std::string_view sName) : sNodeName(sName) {
    mtxIsCalledEveryFrame.second = false;
    mtxIsReceivingInput.second = false;
}

void Node::setNodeName(const std::string& sName) { this->sNodeName = sName; }

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
