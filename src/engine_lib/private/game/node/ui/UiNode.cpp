#include "game/node/ui/UiNode.h"

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
