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

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(LayoutUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<LayoutUiNode>(); },
        std::move(variables));
}

LayoutUiNode::LayoutUiNode() : LayoutUiNode("Layout UI Node") {}
LayoutUiNode::LayoutUiNode(const std::string& sNodeName) : UiNode(sNodeName) {
    mtxLayoutParent.second = nullptr;
}

void LayoutUiNode::onAfterSizeChanged() {
    UiNode::onAfterSizeChanged();

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterChildNodePositionChanged(size_t iIndexFrom, size_t iIndexTo) {
    UiNode::onAfterChildNodePositionChanged(iIndexFrom, iIndexTo);

    recalculatePosAndSizeForDirectChildNodes();
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

void LayoutUiNode::onChildNodesSpawned() {
    UiNode::onChildNodesSpawned();

    recalculatePosAndSizeForDirectChildNodes();
}

void LayoutUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    recalculatePosAndSizeForDirectChildNodes();
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
        for (const auto& pChildNode : mtxChildNodes.second) {
            // Cast type.
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("unexpected state");
            }

            expandPortionSum += pUiChild->getExpandPortionInLayout();
        }

        const auto layoutOldSize = getSize();
        glm::vec2 currentChildPos = getPosition();

        // Consider padding.
        if (bIsHorizontal) {
            currentChildPos.y += padding * layoutOldSize.y;
        } else {
            currentChildPos.x += padding * layoutOldSize.x;
        }
        const auto sizeForChildNodes = glm::vec2(layoutOldSize - 2.0F * (padding * layoutOldSize));

        // Add spacers to total portion sum.
        const auto spacerPortion = expandPortionSum * childNodeSpacing;
        expandPortionSum += spacerPortion * (std::max(1ULL, mtxChildNodes.second.size()) - 1ULL);

        const auto spacerActualPortion = childExpandRule == ChildNodeExpandRule::DONT_EXPAND
                                             ? childNodeSpacing
                                             : spacerPortion / expandPortionSum;

        float layoutNewSizeOnMainAxis =
            padding * 2.0F; // NOLINT: will be updated while iterating over child nodes

        // Update position and size for all direct child nodes.
        for (const auto& pChildNode : mtxChildNodes.second) {
            // Cast type.
            const auto pUiChild = dynamic_cast<UiNode*>(pChildNode);
            if (pUiChild == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("unexpected state");
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
                    childNewSize = glm::vec2(childOldSize.x, expandFactor * sizeForChildNodes.y);
                } else {
                    childNewSize = glm::vec2(expandFactor * sizeForChildNodes.x, childOldSize.y);
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

            pUiChild->setSize(childNewSize);
            pUiChild->setPosition(currentChildPos);

            if (bIsHorizontal) {
                const auto lastNodeSize = childNewSize.x + spacerActualPortion * sizeForChildNodes.x;
                currentChildPos.x += lastNodeSize;
                layoutNewSizeOnMainAxis += lastNodeSize;
            } else {
                const auto lastNodeSize = childNewSize.y + spacerActualPortion * sizeForChildNodes.y;
                currentChildPos.y += lastNodeSize;
                layoutNewSizeOnMainAxis += lastNodeSize;
            }
        }

        if (!mtxChildNodes.second.empty()) {
            // Remove last spacer.
            if (bIsHorizontal) {
                layoutNewSizeOnMainAxis -= spacerActualPortion * sizeForChildNodes.x;
            } else {
                layoutNewSizeOnMainAxis -= spacerActualPortion * sizeForChildNodes.y;
            }
        }

        // Expand our size if needed to fit all child nodes (but don't shrink if can shrink).
        if (bIsHorizontal && layoutNewSizeOnMainAxis > layoutOldSize.x) {
            // Self check:
            if (childExpandRule != ChildNodeExpandRule::DONT_EXPAND) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("unexpected for node \"{}\" to expand", getNodeName()));
            }

            setSize(glm::vec2(layoutNewSizeOnMainAxis, layoutOldSize.y));
        } else if (!bIsHorizontal && layoutNewSizeOnMainAxis > layoutOldSize.y) {
            // Self check:
            if (childExpandRule != ChildNodeExpandRule::DONT_EXPAND) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("unexpected for node \"{}\" to expand", getNodeName()));
            }

            setSize(glm::vec2(layoutOldSize.x, layoutNewSizeOnMainAxis));
        }

        // Notify parent layout.
        std::scoped_lock guard(mtxLayoutParent.first);
        if (mtxLayoutParent.second != nullptr) {
            mtxLayoutParent.second->recalculatePosAndSizeForDirectChildNodes();
        }
    }
    bIsCurrentlyUpdatingChildNodes = false;
}
