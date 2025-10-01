#include "NodeTreeInspector.h"

// Custom.
#include "game/World.h"
#include "node/property_inspector/PropertyInspector.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "misc/ReflectedTypeDatabase.h"
#include "node/menu/SelectNodeTypeMenu.h"
#include "node/menu/FileDialogMenu.h"
#include "node/node_tree_inspector/NodeTreeInspectorItem.h"
#include "EditorTheme.h"
#include "game/node/physics/CompoundCollisionNode.h"
#include "game/node/physics/CollisionNode.h"
#include "game/node/physics/DynamicBodyNode.h"
#include "EditorGameInstance.h"
#include "node/menu/SetNameMenu.h"
#include "game/node/Sound2dNode.h"
#include "game/node/Sound3dNode.h"
#include "EditorConstants.hpp"

// External.
#include "nameof.hpp"
#include "utf/utf.hpp"

NodeTreeInspector::NodeTreeInspector() : NodeTreeInspector("Node Tree Inspector") {}
NodeTreeInspector::NodeTreeInspector(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(EditorTheme::getContainerBackgroundColor());
    setPadding(EditorTheme::getPadding() / 2.0F);

    pLayoutNode = addChildNode(std::make_unique<LayoutUiNode>("Node Tree Inspector Layout"));
    pLayoutNode->setIsScrollBarEnabled(true);
    pLayoutNode->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pLayoutNode->setPadding(EditorTheme::getPadding());
    pLayoutNode->setChildNodeSpacing(EditorTheme::getSpacing());
}

void NodeTreeInspector::onGameNodeTreeLoaded(Node* pGameRootNode) {
    if (this->pGameRootNode != nullptr) {
        clearInspection();

        // Remove old tree.
        const auto mtxChildNodes = pLayoutNode->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            pNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    this->pGameRootNode = pGameRootNode;
    addGameNodeRecursive(pGameRootNode);
}

bool NodeTreeInspector::isNodeExternalTreeRootNode(Node* pNode) {
    if (pNode != pGameRootNode) {
        const auto optDeserializeInfo = pNode->getPathDeserializedFromRelativeToRes();
        if (optDeserializeInfo.has_value() && optDeserializeInfo->second == "0") {
            return true;
        }
    }

    return false;
}

void NodeTreeInspector::selectNodeById(size_t iNodeId) {
    const auto mtxChildNodes = pLayoutNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);
    for (const auto& pNode : mtxChildNodes.second) {
        const auto pItem = dynamic_cast<NodeTreeInspectorItem*>(pNode);
        if (pItem == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected node tree item");
        }
        if (*pItem->getDisplayedGameNode()->getNodeId() == iNodeId) {
            inspectGameNode(pItem);
            break;
        }
    }
}

void NodeTreeInspector::refreshGameNodeName(Node* pGameNode) {
    const auto mtxChildNodes = pLayoutNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);
    for (const auto& pNode : mtxChildNodes.second) {
        const auto pItem = dynamic_cast<NodeTreeInspectorItem*>(pNode);
        if (pItem == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected node tree item");
        }
        if (pItem->getDisplayedGameNode() == pGameNode) {
            pItem->setNodeToDisplay(pGameNode);
            break;
        }
    }
}

void NodeTreeInspector::addGameNodeRecursive(Node* pNode) {
    // Don't display editor nodes.
    if (pNode->getNodeName().starts_with(EditorConstants::getHiddenNodeNamePrefix())) {
        return;
    }

    const auto pItem = pLayoutNode->addChildNode(std::make_unique<NodeTreeInspectorItem>(this));
    pItem->setNodeToDisplay(pNode);

    // Don't display external node tree child nodes (only display root).
    if (isNodeExternalTreeRootNode(pNode)) {
        return;
    }

    const auto mtxChildNodes = pNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    for (const auto& pChildNode : mtxChildNodes.second) {
        addGameNodeRecursive(pChildNode);
    }
}

