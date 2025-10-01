#include "game/node/Node.h"

// Standard.
#include <ranges>

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
 */
static std::atomic<size_t> iAvailableNodeId{0};

namespace {
    constexpr std::string_view sTypeGuid = "a70f1233-ad98-4686-a987-aeb916804369";
}

std::string Node::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string Node::getTypeGuid() const { return sTypeGuid.data(); }

size_t Node::getAliveNodeCount() { return iTotalAliveNodeCount.load(); }

size_t Node::peekNextNodeId() { return iAvailableNodeId.load(); }

std::variant<std::unique_ptr<Node>, Error>
Node::deserializeNodeTree(const std::filesystem::path& pathToFile) {
    PROFILE_FUNC

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

        // This node is a root node of some external node tree it was deserialized using the info from the
        // external node tree file but its child nodes were not deserialized so we should deserialize them
        // here.

        // Construct path to this external node tree.
        const auto pathToExternalNodeTree =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / it->second;
        if (!std::filesystem::exists(pathToExternalNodeTree)) [[unlikely]] {
            return Error(std::format(
                "file storing external node tree \"{}\" does not exist", pathToExternalNodeTree.string()));
        }

        // Deserialize external node tree.
        auto result = deserializeNodeTree(pathToExternalNodeTree);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(result);
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto pExternalRootNode = std::get<std::unique_ptr<Node>>(std::move(result));

        auto mtxChildNodes = pExternalRootNode->getChildNodes();
        std::scoped_lock externalChildNodesGuard(*mtxChildNodes.first);

        while (!mtxChildNodes.second.empty()) {
            // Use a "while" loop instead of a "for" loop because this "child nodes array" will be
            // modified each iteration (this array will be smaller and smaller with each iteration since
            // we are "removing" child nodes of some parent node), thus we avoid modifying the
            // array while iterating over it.
            nodeInfo.pObject->addChildNode(*mtxChildNodes.second.begin());

            // Update list of child nodes.
            mtxChildNodes = pExternalRootNode->getChildNodes();
        };
    }

    // Sort all nodes by their ID. Prepare array of pairs: node - parent info.
    struct ParentInfo {
        size_t iParentId = 0;
        size_t iIndexInChildNodeArray = 0;
    };
    struct NodeInfo {
        std::optional<ParentInfo> optionalParentInfo;
    };

    std::optional<size_t> optionalRootNodeIndex;
    std::vector<std::pair<std::unique_ptr<Node>, NodeInfo>> vNodes(vDeserializedInfo.size());
    for (size_t i = 0; i < vDeserializedInfo.size(); i++) {
        auto& nodeInfo = vDeserializedInfo[i];
        bool bIsRootNode = false;

        // Check that this object has required attribute about parent ID.
        std::optional<ParentInfo> parentInfo;
        const auto parentNodeAttributeIt = nodeInfo.customAttributes.find(sTomlKeyParentNodeId.data());
        if (parentNodeAttributeIt == nodeInfo.customAttributes.end()) {
            if (!optionalRootNodeIndex.has_value()) {
                bIsRootNode = true;
            } else [[unlikely]] {
                return Error(std::format(
                    "found non root node \"{}\" that does not have a parent",
                    nodeInfo.pObject->getNodeName()));
            }
        } else {
            parentInfo = ParentInfo{};
            try {
                parentInfo->iParentId = std::stoull(parentNodeAttributeIt->second);
            } catch (std::exception& exception) {
                return Error(std::format(
                    "failed to convert attribute \"{}\" with value \"{}\" to integer, error: {}",
                    sTomlKeyParentNodeId,
                    parentNodeAttributeIt->second,
                    exception.what()));
            }

            // Check if this parent ID is outside of out array bounds.
            if (parentInfo->iParentId >= vNodes.size()) [[unlikely]] {
                return Error(std::format(
                    "parsed parent node ID is outside of bounds: {} >= {}",
                    parentInfo->iParentId,
                    vNodes.size()));
            }

            // There's also must be a value about node's index in parent's array of child nodes.
            const auto it = nodeInfo.customAttributes.find(sTomlKeyChildNodeArrayIndex.data());
            if (it == nodeInfo.customAttributes.end()) [[unlikely]] {
                return Error(std::format(
                    "error while deserializing node \"{}\" (ID in the file: {}): found parent index in "
                    "the file but also expected an index in the parent's array of child node (which was "
                    "not found)",
                    nodeInfo.pObject->getNodeName(),
                    nodeInfo.sObjectUniqueId));
            }
            try {
                parentInfo->iIndexInChildNodeArray = std::stoull(it->second);
            } catch (std::exception& exception) {
                return Error(std::format(
                    "failed to convert attribute \"{}\" with value \"{}\" to integer, error: {}",
                    sTomlKeyChildNodeArrayIndex,
                    it->second,
                    exception.what()));
            }
        }

        // Try to convert this node's ID to size_t.
        size_t iNodeId = 0;
        try {
            iNodeId = std::stoull(nodeInfo.sObjectUniqueId);
        } catch (std::exception& exception) {
            return Error(std::format(
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
        vNodes[iNodeId].first = std::move(nodeInfo.pObject);
        vNodes[iNodeId].second = NodeInfo{.optionalParentInfo = parentInfo};

        if (bIsRootNode) {
            optionalRootNodeIndex = iNodeId;
        }
    }

    // See if we found root node.
    if (!optionalRootNodeIndex.has_value()) [[unlikely]] {
        return Error("root node was not found");
    }

    // Build hierarchy in reverse ID order, this way we start from nodes without children and will continue
    // to "std::move" nodes without hitting deleted memory as parent nodes (see how to collect these IDs
    // during the serialization).
    std::unordered_map<Node*, std::vector<std::unique_ptr<Node>>> parentNodeToChildNodes;
    for (auto& [pNode, nodeInfo] : std::views::reverse(vNodes)) {
        if (pNode == nullptr) [[unlikely]] {
            return Error("unexpected nullptr");
        }

        if (!nodeInfo.optionalParentInfo.has_value()) {
            continue;
        }
        auto& parentInfo = *nodeInfo.optionalParentInfo;

        // Get parent.
        auto& pParentNode = vNodes[parentInfo.iParentId].first;
        if (pParentNode == nullptr) [[unlikely]] {
            return Error("unexpected nullptr");
        }

        auto& vChildNodesArray = parentNodeToChildNodes[pParentNode.get()];

        if (vChildNodesArray.size() <= parentInfo.iIndexInChildNodeArray) {
            vChildNodesArray.resize(parentInfo.iIndexInChildNodeArray + 1);
        }

        vChildNodesArray[parentInfo.iIndexInChildNodeArray] = std::move(pNode);
    }

    // Add child nodes in the correct order.
    for (auto& [pParentNode, vChildNodeArray] : parentNodeToChildNodes) {
        for (auto& pChildNode : vChildNodeArray) {
            if (pChildNode == nullptr) [[unlikely]] {
                // Found a hole in the parent's "child nodes" array. This might mean that serialized indices
                // in the "child nodes" array are invalid.
                Error::showErrorAndThrowException(std::format(
                    "found empty (nullptr) node in the array of child nodes for parent node \"{}\" this "
                    "might mean that \"{}\" value (in the node tree file) is invalid",
                    pParentNode->getNodeName(),
                    sTomlKeyChildNodeArrayIndex));
            }

            pParentNode->addChildNode(std::move(pChildNode));
        }
    }

    return std::move(vNodes[*optionalRootNodeIndex].first);
}

std::optional<Error> Node::serializeNodeTree(std::filesystem::path pathToFile, bool bEnableBackup) {
    // Self check: make sure this node is marked to be serialized.
    if (!bSerialize) [[unlikely]] {
        return Error(std::format(
            "node \"{}\" is marked to be ignored when serializing as part of a node tree but "
            "this node was explicitly requested to be serialized as a node tree",
            sNodeName));
    }

    // Add TOML extension here because other functions will rely on that.
    if (!pathToFile.string().ends_with(".toml")) {
        pathToFile += ".toml";
    }

    // Prepare path to the geometry directory.
    const std::string sFilename = pathToFile.stem().string();
    const auto pathToGeoDir =
        pathToFile.parent_path() / (sFilename + std::string(Serializable::getNodeTreeGeometryDirSuffix()));
    if (std::filesystem::exists(pathToGeoDir)) {
        // Delete old geometry files.
        // This will cleanup any no longer needed geometry files (for ex. if we saved a mesh node but
        // then deleted and now saving again).
        std::filesystem::remove_all(pathToGeoDir);
    }

    lockChildren(); // make sure nothing is changed/deleted while we are serializing
    {
        // Collect information for serialization.
        size_t iId = 0;
        auto result = getInformationForSerialization(pathToFile, iId, {});
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(result);
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto vOriginalNodesInfo =
            std::get<std::vector<SerializableObjectInformationWithUniquePtr>>(std::move(result));

        // Convert information array.
        std::vector<SerializableObjectInformation> vNodesInfo;
        vNodesInfo.reserve(vOriginalNodesInfo.size());
        for (auto& info : vOriginalNodesInfo) {
            vNodesInfo.push_back(std::move(info.info));
        }

        // Serialize.
        const auto optionalError = Serializable::serializeMultiple(pathToFile, vNodesInfo, bEnableBackup);
        if (optionalError.has_value()) [[unlikely]] {
            auto err = optionalError.value();
            err.addCurrentLocationToErrorStack();
            return err;
        }
    }
    unlockChildren();

    return {};
}

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

void Node::unsafeDetachFromParentAndDespawn(bool bDontLogMessage) {
    if (!bDontLogMessage) {
        Logger::get().info(std::format("detaching and despawning the node \"{}\"", getNodeName()));
        Logger::get().flushToDisk(); // flush in case if we crash later
    }

    std::unique_ptr<Node> pSelf;

    {
        if (isSpawned() && getWorldRootNodeWhileSpawned() == this) [[unlikely]] {
            Error::showErrorAndThrowException(
                "instead of despawning world's root node, create/replace world using GameInstance "
                "functions, this would destroy the previous world with all nodes");
        }

        // Detach from parent.
        std::scoped_lock guard(mtxIsSpawned.first, mtxParentNode.first);

        if (mtxParentNode.second != nullptr) {
            // Notify self.
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

            // Notify parent.
            mtxParentNode.second->onAfterDirectChildDetached(pSelf.get());

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
        Logger::get().warn(std::format(
            "\"total alive nodes\" counter is at its maximum value: {}, another new node will cause "
            "an overflow",
            iNodeCount + 1));
    }
}

Node::~Node() {
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" is being destroyed but it's still spawned", sNodeName));
    }

    // Decrement total node counter.
    iTotalAliveNodeCount.fetch_sub(1);
}

