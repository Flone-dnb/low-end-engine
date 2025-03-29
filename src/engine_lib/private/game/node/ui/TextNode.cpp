#include "game/node/ui/TextNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiManager.h"

TextNode::TextNode() : TextNode("Text Node") {}

TextNode::TextNode(const std::string& sNodeName) : UiNode(sNodeName) {}

void TextNode::setText(const std::string& sText) { this->sText = sText; }

void TextNode::setTextSize(float size) { this->size = std::clamp(size, 0.01F, 1.0F); }

void TextNode::setTextColor(const glm::vec4& color) { this->color = color; }

void TextNode::onSpawning() {
    UiNode::onSpawning();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeSpawning(this);
}

void TextNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeDespawning(this);
}

void TextNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onSpawnedNodeChangedVisibility(this);
}
