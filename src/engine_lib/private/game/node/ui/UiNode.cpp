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

    variables.unsignedInts[NAMEOF_MEMBER(&UiNode::layer).data()] = ReflectedVariableInfo<unsigned int>{
        .setter =
            [](Serializable* pThis, const unsigned int& iNewValue) {
                reinterpret_cast<UiNode*>(pThis)->setUiLayer(static_cast<UiLayer>(iNewValue));
            },
        .getter = [](Serializable* pThis) -> unsigned int {
            return static_cast<unsigned int>(reinterpret_cast<UiNode*>(pThis)->getUiLayer());
        }};

    variables.bools[NAMEOF_MEMBER(&UiNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter = [](Serializable* pThis,
                     const bool& bNewValue) { reinterpret_cast<UiNode*>(pThis)->setIsVisible(bNewValue); },
        .getter = [](Serializable* pThis) -> bool { return reinterpret_cast<UiNode*>(pThis)->isVisible(); }};

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
}

void UiNode::setIsVisible(bool bIsVisible) {
    if (this->bIsVisible == bIsVisible) {
        return;
    }
    this->bIsVisible = bIsVisible;

    onVisibilityChanged();
}

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
