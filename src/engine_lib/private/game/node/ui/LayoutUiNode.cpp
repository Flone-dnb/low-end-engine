#include "game/node/ui/LayoutUiNode.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "b012c9e2-358a-453b-9bf6-a65c2a2cc43c";
}

std::string LayoutUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string LayoutUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo LayoutUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&LayoutUiNode::scrollBarColor).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<LayoutUiNode*>(pThis)->setScrollBarColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<LayoutUiNode*>(pThis)->getScrollBarColor();
        }};

    variables.floats[NAMEOF_MEMBER(&LayoutUiNode::childNodeSpacing).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<LayoutUiNode*>(pThis)->setChildNodeSpacing(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<LayoutUiNode*>(pThis)->getChildNodeSpacing();
        }};

    variables.floats[NAMEOF_MEMBER(&LayoutUiNode::padding).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<LayoutUiNode*>(pThis)->setPadding(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<LayoutUiNode*>(pThis)->getPadding();
        }};

    variables.unsignedInts[NAMEOF_MEMBER(&LayoutUiNode::childExpandRule).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter =
                [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<LayoutUiNode*>(pThis)->setChildNodeExpandRule(
                        static_cast<ChildNodeExpandRule>(iNewValue));
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return static_cast<unsigned int>(
                    reinterpret_cast<LayoutUiNode*>(pThis)->getChildNodeExpandRule());
            }};

    variables.bools[NAMEOF_MEMBER(&LayoutUiNode::bIsHorizontal).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<LayoutUiNode*>(pThis)->setIsHorizontal(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<LayoutUiNode*>(pThis)->getIsHorizontal();
        }};

    variables.bools[NAMEOF_MEMBER(&LayoutUiNode::bIsScrollBarEnabled).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<LayoutUiNode*>(pThis)->setIsScrollBarEnabled(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<LayoutUiNode*>(pThis)->getIsScrollBarEnabled();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(LayoutUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<LayoutUiNode>(); },
        std::move(variables));
}

LayoutUiNode::LayoutUiNode() : LayoutUiNode("Layout UI Node") {}
LayoutUiNode::LayoutUiNode(const std::string& sNodeName) : UiNode(sNodeName) {
    mtxLayoutParent.second = nullptr;
    setIsReceivingInput(bIsScrollBarEnabled);
}

