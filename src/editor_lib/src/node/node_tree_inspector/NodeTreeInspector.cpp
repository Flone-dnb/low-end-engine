#include "NodeTreeInspector.h"

// Custom.
#include "game/World.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "misc/ReflectedTypeDatabase.h"
#include "node/menu/SelectNodeTypeMenu.h"
#include "node/node_tree_inspector/NodeTreeInspectorItem.h"
#include "EditorColorTheme.h"
#include "EditorGameInstance.h"
#include "game/node/Sound2dNode.h"
#include "game/node/Sound3dNode.h"

// Externa.
#include "nameof.hpp"
#include "utf/utf.hpp"

NodeTreeInspector::NodeTreeInspector() : NodeTreeInspector("Node Tree Inspector") {}
NodeTreeInspector::NodeTreeInspector(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(glm::vec4(0.0F));
    setPadding(EditorColorTheme::getPadding() / 2.0F);

    const auto pBackground = addChildNode(std::make_unique<RectUiNode>());
    pBackground->setColor(EditorColorTheme::getContainerBackgroundColor());

    pLayoutNode = pBackground->addChildNode(std::make_unique<LayoutUiNode>("Node Tree Inspector Layout"));
    pLayoutNode->setIsScrollBarEnabled(true);
    pLayoutNode->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pLayoutNode->setPadding(EditorColorTheme::getPadding());
    pLayoutNode->setChildNodeSpacing(EditorColorTheme::getSpacing());
}

void NodeTreeInspector::onGameNodeTreeLoaded(Node* pGameRootNode) {
    if (this->pGameRootNode != nullptr) {
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

void NodeTreeInspector::addGameNodeRecursive(Node* pNode) {
    if (pNode->getNodeName().starts_with(getHiddenNodeNamePrefix())) {
        return;
    }

    const auto pItem = pLayoutNode->addChildNode(std::make_unique<NodeTreeInspectorItem>());
    pItem->setNodeToDisplay(pNode);

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

void NodeTreeInspector::deleteGameNode(NodeTreeInspectorItem* pItem) {
    pItem->getDisplayedGameNode()->unsafeDetachFromParentAndDespawn();

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);
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
}

void NodeTreeInspector::changeNodeType(
    NodeTreeInspectorItem* pItem, const std::string& sTypeGuid, SoundChannel soundChannel) {
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

        const auto pNode = pParentNode->addChildNode(std::move(pNewNode));
    }

    // Refresh tree.
    onGameNodeTreeLoaded(pGameRootNode);
}