void Node::setNodeName(const std::string& sName) { this->sNodeName = sName; }

void Node::changeChildNodePositionIndex(size_t iIndexFrom, size_t iIndexTo) {
    {
        std::scoped_lock guard(mtxChildNodes.first);

        if (iIndexFrom >= mtxChildNodes.second.size() || iIndexTo >= mtxChildNodes.second.size())
            [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" received invalid index to move the child node (from {} to {})",
                sNodeName,
                iIndexFrom,
                iIndexTo));
        }

        if (iIndexFrom == iIndexTo) {
            return;
        }

        std::swap(mtxChildNodes.second[iIndexFrom], mtxChildNodes.second[iIndexTo]);
    }

    onAfterChildNodePositionChanged(iIndexFrom, iIndexTo);
}

Node* Node::getWorldRootNodeWhileSpawned() {
    std::scoped_lock guard(mtxIsSpawned.first);

    if (pWorldWeSpawnedIn == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to get world root node for node \"{}\" because the node is not spawned", sNodeName));
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

    onChangedReceivingInputWhileSpawned(bEnable);
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
        Error::showErrorAndThrowException(std::format(
            "this function should not be called while the node is not spawned (called from node \"{}\")",
            sNodeName));
    }

    if (pWorldWeSpawnedIn == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "spawned node \"{}\" attempted to request the game instance but world is nullptr", sNodeName));
    }

    return pWorldWeSpawnedIn->pGameManager->getGameInstance();
}

