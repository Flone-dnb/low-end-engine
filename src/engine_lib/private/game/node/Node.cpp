#include "game/node/Node.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/node/SpatialNode.h"
#include "io/Logger.h"

// External.
#include "nameof.hpp"

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

namespace {
    constexpr std::string_view sTypeGuid = "a70f1233-ad98-4686-a987-aeb916804369";
}

std::string Node::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string Node::getTypeGuid() const { return sTypeGuid.data(); }

size_t Node::getAliveNodeCount() { return iTotalAliveNodeCount.load(); }

TypeReflectionInfo Node::getReflectionInfo() {
    ReflectedVariables variables;

    variables.strings[NAMEOF_MEMBER(&Node::sNodeName).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<Node*>(pThis)->setNodeName(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return std::string(reinterpret_cast<Node*>(pThis)->getNodeName());
        }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(Node).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<Node>(); },
        std::move(variables));
}

void Node::unsafeDetachFromParentAndDespawn() {
    Logger::get().info(std::format("detaching and despawning the node \"{}\"", getNodeName()));
    Logger::get().flushToDisk(); // flush in case if we crash later

    std::unique_ptr<Node> pSelf;

    {
        if (getWorldRootNodeWhileSpawned() == this) [[unlikely]] {
            Error::showErrorAndThrowException(
                "instead of despawning world's root node, create/replace world using GameInstance "
                "functions, this would destroy the previous world with all nodes");
        }

        // Detach from parent.
        std::scoped_lock guard(mtxIsSpawned.first, mtxParentNode.first);

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
                Logger::get().error(
                    std::format(
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
    if (iNodeCount + 1 == (std::numeric_limits<size_t>::max)()) [[unlikely]] {
        Logger::get().warn(
            std::format(
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
        Error::showErrorAndThrowException("this function should not be called while the node is spawned");
    }

    this->tickGroup = tickGroup;
}

GameInstance* Node::getGameInstanceWhileSpawned() {
    std::scoped_lock guard(mtxIsSpawned.first);

    // Make sure the node is spawned.
    if (!mtxIsSpawned.second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "this function should not be called while the node is not spawned (called from node \"{}\")",
                sNodeName));
    }

    if (pWorldWeSpawnedIn == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "spawned node \"{}\" attempted to request the game instance but world is nullptr",
                sNodeName));
    }

    return pWorldWeSpawnedIn->pGameManager->getGameInstance();
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>&
Node::getActionEventBindings() {
    return mtxBindedActionEvents;
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>&
Node::getAxisEventBindings() {
    return mtxBindedAxisEvents;
}

std::recursive_mutex& Node::getSpawnDespawnMutex() { return mtxIsSpawned.first; }

void Node::spawn() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (mtxIsSpawned.second) [[unlikely]] {
        Logger::get().warn(
            std::format(
                "an attempt was made to spawn already spawned node \"{}\", ignoring this operation",
                getNodeName()));
        return;
    }

    // Initialize world.
    pWorldWeSpawnedIn = askParentsAboutWorldPointer();

    // Get unique ID.
    iNodeId = iAvailableNodeId.fetch_add(1);
    if (iNodeId.value() + 1 == (std::numeric_limits<size_t>::max)()) [[unlikely]] {
        Logger::get().warn(
            std::format(
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
        Logger::get().warn(
            std::format(
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
        Error::showErrorAndThrowException(
            std::format(
                "node \"{}\" can't find a pointer to a valid world instance because "
                "there is no parent node",
                getNodeName()));
    }

    return mtxParentNode.second->askParentsAboutWorldPointer();
}

void Node::getNodeWorldLocationRotationScale(
    Node* pNode, glm::vec3& worldLocation, glm::vec3& worldRotation, glm::vec3& worldScale) {
    worldLocation = glm::vec3(0.0F, 0.0F, 0.0F);
    worldRotation = glm::vec3(0.0F, 0.0F, 0.0F);
    worldScale = glm::vec3(1.0F, 1.0F, 1.0F);

    const auto pSpatialNode = dynamic_cast<SpatialNode*>(pNode);
    if (pSpatialNode != nullptr) {
        worldLocation = pSpatialNode->getWorldLocation();
        worldRotation = pSpatialNode->getWorldRotation();
        worldScale = pSpatialNode->getWorldScale();
    }
}

void Node::applyAttachmentRuleForNode(
    Node* pNode,
    AttachmentRule locationRule,
    const glm::vec3& worldLocationBeforeAttachment,
    AttachmentRule rotationRule,
    const glm::vec3& worldRotationBeforeAttachment,
    AttachmentRule scaleRule,
    const glm::vec3& worldScaleBeforeAttachment) {
    // Cast type.
    const auto pSpatialNode = dynamic_cast<SpatialNode*>(pNode);
    if (pSpatialNode == nullptr) {
        return;
    }

    pSpatialNode->applyAttachmentRule(
        locationRule,
        worldLocationBeforeAttachment,
        rotationRule,
        worldRotationBeforeAttachment,
        scaleRule,
        worldScaleBeforeAttachment);
}
