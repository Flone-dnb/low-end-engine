#include "game/node/ui/UiNode.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "291887b8-dead-4fd8-9999-55d7585971c2";
}

std::string UiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string UiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo UiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec2s[NAMEOF_MEMBER(&UiNode::position).data()] = ReflectedVariableInfo<glm::vec2>{
        .setter = [](Serializable* pThis,
                     const glm::vec2& newValue) { reinterpret_cast<UiNode*>(pThis)->setPosition(newValue); },
        .getter = [](Serializable* pThis) -> glm::vec2 {
            return reinterpret_cast<UiNode*>(pThis)->getPosition();
        }};

    variables.bools[NAMEOF_MEMBER(&UiNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter = [](Serializable* pThis,
                     const bool& bNewValue) { reinterpret_cast<UiNode*>(pThis)->setIsVisible(bNewValue); },
        .getter = [](Serializable* pThis) -> bool { return reinterpret_cast<UiNode*>(pThis)->isVisible(); }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(UiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<UiNode>(); },
        std::move(variables));
}

UiNode::UiNode() : UiNode("UI Node") {}

UiNode::UiNode(const std::string& sNodeName) : Node(sNodeName) {}

void UiNode::setPosition(const glm::vec2& position) {
    this->position.x = std::clamp(position.x, 0.0F, 1.0F);
    this->position.y = std::clamp(position.y, 0.0F, 1.0F);
}

void UiNode::setIsVisible(bool bIsVisible) {
    if (this->bIsVisible == bIsVisible) {
        return;
    }
    this->bIsVisible = bIsVisible;

    onVisibilityChanged();
}

glm::vec2 UiNode::getPosition() const { return position; }

bool UiNode::isVisible() const { return bIsVisible; }