void LayoutUiNode::onAfterPositionChanged() {
    UiNode::onAfterPositionChanged();

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterSizeChanged() {
    UiNode::onAfterSizeChanged();

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterChildNodePositionChanged(size_t iIndexFrom, size_t iIndexTo) {
    UiNode::onAfterChildNodePositionChanged(iIndexFrom, iIndexTo);

    recalculatePosAndSizeForDirectChildNodes();
}

bool LayoutUiNode::onMouseScrollMoveWhileHovered(int iOffset) {
    if (!bIsScrollBarEnabled) {
        return UiNode::onMouseScrollMoveWhileHovered(iOffset);
    }

    if (iOffset < 0) {
        iCurrentScrollOffset += static_cast<size_t>(std::abs(iOffset));
    } else if (iCurrentScrollOffset > 0) {
        if (static_cast<size_t>(iOffset) > iCurrentScrollOffset) {
            iCurrentScrollOffset = 0;
        } else {
            iCurrentScrollOffset -= static_cast<size_t>(iOffset);
        }
    }

    recalculatePosAndSizeForDirectChildNodes();

    return true;
}

void LayoutUiNode::setIsHorizontal(bool bIsHorizontal) {
    this->bIsHorizontal = bIsHorizontal;

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::setChildNodeSpacing(float spacing) {
    childNodeSpacing = std::clamp(spacing, 0.0F, 1.0F);

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::setChildNodeExpandRule(ChildNodeExpandRule expandRule) {
    childExpandRule = expandRule;

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::setPadding(float padding) {
    this->padding = std::clamp(padding, 0.0F, 0.5F); // NOLINT

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::setIsScrollBarEnabled(bool bEnable) {
    bIsScrollBarEnabled = bEnable;

    setIsReceivingInput(bIsScrollBarEnabled);

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::setScrollBarColor(const glm::vec4& color) { scrollBarColor = color; }

void LayoutUiNode::onAfterDeserialized() {
    UiNode::onAfterDeserialized();

    setIsReceivingInput(bIsScrollBarEnabled);
}

void LayoutUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    if (!bIsScrollBarEnabled) {
        return;
    }

    setIsReceivingInput(isVisible());
}

void LayoutUiNode::onChildNodesSpawned() {
    UiNode::onChildNodesSpawned();

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    if (bIsScrollBarEnabled) {
        // Scroll to bottom.
        iCurrentScrollOffset = static_cast<unsigned int>(
            std::max(0.0F, totalScrollHeight - getSize().y * 3.25F) / // TODO: rework magic numbers
            scrollBarStepLocal);
        recalculatePosAndSizeForDirectChildNodes();
    }
}

void LayoutUiNode::onAfterDirectChildDetached(Node* pDetachedDirectChild) {
    UiNode::onAfterDirectChildDetached(pDetachedDirectChild);

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    UiNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    // Find a layout node in the parent chain and save it.
    std::scoped_lock parentGuard(mtxLayoutParent.first);

    mtxLayoutParent.second = getParentNodeOfType<LayoutUiNode>();
}

void LayoutUiNode::onDirectChildNodeVisibilityChanged() { recalculatePosAndSizeForDirectChildNodes(); }

void LayoutUiNode::recalculatePosAndSizeForDirectChildNodes() {
    const auto mtxChildNodes = getChildNodes();
    std::scoped_lock childGuard(*mtxChildNodes.first);

    if (bIsCurrentlyUpdatingChildNodes) {
        return;
    }

    bIsCurrentlyUpdatingChildNodes = true;
    {
        // First collect expand portions.
        float expandPortionSum = 0.0F;
        bool bAtLeastOneChildVisible = false;
        for (const auto& pChildNode : mtxChildNodes.second) {
            // Cast type.
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("unexpected state");
            }

            if (!pUiChild->isVisible() && !pUiChild->getOccupiesSpaceEvenIfInvisible()) {
                continue;
            }
            if (!isVisible() && pUiChild->isVisible()) {
                pUiChild->setIsVisible(false);
            }

            bAtLeastOneChildVisible = true;
            expandPortionSum += static_cast<float>(pUiChild->getExpandPortionInLayout());
        }
        if (!bAtLeastOneChildVisible) {
            // Notify parent layout.
            std::scoped_lock guard(mtxLayoutParent.first);
            if (mtxLayoutParent.second != nullptr) {
                mtxLayoutParent.second->recalculatePosAndSizeForDirectChildNodes();
            }

            bIsCurrentlyUpdatingChildNodes = false;
            return;
        }

        const auto layoutOldSize = getSize();
        const auto layoutPos = getPosition();
        glm::vec2 currentChildPos = getPosition();

        // Consider padding.
        const auto screenPadding = std::min(layoutOldSize.x, layoutOldSize.y) * padding;
        currentChildPos += screenPadding;
        const auto sizeForChildNodes = glm::vec2(layoutOldSize - 2.0F * screenPadding);

        // Check scroll bar.
        if (bIsScrollBarEnabled && bIsHorizontal) [[unlikely]] {
            Error::showErrorAndThrowException("scroll bar for horizontal layouts is not supported yet");
        }
        if (bIsScrollBarEnabled && (childExpandRule == ChildNodeExpandRule::EXPAND_ALONG_MAIN_AXIS ||
                                    childExpandRule == ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS))
            [[unlikely]] {
            Error::showErrorAndThrowException(
                "scroll bar with child expand rule is only allowed when expand rule is set to \"secondary "
                "axis\"");
        }
        float yOffsetForScrollToSkip = 0.0F;
        if (bIsScrollBarEnabled) {
            yOffsetForScrollToSkip -=
                (scrollBarStepLocal * layoutOldSize.y) * static_cast<float>(iCurrentScrollOffset);
        }
        totalScrollHeight = 0.0F;

        // Add spacers to total portion sum.
        const auto spacerPortion = expandPortionSum * childNodeSpacing;
        expandPortionSum +=
            spacerPortion *
            static_cast<float>(std::max(static_cast<size_t>(1), mtxChildNodes.second.size()) - 1ULL);

        const auto spacerActualPortion = childExpandRule == ChildNodeExpandRule::DONT_EXPAND
                                             ? childNodeSpacing
                                             : spacerPortion / expandPortionSum;

        float layoutNewSizeOnMainAxis =
            screenPadding * 2.0F; // will be updated while iterating over child nodes

        // Update position and size for all direct child nodes.
        for (const auto& pChildNode : mtxChildNodes.second) {
            // Cast type.
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("unexpected state");
            }

            if (!pUiChild->isVisible() && !pUiChild->getOccupiesSpaceEvenIfInvisible()) {
                continue;
            }

            const auto childOldSize = pUiChild->getSize();
            glm::vec2 childNewSize = childOldSize;

            const auto expandFactor =
                static_cast<float>(pUiChild->getExpandPortionInLayout()) / expandPortionSum;

            // Calculate child size.
            switch (childExpandRule) {
            case (ChildNodeExpandRule::DONT_EXPAND): {
                break;
            }
            case (ChildNodeExpandRule::EXPAND_ALONG_MAIN_AXIS): {
                if (bIsHorizontal) {
                    childNewSize = glm::vec2(expandFactor * sizeForChildNodes.x, childOldSize.y);
                } else {
                    childNewSize = glm::vec2(childOldSize.x, expandFactor * sizeForChildNodes.y);
                }
                break;
            }
            case (ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS): {
                if (bIsHorizontal) {
                    childNewSize = glm::vec2(childOldSize.x, sizeForChildNodes.y);
                } else {
                    childNewSize = glm::vec2(sizeForChildNodes.x, childOldSize.y);
                }
                break;
            }
            case (ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS): {
                if (bIsHorizontal) {
                    childNewSize = glm::vec2(expandFactor * sizeForChildNodes.x, sizeForChildNodes.y);
                } else {
                    childNewSize = glm::vec2(sizeForChildNodes.x, expandFactor * sizeForChildNodes.y);
                }
                break;
            }
            default:
                [[unlikely]] {
                    Error::showErrorAndThrowException("unhandled case");
                    break;
                }
            }

            float lastChildSize = 0.0F;
            if (bIsHorizontal) {
                lastChildSize = childNewSize.x + spacerActualPortion * sizeForChildNodes.x;
            } else {
                lastChildSize = childNewSize.y + spacerActualPortion * sizeForChildNodes.y;
            }

            totalScrollHeight += lastChildSize;

            if (bIsScrollBarEnabled) {
                if (yOffsetForScrollToSkip + lastChildSize < 0.0F ||
                    currentChildPos.y + childNewSize.y > layoutPos.y + layoutOldSize.y) {
                    //  Partially outside of the visible area - don't render (TODO: for now).
                    yOffsetForScrollToSkip += lastChildSize;
                    pUiChild->setAllowRendering(false);
                    continue;
                }
                yOffsetForScrollToSkip += lastChildSize;
            }

            pUiChild->setAllowRendering(bAllowRendering);
            pUiChild->setSize(childNewSize);
            pUiChild->setPosition(currentChildPos);

            if (bIsHorizontal) {
                currentChildPos.x += lastChildSize;
                layoutNewSizeOnMainAxis += lastChildSize;
            } else {
                currentChildPos.y += lastChildSize;
                layoutNewSizeOnMainAxis += lastChildSize;
            }
        }

        if (!mtxChildNodes.second.empty()) {
            // Remove last spacer.
            if (bIsHorizontal) {
                layoutNewSizeOnMainAxis -= spacerActualPortion * sizeForChildNodes.x;
            } else {
                layoutNewSizeOnMainAxis -= spacerActualPortion * sizeForChildNodes.y;
                totalScrollHeight -= spacerActualPortion * sizeForChildNodes.y;
            }
        }

        totalScrollHeight /= layoutOldSize.y;

        if (!mtxChildNodes.second.empty() && !bIsScrollBarEnabled &&
            childExpandRule == ChildNodeExpandRule::DONT_EXPAND) {
            // Expand layout size if needed to fit all child nodes (but don't shrink if can shrink).
            if (bIsHorizontal) {
                setSize(glm::vec2(layoutNewSizeOnMainAxis, layoutOldSize.y));
            } else if (!bIsHorizontal) {
                setSize(glm::vec2(layoutOldSize.x, layoutNewSizeOnMainAxis));
            }
        }

        // Notify parent layout.
        std::scoped_lock guard(mtxLayoutParent.first);
        if (mtxLayoutParent.second != nullptr) {
            mtxLayoutParent.second->recalculatePosAndSizeForDirectChildNodes();
        }
    }
    bIsCurrentlyUpdatingChildNodes = false;
}
