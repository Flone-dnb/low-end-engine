#include "game/node/ui/UiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "misc/Profiler.hpp"
#include "render/Renderer.h"
#include "render/UiNodeManager.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/World.h"

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
    // Note: don't clamp to [0; 1] because layout with scroll can cause this to have negative Y (which is OK).
    this->position = position;

    onAfterPositionChanged();
}

void UiNode::setSize(const glm::vec2& size) {
    // Note: don't clamp to [0; 1] in some cases it might be needed.
    this->size.x = std::max(size.x, 0.001f);
    this->size.y = std::max(size.y, 0.001f);

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

    processVisibilityChange();
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

    // Affects all child nodes.
    {
        const auto mtxChildNodes = getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);

        for (const auto& pChildNode : mtxChildNodes.second) {
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected a UI node");
            }
            pUiChild->setUiLayer(layer);
        }
    }
}

void UiNode::setModal() {
    bShouldBeModal = true;
    // don't check if receiving input, some child nodes can receive input instead of this one
    if (isSpawned() && bAllowRendering && bIsVisible) {
        getWorldWhileSpawned()->getUiNodeManager().setModalNode(this);
    }
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

    getWorldWhileSpawned()->getUiNodeManager().setFocusedNode(this);
}

size_t UiNode::getNodeDepthWhileSpawned() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }

    return iNodeDepth;
}

size_t UiNode::getMaxChildCount() const { return std::numeric_limits<size_t>::max(); }

void UiNode::onSpawning() {
    Node::onSpawning();

    recalculateNodeDepthWhileSpawned();
}

void UiNode::onChildNodesSpawned() {
    Node::onChildNodesSpawned();

    if (bAllowRendering && bIsVisible) {
        if (isReceivingInput()) {
            getWorldWhileSpawned()->getUiNodeManager().onSpawnedUiNodeInputStateChange(this, true);
        }
        if (bShouldBeModal) {
            // don't check if receiving input, some child nodes can receive input instead of this one
            getWorldWhileSpawned()->getUiNodeManager().setModalNode(this);
        }
    }
}

void UiNode::onDespawning() {
    Node::onDespawning();

    if (bAllowRendering && bIsVisible && isReceivingInput()) {
        getWorldWhileSpawned()->getUiNodeManager().onSpawnedUiNodeInputStateChange(this, false);
    }
}

void UiNode::onChangedReceivingInputWhileSpawned(bool bEnabledNow) {
    Node::onChangedReceivingInputWhileSpawned(bEnabledNow);

    if (bAllowRendering && bIsVisible) {
        getWorldWhileSpawned()->getUiNodeManager().onSpawnedUiNodeInputStateChange(this, bEnabledNow);
    }
}

void UiNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    Node::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    // Reset clipping that was possibly set by some node in the parent hierarchy.
    setYClip(glm::vec2(0.0f, 1.0f));
    setAllowRendering(true);

    {
        const auto mtxParent = getParentNode();
        std::scoped_lock guard(*mtxParent.first);
        const auto pUiParent = dynamic_cast<UiNode*>(mtxParent.second);

        if (!isSpawned() && pUiParent != nullptr) {
            // Inherint UI layer.
            setUiLayer(pUiParent->getUiLayer());
        }
    }

    if (isSpawned()) {
        recalculateNodeDepthWhileSpawned();

        getWorldWhileSpawned()->getUiNodeManager().onNodeChangedDepth(this);
    }
}

void UiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    Node::onAfterNewDirectChildAttached(pNewDirectChild);

    if (getTypeGuid() == UiNode::getTypeGuidStatic()) {
        // Forbid child nodes because it might create confusion, for example when our parent is rect
        // but our children aren't scaled to full rect because there's a base UI node in the middle
        Error::showErrorAndThrowException(std::format(
            "node \"{}\" of type \"UI node\" (type GUID: {}) can't have child nodes because it has base UI "
            "node type",
            getNodeName(),
            getTypeGuid()));
    }

    // This rule just makes it easier to work with UI node hierarchy.
    const auto pUiChild = dynamic_cast<UiNode*>(pNewDirectChild);
    if (pUiChild == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("UI nodes can have only UI nodes as child nodes");
    }

    if (!isSpawned()) {
        // Apply layer to the new child.
        pUiChild->setUiLayer(layer);
    }

    // Apply visibility.
    pUiChild->setIsVisible(bIsVisible);
    pUiChild->setAllowRendering(bAllowRendering);
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

    processVisibilityChange();
}

bool UiNode::onMouseScrollMoveWhileHovered(int iOffset) {
    // Notify parent container.
    const auto mtxParent = getParentNode();
    std::scoped_lock guard(*mtxParent.first);

    if (mtxParent.second != nullptr) {
        const auto pUiNode = dynamic_cast<UiNode*>(mtxParent.second);
        if (pUiNode == nullptr) {
            return false;
        }

        return pUiNode->onMouseScrollMoveWhileHovered(iOffset);
    }

    return false;
}

void UiNode::processVisibilityChange() {
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
            pUiChild->setAllowRendering(bAllowRendering);
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

    if (isSpawned()) {
        if (isReceivingInput()) {
            getWorldWhileSpawned()->getUiNodeManager().onSpawnedUiNodeInputStateChange(
                this, bAllowRendering && bIsVisible);
        }

        if (bAllowRendering && bIsVisible && bShouldBeModal) {
            getWorldWhileSpawned()->getUiNodeManager().setModalNode(this);
        }

        // Do as the last step because the node can despawn itself in the user callback.
        if (bIsMouseCursorHovered) {
            bIsMouseCursorHovered = false;
            onMouseLeft();
        }
    }
}

void UiNode::setYClip(const glm::vec2& clip) {
    yClip = clip;
    onAfterYClipChanged();
}

glm::vec2 UiNode::calculateYClipForChild(const glm::vec2& childPos, const glm::vec2& childSize) {
    PROFILE_FUNC

    const auto pos = getPosition();

    float yClipStart = pos.y + yClip.x * size.y;
    float yClipSize = yClip.y * size.y;
    auto childYClip = glm::vec2(0.0f, 1.0f);

    if (yClipStart > childPos.y) {
        childYClip.x = std::min((yClipStart - childPos.y) / childSize.y, 1.0f);
    }

    if (yClipStart + yClipSize < childPos.y + childSize.y) {
        if (yClipStart + yClipSize <= childPos.y) {
            childYClip.y = 0.0f;
        } else {
            if (yClipStart <= childPos.y) {
                childYClip.y = (yClipStart + yClipSize - childPos.y) / childSize.y;
            } else {
                childYClip.y = yClipSize / childSize.y;
            }
        }
    } else {
        childYClip.y = 1.0f - childYClip.x;
    }

    return childYClip;
}
