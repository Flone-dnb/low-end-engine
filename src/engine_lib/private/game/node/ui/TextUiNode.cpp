#include "game/node/ui/TextUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "e9153575-0825-4934-b8a0-666f2eaa9fe9";
}

std::string TextUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string TextUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TextUiNode::TextUiNode() : TextUiNode("Text UI Node") {}

TextUiNode::TextUiNode(const std::string& sNodeName) : UiNode(sNodeName) {}

TypeReflectionInfo TextUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&TextUiNode::color).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setTextColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<TextUiNode*>(pThis)->getTextColor();
        }};

    variables.floats[NAMEOF_MEMBER(&TextUiNode::size).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<TextUiNode*>(pThis)->setTextSize(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextUiNode*>(pThis)->getTextSize();
        }};

    variables.floats[NAMEOF_MEMBER(&TextUiNode::lineSpacing).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setTextLineSpacing(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextUiNode*>(pThis)->getTextLineSpacing();
        }};

    variables.strings[NAMEOF_MEMBER(&TextUiNode::sText).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setText(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<TextUiNode*>(pThis)->getText().data();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TextUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextUiNode>(); },
        std::move(variables));
}

void TextUiNode::setText(const std::string& sText) { this->sText = sText; }

void TextUiNode::setTextSize(float size) {
    this->size = std::clamp(size, 0.001F, 1.0F); // NOLINT: avoid zero size
}

void TextUiNode::setTextColor(const glm::vec4& color) { this->color = color; }

void TextUiNode::setTextLineSpacing(float lineSpacing) { this->lineSpacing = std::max(lineSpacing, 0.0F); }

void TextUiNode::onSpawning() {
    UiNode::onSpawning();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeSpawning(this);
}

void TextUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeDespawning(this);
}

void TextUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onSpawnedNodeChangedVisibility(this);
}
