#include "game/node/ui/UiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiManager.h"
#include "game/node/ui/LayoutUiNode.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "291887b8-dead-4fd8-9999-55d7585971c2";
}

std::string UiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string UiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo UiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec2s[NAMEOF_MEMBER(&UiNode::size).data()] = ReflectedVariableInfo<glm::vec2>{
        .setter = [](Serializable* pThis,
                     const glm::vec2& newValue) { reinterpret_cast<UiNode*>(pThis)->setSize(newValue); },
        .getter = [](Serializable* pThis) -> glm::vec2 {
            return reinterpret_cast<UiNode*>(pThis)->getSize();
        }};

    variables.vec2s[NAMEOF_MEMBER(&UiNode::position).data()] = ReflectedVariableInfo<glm::vec2>{
        .setter = [](Serializable* pThis,
                     const glm::vec2& newValue) { reinterpret_cast<UiNode*>(pThis)->setPosition(newValue); },
        .getter = [](Serializable* pThis) -> glm::vec2 {
            return reinterpret_cast<UiNode*>(pThis)->getPosition();
        }};

    variables.unsignedInts[NAMEOF_MEMBER(&UiNode::layer).data()] = ReflectedVariableInfo<unsigned int>{
        .setter =
            [](Serializable* pThis, const unsigned int& iNewValue) {
                reinterpret_cast<UiNode*>(pThis)->setUiLayer(static_cast<UiLayer>(iNewValue));
            },
        .getter = [](Serializable* pThis) -> unsigned int {
            return static_cast<unsigned int>(reinterpret_cast<UiNode*>(pThis)->getUiLayer());
        }};

    variables.unsignedInts[NAMEOF_MEMBER(&UiNode::iExpandPortionInLayout).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter =
                [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<UiNode*>(pThis)->setExpandPortionInLayout(iNewValue);
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return reinterpret_cast<UiNode*>(pThis)->getExpandPortionInLayout();
            }};

    variables.bools[NAMEOF_MEMBER(&UiNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter = [](Serializable* pThis,
                     const bool& bNewValue) { reinterpret_cast<UiNode*>(pThis)->setIsVisible(bNewValue); },
        .getter = [](Serializable* pThis) -> bool { return reinterpret_cast<UiNode*>(pThis)->isVisible(); }};

    variables.bools[NAMEOF_MEMBER(&UiNode::bOccupiesSpaceEvenIfInvisible).data()] =
        ReflectedVariableInfo<bool>{
            .setter =
                [](Serializable* pThis, const bool& bNewValue) {
                    reinterpret_cast<UiNode*>(pThis)->setOccupiesSpaceEvenIfInvisible(bNewValue);
                },
            .getter = [](Serializable* pThis) -> bool {
                return reinterpret_cast<UiNode*>(pThis)->getOccupiesSpaceEvenIfInvisible();
            }};

    return TypeReflectionInfo(
        Node::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(UiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<UiNode>(); },
        std::move(variables));
}

UiNode::UiNode() : UiNode("UI Node") {}

UiNode::UiNode(const std::string& sNodeName) : Node(sNodeName) {}

void UiNode::setPosition(const glm::vec2& position) {
    this->position.x = std::clamp(position.x, 0.0F, 1.0F);
    this->position.y = std::clamp(position.y, 0.0F, 1.0F);

    onAfterPositionChanged();
}

void UiNode::setSize(const glm::vec2& size) {
    this->size.x = std::clamp(size.x, 0.0F, 1.0F);
    this->size.y = std::clamp(size.y, 0.0F, 1.0F);

    onAfterSizeChanged();
}

void UiNode::setExpandPortionInLayout(unsigned int iPortion) {
    iExpandPortionInLayout = std::max(1U, iPortion); // don't allow 0

    const auto mtxParent = getParentNode();
    std::scoped_lock guard(*mtxParent.first);

    if (auto pParentLayout = dynamic_cast<LayoutUiNode*>(mtxParent.second)) {
        pParentLayout->recalculatePosAndSizeForDirectChildNodes();
    }
}