void NodeTreeInspector::showChildNodeCreationMenu(NodeTreeInspectorItem* pParent) {
    const auto pMenu = getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<SelectNodeTypeMenu>(
        "Add child node - select type", pParent->getDisplayedGameNode()));
    pMenu->setPosition(pParent->getPosition());

    pMenu->setOnTypeSelected([this, pParent](const std::string& sTypeGuid) {
        if (sTypeGuid == Sound2dNode::getTypeGuidStatic() || sTypeGuid == Sound3dNode::getTypeGuidStatic()) {
            // We need to assign sound channel before spawning.
            std::vector<std::pair<std::u16string, std::function<void()>>> vOptions;
            vOptions.reserve(static_cast<size_t>(SoundChannel::COUNT));

            for (size_t i = 0; i < static_cast<size_t>(SoundChannel::COUNT); i++) {
                vOptions.push_back(
                    {utf::as_u16(NAMEOF_ENUM(static_cast<SoundChannel>(i)).data()),
                     [this, pParent, sTypeGuid, i]() {
                         addChildNodeToNodeTree(pParent, sTypeGuid, static_cast<SoundChannel>(i));
                     }});
            }

            dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())
                ->openContextMenu(vOptions, u"select sound channel:");
            return;
        }

        addChildNodeToNodeTree(pParent, sTypeGuid);
    });
}

void NodeTreeInspector::showChangeNodeNameMenu(NodeTreeInspectorItem* pItem) {
    const auto pSetNameMenu = getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<SetNameMenu>());
    pSetNameMenu->setOnNameChanged([this, pItem](std::u16string_view sText) {
        pItem->pGameNode->setNodeName(utf::as_str8(sText));
        // Refresh tree.
        onGameNodeTreeLoaded(pGameRootNode);
    });
}

void NodeTreeInspector::showNodeTypeChangeMenu(NodeTreeInspectorItem* pItem) {
    const auto pMenu = getWorldRootNodeWhileSpawned()->addChildNode(
        std::make_unique<SelectNodeTypeMenu>("Change node type", pItem->getDisplayedGameNode()));
    pMenu->setPosition(pItem->getPosition());

    pMenu->setOnTypeSelected([this, pItem](const std::string& sTypeGuid) {
        if (sTypeGuid == Sound2dNode::getTypeGuidStatic() || sTypeGuid == Sound3dNode::getTypeGuidStatic()) {
            // We need to assign sound channel before spawning.
            std::vector<std::pair<std::u16string, std::function<void()>>> vOptions;
            vOptions.reserve(static_cast<size_t>(SoundChannel::COUNT));

            for (size_t i = 0; i < static_cast<size_t>(SoundChannel::COUNT); i++) {
                vOptions.push_back(
                    {utf::as_u16(NAMEOF_ENUM(static_cast<SoundChannel>(i)).data()),
                     [this, pItem, sTypeGuid, i]() {
                         changeNodeType(pItem, sTypeGuid, static_cast<SoundChannel>(i));
                     }});
            }

            dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())
                ->openContextMenu(vOptions, u"select sound channel:");
            return;
        }

        changeNodeType(pItem, sTypeGuid);
    });
}

void NodeTreeInspector::showAddExternalNodeTreeMenu(NodeTreeInspectorItem* pItem) {
    clearInspection();

    // Show file dialog.
    getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<FileDialogMenu>(
        ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME),
        std::vector<std::string>{".toml"},
        [this, pItem](const std::filesystem::path& selectedPath) {
            // Load selected tree.
            auto result = Node::deserializeNodeTree(selectedPath);
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(std::move(result));
                Logger::get().error(error.getInitialMessage());
                return;
            }
            auto pRoot = std::get<std::unique_ptr<Node>>(std::move(result));

            pItem->getDisplayedGameNode()->addChildNode(std::move(pRoot));

            // Refresh tree.
            onGameNodeTreeLoaded(pGameRootNode);
        }));
}

