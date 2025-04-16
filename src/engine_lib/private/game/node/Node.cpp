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

std::variant<std::unique_ptr<Node>, Error>
Node::deserializeNodeTree(const std::filesystem::path& pathToFile) {
    // Deserialize all nodes.
    auto deserializeResult = Serializable::deserializeMultiple<Node>(pathToFile);
    if (std::holds_alternative<Error>(deserializeResult)) [[unlikely]] {
        auto error = std::get<Error>(deserializeResult);
        error.addCurrentLocationToErrorStack();
        return error;
    }
    auto vDeserializedInfo = std::get<std::vector<DeserializedObjectInformation<std::unique_ptr<Node>>>>(
        std::move(deserializeResult));

    // See if some node is external node tree.
    for (auto& nodeInfo : vDeserializedInfo) {
        // Find attribute that stores path to the external node tree file.
        const auto it = nodeInfo.customAttributes.find(sTomlKeyExternalNodeTreePath.data());
        if (it == nodeInfo.customAttributes.end()) {
            continue;
        }

        // Construct path to this external node tree.
        const auto pathToExternalNodeTree =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / it->second;
        if (!std::filesystem::exists(pathToExternalNodeTree)) [[unlikely]] {
            return Error(
                std::format(
                    "file storing external node tree \"{}\" does not exist",
                    pathToExternalNodeTree.string()));
        }

        // Deserialize external node tree.
        auto result = deserializeNodeTree(pathToExternalNodeTree);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(result);
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto pExternalRootNode = std::get<std::unique_ptr<Node>>(std::move(result));

        // Get child nodes of external node tree.
        const auto pMtxChildNodes = pExternalRootNode->getChildNodes();
        std::scoped_lock externalChildNodesGuard(*pMtxChildNodes.first);

        // Attach child nodes of this external root node to our node
        // (their data cannot be changed since when you reference an external node tree
        // you can only modify root node of the external node tree).
        while (!pMtxChildNodes.second.empty()) {
            // Use a "while" loop instead of a "for" loop because this "child nodes array" will be
            // modified each iteration (this array will be smaller and smaller with each iteration since
            // we are "removing" child nodes of some parent node), thus we avoid modifying the
            // array while iterating over it.
            nodeInfo.pObject->addChildNode(
                *pMtxChildNodes.second.begin(),
                AttachmentRule::KEEP_RELATIVE,
                AttachmentRule::KEEP_RELATIVE,
                AttachmentRule::KEEP_RELATIVE);
        }
    }

    // Sort all nodes by their ID. Prepare array of pairs: Node - Parent index.
    std::optional<size_t> optionalRootNodeIndex;
    std::vector<std::pair<std::unique_ptr<Node>, std::optional<size_t>>> vNodes(vDeserializedInfo.size());
    for (size_t i = 0; i < vDeserializedInfo.size(); i++) {
        auto& nodeInfo = vDeserializedInfo[i];
        bool bIsRootNode = false;

        // Check that this object has required attribute about parent ID.
        std::optional<size_t> iParentNodeId = {};
        const auto parentNodeAttributeIt = nodeInfo.customAttributes.find(sTomlKeyParentNodeId.data());
        if (parentNodeAttributeIt == nodeInfo.customAttributes.end()) {
            if (!optionalRootNodeIndex.has_value()) {
                bIsRootNode = true;
            } else [[unlikely]] {
                return Error(
                    std::format(
                        "found non root node \"{}\" that does not have a parent",
                        nodeInfo.pObject->getNodeName()));
            }
        } else {
            try {
                iParentNodeId = std::stoull(parentNodeAttributeIt->second);
            } catch (std::exception& exception) {
                return Error(
                    std::format(
                        "failed to convert attribute \"{}\" with value \"{}\" to integer, error: {}",
                        sTomlKeyParentNodeId,
                        parentNodeAttributeIt->second,
                        exception.what()));
            }

            // Check if this parent ID is outside of out array bounds.
            if (iParentNodeId.value() >= vNodes.size()) [[unlikely]] {
                return Error(
                    std::format(
                        "parsed parent node ID is outside of bounds: {} >= {}",
                        iParentNodeId.value(),
                        vNodes.size()));
            }
        }

        // Try to convert this node's ID to size_t.
        size_t iNodeId = 0;
        try {
            iNodeId = std::stoull(nodeInfo.sObjectUniqueId);
        } catch (std::exception& exception) {
            return Error(
                std::format(
                    "failed to convert ID \"{}\" to integer, error: {}",
                    nodeInfo.sObjectUniqueId,
                    exception.what()));
        }

        // Check if this ID is outside of our array bounds.
        if (iNodeId >= vNodes.size()) [[unlikely]] {
            return Error(std::format("parsed ID is outside of bounds: {} >= {}", iNodeId, vNodes.size()));
        }

        // Check if we already set a node in this index position.
        if (vNodes[iNodeId].first != nullptr) [[unlikely]] {
            return Error(std::format("parsed ID {} was already used by some other node", iNodeId));
        }

        // Save node.
        vNodes[iNodeId] = {std::move(nodeInfo.pObject), iParentNodeId};

        if (bIsRootNode) {
            optionalRootNodeIndex = iNodeId;
        }
    }

    // See if we found root node.
    if (!optionalRootNodeIndex.has_value()) [[unlikely]] {
        return Error("root node was not found");
    }

    // Build hierarchy using value from attribute.
    for (auto& [pNode, optionalParentIndex] : vNodes) {
        if (pNode == nullptr) [[unlikely]] {
            return Error("unexpected nullptr");
        }
        if (!optionalParentIndex.has_value()) {
            continue;
        }
        vNodes[*optionalParentIndex].first->addChildNode(
            std::move(pNode),
            AttachmentRule::KEEP_RELATIVE,
            AttachmentRule::KEEP_RELATIVE,
            AttachmentRule::KEEP_RELATIVE);
    }

    return std::move(vNodes[*optionalRootNodeIndex].first);
}