void UiNode::setIsVisible(bool bIsVisible) {
    if (this->bIsVisible == bIsVisible) {
        return;
    }
    this->bIsVisible = bIsVisible;

    // Affects all child nodes.
    {
        const auto mtxChildNodes = getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected a UI node");
            }
            pUiChild->setIsVisible(bIsVisible);
        }
    }

    onVisibilityChanged();

    // Notify parent container.
    {
        const auto mtxParent = getParentNode();
        std::scoped_lock guard(*mtxParent.first);

        if (mtxParent.second != nullptr) {
            if (auto pLayout = dynamic_cast<LayoutUiNode*>(mtxParent.second)) {
                pLayout->onDirectChildNodeVisibilityChanged();
            }
        }
    }
}

void UiNode::setOccupiesSpaceEvenIfInvisible(bool bTakeSpace) { bOccupiesSpaceEvenIfInvisible = bTakeSpace; }

void UiNode::setUiLayer(UiLayer layer) {
    if (layer >= UiLayer::COUNT) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("invalid UI layer on node \"{}\"", getNodeName()));
    }

    if (isSpawned()) [[unlikely]] {
        // not allowed because UI manager does not expect this
        Error::showErrorAndThrowException("changing node UI layer is now allowed while it's spawned");
    }

    this->layer = layer;
}

void UiNode::setModal() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }
    if (!bIsVisible) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called on visible nodes");
    }
    // don't check if receiving input, some child nodes can receive input instead of this one

    getGameInstanceWhileSpawned()->getRenderer()->getUiManager().setModalNode(this);
}

void UiNode::setFocused() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }
    if (!bIsVisible) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called on visible nodes");
    }
    if (!isReceivingInput()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called on nodes that receive input");
    }

    getGameInstanceWhileSpawned()->getRenderer()->getUiManager().setFocusedNode(this);
}

size_t UiNode::getNodeDepthWhileSpawned() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }

    return iNodeDepth;
}

void UiNode::onSpawning() {
    Node::onSpawning();

    recalculateNodeDepthWhileSpawned();

    if (isReceivingInput()) {
        getGameInstanceWhileSpawned()->getRenderer()->getUiManager().onSpawnedUiNodeInputStateChange(
            this, true);
    }
}

void UiNode::onDespawning() {
    Node::onDespawning();

    if (isReceivingInput()) {
        getGameInstanceWhileSpawned()->getRenderer()->getUiManager().onSpawnedUiNodeInputStateChange(
            this, false);
    }
}

void UiNode::onChangedReceivingInputWhileSpawned(bool bEnabledNow) {
    Node::onChangedReceivingInputWhileSpawned(bEnabledNow);

    getGameInstanceWhileSpawned()->getRenderer()->getUiManager().onSpawnedUiNodeInputStateChange(
        this, bEnabledNow);
}

void UiNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    Node::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    // Reset clip that maybe was used by the previous parent.
    clipY = glm::vec2(0.0F, 1.0F); // NOLINT

    if (!isSpawned()) {
        return;
    }

    recalculateNodeDepthWhileSpawned();

    getGameInstanceWhileSpawned()->getRenderer()->getUiManager().onNodeChangedDepth(this);
}

void UiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    Node::onAfterNewDirectChildAttached(pNewDirectChild);

    // This rule just makes it easier to work with UI node hierarchy.
    if (dynamic_cast<UiNode*>(pNewDirectChild) == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("UI nodes can have only UI nodes as child nodes");
    }
}

void countDepthToRoot(Node* pCurrentNode, size_t& iDepth) {
    const auto mtxParent = pCurrentNode->getParentNode();
    std::scoped_lock guard(*mtxParent.first);

    if (mtxParent.second == nullptr) {
        return;
    }

    iDepth += 1;

    countDepthToRoot(mtxParent.second, iDepth);
}

void UiNode::recalculateNodeDepthWhileSpawned() {
    size_t iDepth = 0;
    countDepthToRoot(this, iDepth);

    iNodeDepth = iDepth;
}

void UiNode::setAllowRendering(bool bAllowRendering) {
    if (this->bAllowRendering == bAllowRendering) {
        return;
    }
    this->bAllowRendering = bAllowRendering;

    // Affects all child nodes.
    {
        const auto mtxChildNodes = getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected a UI node");
            }
            pUiChild->setAllowRendering(bAllowRendering);
        }
    }
}