std::recursive_mutex& Node::getSpawnDespawnMutex() { return mtxIsSpawned.first; }

World* Node::getWorldWhileSpawned() const {
    if (pWorldWeSpawnedIn == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to get world - node \"{}\" is not spawned", sNodeName));
    }

    return pWorldWeSpawnedIn;
}

void Node::spawn() {
    PROFILE_FUNC
    PROFILE_ADD_SCOPE_TEXT(sNodeName.c_str(), sNodeName.size());

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
    if (iNodeId.value() + 1 == (std::numeric_limits<size_t>::max)()) [[unlikely]] {
        Logger::get().warn(std::format(
            "\"next available node ID\" is at its maximum value: {}, another spawned node will "
            "cause an overflow",
            iNodeId.value() + 1));
    }

    // Mark state.
    mtxIsSpawned.second = true;

    // Notify world in order for node ID to be registered before running custom user spawn logic.
    pWorldWeSpawnedIn->onNodeSpawned(this);

    {
        // Do custom user spawn logic.
        PROFILE_SCOPE("on spawning");
        PROFILE_ADD_SCOPE_TEXT(sNodeName.c_str(), sNodeName.size());
        onSpawning();
    }

    // We spawn self first and only then child nodes.
    // This spawn order is required for some nodes and engine parts to work correctly.
    // With this spawn order we will not make "holes" in the world's node tree
    // (i.e. when node is spawned, node's parent is not spawned but parent's parent node is spawned).

    // Spawn children.
    {
        std::scoped_lock childGuard(mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            if (pChildNode->isSpawned()) {
                // This node was most likely spawned in `onSpawning` from above.
                continue;
            }
            pChildNode->spawn();
        }
    }

    {
        // Notify user code.
        PROFILE_SCOPE("on child nodes spawned");
        PROFILE_ADD_SCOPE_TEXT(sNodeName.c_str(), sNodeName.size());
        onChildNodesSpawned();
    }
}

