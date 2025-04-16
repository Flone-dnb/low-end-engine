#include "game/node/ui/TextNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "e9153575-0825-4934-b8a0-666f2eaa9fe9";
}

std::string TextNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string TextNode::getTypeGuid() const { return sTypeGuid.data(); }

TextNode::TextNode() : TextNode("Text Node") {}

TextNode::TextNode(const std::string& sNodeName) : UiNode(sNodeName) {}

TypeReflectionInfo TextNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&TextNode::color).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<TextNode*>(pThis)->setTextColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<TextNode*>(pThis)->getTextColor();
        }};

    variables.floats[NAMEOF_MEMBER(&TextNode::size).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<TextNode*>(pThis)->setTextSize(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextNode*>(pThis)->getTextSize();
        }};

    variables.floats[NAMEOF_MEMBER(&TextNode::lineSpacing).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<TextNode*>(pThis)->setTextLineSpacing(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextNode*>(pThis)->getTextLineSpacing();
        }};

    variables.strings[NAMEOF_MEMBER(&TextNode::sText).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<TextNode*>(pThis)->setText(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<TextNode*>(pThis)->getText().data();
        }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(TextNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextNode>(); },
        std::move(variables));
}

void TextNode::setText(const std::string& sText) { this->sText = sText; }

void TextNode::setTextSize(float size) {
    this->size = std::clamp(size, 0.001F, 1.0F); // NOLINT: avoid zero size
}

void TextNode::setTextColor(const glm::vec4& color) { this->color = color; }

void TextNode::setTextLineSpacing(float lineSpacing) { this->lineSpacing = std::max(lineSpacing, 0.0F); }

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