std::optional<Error> Node::serializeNodeTree(const std::filesystem::path& pathToFile, bool bEnableBackup) {
    // Self check: make sure this node is marked to be serialized.
    if (!bSerialize) [[unlikely]] {
        return Error(
            std::format(
                "node \"{}\" is marked to be ignored when serializing as part of a node tree but "
                "this node was explicitly requested to be serialized as a node tree",
                sNodeName));
    }

    lockChildren(); // make sure nothing is changed/deleted while we are serializing
    {
        // Collect information for serialization.
        size_t iId = 0;
        auto result = getInformationForSerialization(iId, {});
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(result);
            error.addCurrentLocationToErrorStack();
            return error;
        }
        const auto vOriginalNodesInfo =
            std::get<std::vector<SerializableObjectInformationWithUniquePtr>>(std::move(result));

        // Convert information array.
        std::vector<SerializableObjectInformation> vNodesInfo;
        for (const auto& info : vOriginalNodesInfo) {
            vNodesInfo.push_back(std::move(info.info));
        }

        // Serialize.
        const auto optionalError =
            Serializable::serializeMultiple(pathToFile, std::move(vNodesInfo), bEnableBackup);
        if (optionalError.has_value()) [[unlikely]] {
            auto err = optionalError.value();
            err.addCurrentLocationToErrorStack();
            return err;
        }
    }
    unlockChildren();

    return {};
}

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

void Node::setSerialize(bool bSerialize) { this->bSerialize = bSerialize; }

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

void Node::lockChildren() {
    mtxChildNodes.first.lock();
    for (const auto& pChildNode : mtxChildNodes.second) {
        pChildNode->lockChildren();
    }
}

void Node::unlockChildren() {
    mtxChildNodes.first.unlock();
    for (const auto& pChildNode : mtxChildNodes.second) {
        pChildNode->unlockChildren();
    }
}

