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
TextUiNode::TextUiNode(const std::string& sNodeName) : UiNode(sNodeName) {
    // Text generally needs less size that default for nodes.
    setSize(glm::vec2(0.2F, 0.03F)); // NOLINT
}

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

    variables.floats[NAMEOF_MEMBER(&TextUiNode::lineSpacing).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setTextLineSpacing(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextUiNode*>(pThis)->getTextLineSpacing();
        }};

    variables.floats[NAMEOF_MEMBER(&TextUiNode::textHeight).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setTextHeight(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<TextUiNode*>(pThis)->getTextHeight();
        }};

    variables.strings[NAMEOF_MEMBER(&TextUiNode::sText).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setText(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<TextUiNode*>(pThis)->getText().data();
        }};

    variables.bools[NAMEOF_MEMBER(&TextUiNode::bIsWordWrapEnabled).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setIsWordWrapEnabled(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<TextUiNode*>(pThis)->getIsWordWrapEnabled();
        }};

    variables.bools[NAMEOF_MEMBER(&TextUiNode::bHandleNewLineChars).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setHandleNewLineChars(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<TextUiNode*>(pThis)->getHandleNewLineChars();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TextUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextUiNode>(); },
        std::move(variables));
}

void TextUiNode::setText(const std::string& sText) { this->sText = sText; }

void TextUiNode::setTextColor(const glm::vec4& color) { this->color = color; }

void TextUiNode::setTextHeight(float height) { this->textHeight = height; }

void TextUiNode::setTextLineSpacing(float lineSpacing) { this->lineSpacing = std::max(lineSpacing, 0.0F); }

void TextUiNode::setIsWordWrapEnabled(bool bIsEnabled) { bIsWordWrapEnabled = bIsEnabled; }

void TextUiNode::setHandleNewLineChars(bool bHandleNewLineChars) {
    this->bHandleNewLineChars = bHandleNewLineChars;
}

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

void TextUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    const auto mtxChildNodes = getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    if (!mtxChildNodes.second.empty()) {
        Error::showErrorAndThrowException(
            std::format("text ui nodes can't have child nodes (text node \"{}\")", getNodeName()));
    }
}