void NodeTreeInspector::moveGameNodeInChildArray(NodeTreeInspectorItem* pItem, bool bMoveUp) {
    clearInspection();

    const auto pNode = pItem->getDisplayedGameNode();
    const auto mtxParentNode = pNode->getParentNode();
    const auto mtxChildNodes = mtxParentNode.second->getChildNodes();
    {
        std::scoped_lock guard(*mtxParentNode.first, *mtxChildNodes.first);
        if (mtxParentNode.second == nullptr) [[unlikely]] {
            Logger::get().error(
                std::format("expected to the node \"{}\" to have a parent node", pNode->getNodeName()));
            return;
        }

        // Collect indices of non-hidden nodes.
        std::vector<size_t> vNonHiddenNodeIndices;
        vNonHiddenNodeIndices.reserve(mtxChildNodes.second.size());
        for (size_t i = 0; i < mtxChildNodes.second.size(); i++) {
            if (mtxChildNodes.second[i]->getNodeName().starts_with(
                    EditorConstants::getHiddenNodeNamePrefix())) {
                continue;
            }

            vNonHiddenNodeIndices.push_back(i);
        }
        if (vNonHiddenNodeIndices.empty() || vNonHiddenNodeIndices.size() == 1) {
            return;
        }

        // Find current index.
        size_t iCurrentIndex = 0;
        for (size_t iIndex : vNonHiddenNodeIndices) {
            if (mtxChildNodes.second[iIndex] != pNode) {
                continue;
            }
            iCurrentIndex = iIndex;
            break;
        }

        // Determine the target index.
        size_t iTargetIndex = 0;
        if (bMoveUp) {
            if (iCurrentIndex == 0) {
                iTargetIndex = vNonHiddenNodeIndices.size() - 1;
            } else {
                iTargetIndex = iCurrentIndex - 1;
            }
        } else {
            if (iCurrentIndex == vNonHiddenNodeIndices.size() - 1) {
                iTargetIndex = 0;
            } else {
                iTargetIndex = iCurrentIndex + 1;
            }
        }

        mtxParentNode.second->changeChildNodePositionIndex(
            vNonHiddenNodeIndices[iCurrentIndex], vNonHiddenNodeIndices[iTargetIndex]);
    }

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);
}

void NodeTreeInspector::duplicateGameNode(NodeTreeInspectorItem* pItem) {
    if (pItem->getDisplayedGameNode() == pGameRootNode) {
        return;
    }
    if (isNodeExternalTreeRootNode(pItem->getDisplayedGameNode())) {
        return;
    }

    clearInspection();

    // Create a duplicate.
    const auto pOriginalNode = pItem->getDisplayedGameNode();
    const auto sNodeName = std::string(pOriginalNode->getNodeName());
    auto pDuplicatedNode = duplicateNodeWithChildren(pOriginalNode);

    // Add to parent.
    {
        const auto& mtxParent = pOriginalNode->getParentNode();
        std::scoped_lock guard(*mtxParent.first);

        if (dynamic_cast<CollisionNode*>(pOriginalNode) != nullptr &&
            dynamic_cast<CompoundCollisionNode*>(mtxParent.second) == nullptr) {
            // Duplicating a collision node but the parent is not a compound.
            // Create a new parent to group collisions into a compound.
            auto pCompound = std::make_unique<CompoundCollisionNode>();
            pCompound->addChildNode(pOriginalNode);
            pCompound->addChildNode(std::move(pDuplicatedNode));
            mtxParent.second->addChildNode(std::move(pCompound));
            Logger::get().info(
                "created a compound node and grouped your collision nodes to speed up collision "
                "detection and thus improve performance");
        } else {
            mtxParent.second->addChildNode(std::move(pDuplicatedNode));
        }
    }

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);

    Logger::get().info(std::format("duplicated node \"{}\"", sNodeName));
}

