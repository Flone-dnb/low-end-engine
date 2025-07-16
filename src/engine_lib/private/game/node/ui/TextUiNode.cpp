#include "game/node/ui/TextUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "game/Window.h"
#include "render/FontManager.h"
#include "render/Renderer.h"
#include "render/UiNodeManager.h"

// External.
#include "nameof.hpp"
#include "utf/utf.hpp"

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

    variables.vec4s[NAMEOF_MEMBER(&TextUiNode::scrollBarColor).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setScrollBarColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<TextUiNode*>(pThis)->getScrollBarColor();
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
                reinterpret_cast<TextUiNode*>(pThis)->setText(utf::as_u16(sNewValue));
            },
        .getter = [](Serializable* pThis) -> std::string {
            return utf::as_str8(reinterpret_cast<TextUiNode*>(pThis)->getText());
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

    variables.bools[NAMEOF_MEMBER(&TextUiNode::bIsScrollBarEnabled).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<TextUiNode*>(pThis)->setIsScrollBarEnabled(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<TextUiNode*>(pThis)->getIsScrollBarEnabled();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TextUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextUiNode>(); },
        std::move(variables));
}

void TextUiNode::setText(std::u16string_view sNewText) {
    sText = sNewText;

    iNewLineCharCountInText = 0;
    for (size_t i = 0; i < sText.size(); i++) {
        if (sText[i] == '\n') {
            if (bHandleNewLineChars) {
                iNewLineCharCountInText += 1;
            } else {
                sText[i] = u' ';
            }
        }
    }

    onAfterTextChanged();
}

void TextUiNode::setTextColor(const glm::vec4& color) { this->color = color; }

void TextUiNode::setScrollBarColor(const glm::vec4& color) { scrollBarColor = color; }

void TextUiNode::setTextHeight(float height) { this->textHeight = height; }

void TextUiNode::setTextLineSpacing(float lineSpacing) { this->lineSpacing = std::max(lineSpacing, 0.0F); }

void TextUiNode::setIsWordWrapEnabled(bool bIsEnabled) { bIsWordWrapEnabled = bIsEnabled; }

void TextUiNode::setHandleNewLineChars(bool bHandleNewLineChars) {
    this->bHandleNewLineChars = bHandleNewLineChars;
}

void TextUiNode::setIsScrollBarEnabled(bool bEnable) {
    bIsScrollBarEnabled = bEnable;
    iCurrentScrollOffset = 0;
}

void TextUiNode::moveScrollToTextCharacter(size_t iTextCharOffset) {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }

    if (!bIsScrollBarEnabled) [[unlikely]] {
        Error::showErrorAndThrowException("this function expects scroll bar to be enabled");
    }

    if (!bIsWordWrapEnabled && !bHandleNewLineChars) {
        iCurrentScrollOffset = 0;
        return;
    }

    iCurrentScrollOffset = getLineIndexForTextChar(iTextCharOffset);
}

size_t TextUiNode::getLineIndexForTextChar(size_t iTextCharOffset) {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be called while spawned");
    }

    if (!bIsScrollBarEnabled) [[unlikely]] {
        Error::showErrorAndThrowException("this function expects scroll bar to be enabled");
    }

    if (!bIsWordWrapEnabled && !bHandleNewLineChars) {
        return 0;
    }

    // Get font glyphs.
    auto& fontManager = getGameInstanceWhileSpawned()->getRenderer()->getFontManager();
    auto glyphGuard = fontManager.getGlyphs();

    // Prepare some variables.
    const auto [iWindowWidth, iWindowHeight] = getGameInstanceWhileSpawned()->getWindow()->getWindowSize();
    const auto textScaleFullscreen = getTextHeight() / fontManager.getFontHeightToLoad();
    const auto sizeInPixels = glm::vec2(
        getSize().x * static_cast<float>(iWindowWidth), getSize().y * static_cast<float>(iWindowHeight));

    float localX = 0.0F;
    size_t iLineIndex = 0;

    for (size_t i = 0; i < sText.size(); i++) {
        if (iTextCharOffset == i) {
            break;
        }

        const auto& character = sText[i];

        // Handle new line.
        if (character == '\n' && bHandleNewLineChars) {
            localX = 0.0F;
            iLineIndex += 1;
            continue;
        }

        const auto& glyph = glyphGuard.getGlyph(character);

        const float distanceToNextGlyph =
            static_cast<float>(glyph.advance >> 6) / // bitshift by 6 to get value in pixels (2^6 = 64)
            sizeInPixels.x * textScaleFullscreen;
        const float glyphWidth = std::max(
            static_cast<float>(glyph.size.x) / sizeInPixels.x * textScaleFullscreen, distanceToNextGlyph);

        if (bIsWordWrapEnabled && (localX + distanceToNextGlyph > 1.0F)) {
            localX = 0.0F;
            iLineIndex += 1;
        }

        localX += glyphWidth;
    }

    return iLineIndex;
}

void TextUiNode::onSpawning() {
    UiNode::onSpawning();

    const auto pRenderer = getGameInstanceWhileSpawned()->getRenderer();
    auto& fontManager = pRenderer->getFontManager();

    // Cache used glyphs.
    for (size_t i = 0; i < sText.size(); i++) {
        const auto& character = sText[i];
        fontManager.cacheGlyphs({character, character});
    }

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeSpawning(this);
}

void TextUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeDespawning(this);
}

void TextUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    if (isSpawned()) {
        // Notify manager.
        getWorldWhileSpawned()->getUiNodeManager().onSpawnedNodeChangedVisibility(this);
    }
}

void TextUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    Error::showErrorAndThrowException(
        std::format("text ui nodes can't have child nodes (text node \"{}\")", getNodeName()));
}

bool TextUiNode::onMouseScrollMoveWhileHovered(int iOffset) {
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

    return true;
}