std::variant<std::vector<Node::SerializableObjectInformationWithUniquePtr>, Error>
Node::getInformationForSerialization(size_t& iId, std::optional<size_t> iParentId) {
    // Prepare information about nodes.
    // Use custom attributes for storing hierarchy information.
    std::vector<SerializableObjectInformationWithUniquePtr> vNodesInfo;

    // Add self first.
    const size_t iMyId = iId;

    SerializableObjectInformation selfInfo(this, std::to_string(iId));

    // Add parent ID.
    if (iParentId.has_value()) {
        selfInfo.customAttributes[sTomlKeyParentNodeId.data()] = std::to_string(iParentId.value());
    }

    // Add original object (if was specified).
    bool bIncludeInformationAboutChildNodes = true;
    std::unique_ptr<Node> pOptionalOriginalObject;
    if (getPathDeserializedFromRelativeToRes().has_value()) {
        // Get path and ID.
        const auto sPathDeserializedFromRelativeRes = getPathDeserializedFromRelativeToRes()->first;
        const auto sObjectIdInDeserializedFile = getPathDeserializedFromRelativeToRes()->second;

        // This entity was deserialized from the `res` directory.
        // We should only serialize fields with changed values and additionally serialize
        // the path to the original file so that the rest
        // of the fields can be deserialized from that file.

        // Check that the original file exists.
        const auto pathToOriginalFile =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathDeserializedFromRelativeRes;
        if (!std::filesystem::exists(pathToOriginalFile)) [[unlikely]] {
            return Error(
                std::format(
                    "node \"{}\" has the path it was deserialized from ({}, ID {}) but this "
                    "file \"{}\" does not exist",
                    getNodeName(),
                    sPathDeserializedFromRelativeRes,
                    sObjectIdInDeserializedFile,
                    pathToOriginalFile.string()));
        }

        // Deserialize the original.
        std::unordered_map<std::string, std::string> customAttributes;
        auto result = deserialize<Node>(pathToOriginalFile, sObjectIdInDeserializedFile, customAttributes);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(result);
            error.addCurrentLocationToErrorStack();
            return error;
        }
        // Save original object to only save changed fields later.
        pOptionalOriginalObject = std::get<std::unique_ptr<Node>>(std::move(result));
        selfInfo.pOriginalObject = pOptionalOriginalObject.get();

        // Check if child nodes are located in the same file
        // (i.e. check if this node is a root of some external node tree).
        if (!mtxChildNodes.second.empty() &&
            isTreeDeserializedFromOneFile(sPathDeserializedFromRelativeRes)) {
            // Don't serialize information about child nodes,
            // when referencing an external node tree, we should only
            // allow modifying the root node, thus, because only root node
            // can have changed fields, we don't include child nodes here.
            bIncludeInformationAboutChildNodes = false;
            selfInfo.customAttributes[sTomlKeyExternalNodeTreePath.data()] = sPathDeserializedFromRelativeRes;
        }
    }
    vNodesInfo.push_back(
        SerializableObjectInformationWithUniquePtr(std::move(selfInfo), std::move(pOptionalOriginalObject)));

    iId += 1;

    if (bIncludeInformationAboutChildNodes) {
        // Get information about child nodes.
        std::scoped_lock guard(mtxChildNodes.first);
        for (const auto& pChildNode : mtxChildNodes.second) {
            // Skip node (and its child nodes) if it should not be serialized.
            if (!pChildNode->isSerialized()) {
                continue;
            }

            // Get serialization info.
            auto result = pChildNode->getInformationForSerialization(iId, iMyId);
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(std::move(result));
                return error;
            }
            auto vChildArray =
                std::get<std::vector<SerializableObjectInformationWithUniquePtr>>(std::move(result));

            // Add information about children.
            for (auto& childInfo : vChildArray) {
                vNodesInfo.push_back(std::move(childInfo));
            }
        }
    }

    return vNodesInfo;
}

bool Node::isTreeDeserializedFromOneFile(const std::string& sPathRelativeToRes) {
    if (!getPathDeserializedFromRelativeToRes().has_value()) {
        return false;
    }

    if (getPathDeserializedFromRelativeToRes().value().first != sPathRelativeToRes) {
        return false;
    }

    bool bPathEqual = true;
    lockChildren();
    {
        for (const auto& pChildNode : mtxChildNodes.second) {
            if (!pChildNode->isTreeDeserializedFromOneFile(sPathRelativeToRes)) {
                bPathEqual = false;
                break;
            }
        }
    }
    unlockChildren();

    return bPathEqual;
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