void Node::despawn() {
    PROFILE_FUNC
    PROFILE_ADD_SCOPE_TEXT(sNodeName.c_str(), sNodeName.size());

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
    // See if this action event is registered.
    const auto it = boundActionEvents.find(iActionId);
    if (it == boundActionEvents.end()) {
        return;
    }

    // Trigger user logic.
    if (bIsPressedDown && it->second.onPressed != nullptr) {
        it->second.onPressed(modifiers);
    } else if (it->second.onReleased != nullptr) {
        it->second.onReleased(modifiers);
    }
}

void Node::onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input) {
    // See if this axis event is registered.
    const auto it = boundAxisEvents.find(iAxisEventId);
    if (it == boundAxisEvents.end()) {
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
        Error::showErrorAndThrowException(std::format(
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
Node::getInformationForSerialization(
    const std::filesystem::path& pathToSerializeTo, size_t& iId, std::optional<size_t> iParentId) {
    if (!pathToSerializeTo.string().ends_with(".toml")) [[unlikely]] {
        // internal Node code that called this function should have added .toml
        Error::showErrorAndThrowException("unexpected state");
    }

    // Prepare information about nodes.
    // Use custom attributes for storing hierarchy information.
    std::vector<SerializableObjectInformationWithUniquePtr> vNodesInfo;

    // Add self first.
    const size_t iMyId = iId;

    SerializableObjectInformation selfInfo(this, std::to_string(iId));

    // Add parent ID.
    if (iParentId.has_value()) {
        selfInfo.customAttributes[sTomlKeyParentNodeId.data()] = std::to_string(iParentId.value());

        // Find self in the parent's array of child nodes.
        const auto mtxChildNodes = mtxParentNode.second->getChildNodes();
        std::optional<size_t> optionalIndex;
        for (size_t i = 0, iSerializableIndex = 0; i < mtxChildNodes.second.size(); i++) {
            if (mtxChildNodes.second[i] == this) {
                optionalIndex = iSerializableIndex;
                break;
            }
            if (mtxChildNodes.second[i]->isSerialized()) {
                iSerializableIndex += 1;
            }
        }
        if (!optionalIndex.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "unable to find child node \"{}\" in parent's array of child nodes", getNodeName()));
        }
        selfInfo.customAttributes[sTomlKeyChildNodeArrayIndex.data()] = std::to_string(*optionalIndex);
    }

    // Add original object (if was specified).
    bool bIncludeInformationAboutChildNodes = true;
    std::unique_ptr<Node> pOptionalOriginalObject;
    if (getPathDeserializedFromRelativeToRes().has_value()) {
        // Get path and ID.
        const auto sPathDeserializedFromRelativeRes = getPathDeserializedFromRelativeToRes()->first;
        const auto sObjectIdInDeserializedFile = getPathDeserializedFromRelativeToRes()->second;

        auto pathToOriginal =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathDeserializedFromRelativeRes;
        if (!pathToOriginal.string().ends_with(".toml")) {
            pathToOriginal += ".toml";
        }

        // Make sure to not use an original object if the same file is being overwritten.
        if (!std::filesystem::exists(pathToSerializeTo) ||
            std::filesystem::canonical(pathToSerializeTo) != std::filesystem::canonical(pathToOriginal)) {
            // This object was previously deserialized from the `res` directory and now is serialized
            // into a different file in the `res` directory.
            // We should only serialize fields with changed values and additionally serialize
            // the path to the original file so that the rest of the fields can be deserialized from that
            // file.

            // Deserialize the original.
            std::unordered_map<std::string, std::string> customAttributes;
            auto result = deserialize<Node>(pathToOriginal, sObjectIdInDeserializedFile, customAttributes);
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
                selfInfo.customAttributes[sTomlKeyExternalNodeTreePath.data()] =
                    sPathDeserializedFromRelativeRes;
            }
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
            auto result = pChildNode->getInformationForSerialization(pathToSerializeTo, iId, iMyId);
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