std::unique_ptr<Node> NodeTreeInspector::duplicateNodeWithChildren(Node* pNodeToDuplicate) {
    // Create a duplicate.
    auto pDuplicatedNode =
        std::unique_ptr<Node>(dynamic_cast<Node*>(pNodeToDuplicate->createDuplicate().release()));
    if (pDuplicatedNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("failed to duplicate node");
    }

    // Duplicate child nodes.
    {
        const auto mtxChildNodes = pNodeToDuplicate->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pChildNode : mtxChildNodes.second) {
            pDuplicatedNode->addChildNode(duplicateNodeWithChildren(pChildNode));
        }
    }

    return pDuplicatedNode;
}

void NodeTreeInspector::deleteGameNode(NodeTreeInspectorItem* pItem) {
    clearInspection();

    pItem->getDisplayedGameNode()->unsafeDetachFromParentAndDespawn();

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);
}

void NodeTreeInspector::inspectGameNode(NodeTreeInspectorItem* pItem) {
    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());

    if (pInspectedItem == pItem) {
        // Clicked again - clear selection.
        clearInspection();
        return;
    }

    // Display properties.
    pGameInstance->getPropertyInspector()->setNodeToInspect(pItem->getDisplayedGameNode());

    // Update inspected item.
    if (pInspectedItem != nullptr) {
        pInspectedItem->setColor(EditorTheme::getButtonColor());
    }
    pInspectedItem = pItem;
    pInspectedItem->setColor(EditorTheme::getAccentColor());

    // Show gizmo.
    if (auto pSpatialNode = dynamic_cast<SpatialNode*>(pItem->getDisplayedGameNode())) {
        pGameInstance->showGizmoToControlNode(pSpatialNode);
    }
}

void NodeTreeInspector::clearInspection() {
    if (pInspectedItem == nullptr) {
        return;
    }

    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());

    pGameInstance->getPropertyInspector()->setNodeToInspect(nullptr);
    pGameInstance->showGizmoToControlNode(nullptr);

    pInspectedItem->setColor(EditorTheme::getButtonColor());
    pInspectedItem = nullptr;
}

void NodeTreeInspector::addChildNodeToNodeTree(
    NodeTreeInspectorItem* pParent, const std::string& sTypeGuid, SoundChannel soundChannel) {
    // Create new node.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);
    auto pSerializable = typeInfo.createNewObject();
    std::unique_ptr<Node> pNewNode(dynamic_cast<Node*>(pSerializable.get()));
    if (pNewNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("expected a node type for GUID \"{}\"", sTypeGuid));
    }
    pSerializable.release();

    if (auto pSoundNode = dynamic_cast<Sound2dNode*>(pNewNode.get())) {
        pSoundNode->setSoundChannel(soundChannel);
    } else if (auto pSoundNode = dynamic_cast<Sound3dNode*>(pNewNode.get())) {
        pSoundNode->setSoundChannel(soundChannel);
    }

    // Add child.
    pParent->getDisplayedGameNode()->addChildNode(std::move(pNewNode));

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);

    if (sTypeGuid == DynamicBodyNode::getTypeGuidStatic()) {
        Logger::get().info("note: dynamic bodies are not simulated in the editor");
    }
}

void NodeTreeInspector::changeNodeType(
    NodeTreeInspectorItem* pItem, const std::string& sTypeGuid, SoundChannel soundChannel) {
    clearInspection();

    // Create new node.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);
    auto pSerializable = typeInfo.createNewObject();
    std::unique_ptr<Node> pNewNode(dynamic_cast<Node*>(pSerializable.get()));
    if (pNewNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("expected a node type for GUID \"{}\"", sTypeGuid));
    }
    pSerializable.release();

    if (pItem->pGameNode->getWorldRootNodeWhileSpawned() == pItem->pGameNode) {
        pGameRootNode = pNewNode.get();
        dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())
            ->changeGameWorldRootNode(std::move(pNewNode));
    } else {
        const auto pParentNode = pItem->pGameNode->getParentNode().second;
        pNewNode->setNodeName(std::string(pItem->pGameNode->getNodeName()));

        pItem->pGameNode->unsafeDetachFromParentAndDespawn(true);

        pParentNode->addChildNode(std::move(pNewNode));
    }

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);
}
