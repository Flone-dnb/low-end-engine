#include "game/node/ui/TextEditUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "game/Window.h"
#include "render/UiManager.h"
#include "render/font/FontManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "69581f29-3b7c-4bcf-9fa3-62c428083f6e";
}

std::string TextEditUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string TextEditUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TextEditUiNode::TextEditUiNode() : TextEditUiNode("Text Edit UI Node") {}
TextEditUiNode::TextEditUiNode(const std::string& sNodeName) : TextUiNode(sNodeName) {
    // Text generally needs less size that default for nodes.
    setSize(glm::vec2(0.2F, 0.03F)); // NOLINT
    setIsReceivingInput(true);
    setHandleNewLineChars(true);
    setIsWordWrapEnabled(true);
}

TypeReflectionInfo TextEditUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&TextEditUiNode::textSelectionColor).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<TextEditUiNode*>(pThis)->setTextSelectionColor(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<TextEditUiNode*>(pThis)->getTextSelectionColor();
            }};

    variables.bools[NAMEOF_MEMBER(&TextEditUiNode::bIsReadOnly).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<TextEditUiNode*>(pThis)->setIsReadOnly(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<TextEditUiNode*>(pThis)->getIsReadOnly();
        }};

    return TypeReflectionInfo(
        TextUiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TextEditUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextEditUiNode>(); },
        std::move(variables));
}

void TextEditUiNode::setIsReadOnly(bool bIsReadOnly) {
    this->bIsReadOnly = bIsReadOnly;
    setIsReceivingInput(!bIsReadOnly);

    if (bIsReadOnly) {
        optionalCursorOffset = {};
        optionalSelection = {};
    }
}

void TextEditUiNode::setTextSelectionColor(const glm::vec4& textSelectionColor) {
    this->textSelectionColor = textSelectionColor;
}

void TextEditUiNode::onAfterDeserialized() {
    TextUiNode::onAfterDeserialized();

    setIsReceivingInput(!bIsReadOnly);
}

void TextEditUiNode::onMouseClickOnUiNode(
    MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    TextUiNode::onMouseClickOnUiNode(button, modifiers, bIsPressedDown);

    if (button != MouseButton::LEFT) {
        return;
    }

    if (bIsPressedDown) {
        optionalCursorOffset = convertMouseCursorPosToTextOffset();
        optionalSelection = {};
    } else {
        const auto textOffset = convertMouseCursorPosToTextOffset();
        if (optionalCursorOffset.has_value() && textOffset != *optionalCursorOffset) {
            optionalSelection = {
                std::min(textOffset, *optionalCursorOffset), std::max(textOffset, *optionalCursorOffset)};
            optionalCursorOffset = optionalSelection->second;
        }
    }
}

void TextEditUiNode::onKeyboardInputWhileFocused(
    KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    TextUiNode::onKeyboardInputWhileFocused(button, modifiers, bIsPressedDown);

    if (!bIsPressedDown) {
        return;
    }

    if (!optionalCursorOffset.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" expected to have a cursor offset already prepared", getNodeName()));
    }

    if (button == KeyboardButton::ENTER) {
        auto sText = std::string(getText());
        sText.insert(*optionalCursorOffset, "\n");
        setText(sText);

        (*optionalCursorOffset) += 1;
    } else if (button == KeyboardButton::BACKSPACE) {
        if (optionalSelection.has_value()) {
            auto sText = std::string(getText());
            sText.erase(optionalSelection->first, optionalSelection->second - optionalSelection->first);
            setText(sText);

            optionalCursorOffset = optionalSelection->first;
            optionalSelection = {};
        } else if ((*optionalCursorOffset) > 0) {
            auto sText = std::string(getText());
            sText.erase(*optionalCursorOffset - 1, 1);
            setText(sText);

            (*optionalCursorOffset) -= 1;
        }
    } else if (button == KeyboardButton::RIGHT) {
        optionalCursorOffset = std::min(*optionalCursorOffset + 1, getText().size());
    } else if (button == KeyboardButton::LEFT && (*optionalCursorOffset) > 0) {
        (*optionalCursorOffset) -= 1;
    }
}

void TextEditUiNode::onKeyboardInputTextCharacterWhileFocused(const std::string& sTextCharacter) {
    TextUiNode::onKeyboardInputTextCharacterWhileFocused(sTextCharacter);

    if (!optionalCursorOffset.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" expected to have a cursor offset already prepared", getNodeName()));
    }

    auto sText = std::string(getText());
    sText.insert(*optionalCursorOffset, sTextCharacter);
    setText(sText);

    (*optionalCursorOffset) += 1;
}

void TextEditUiNode::onGainedFocus() {
    TextUiNode::onGainedFocus();

    optionalCursorOffset = getText().size();
}

void TextEditUiNode::onLostFocus() {
    TextUiNode::onLostFocus();

    optionalCursorOffset = {};
}

size_t TextEditUiNode::convertMouseCursorPosToTextOffset() {
    const auto [iWindowWidth, iWindowHeight] = getGameInstanceWhileSpawned()->getWindow()->getWindowSize();
    const auto [iCursorX, iCursorY] = getGameInstanceWhileSpawned()->getWindow()->getCursorPosition();

    const auto mouseCursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWindowWidth),
        1.0F - static_cast<float>(iCursorY) / static_cast<float>(iWindowHeight)); // flip Y

    const auto leftBottomPos = getPosition();
    const auto size = getSize();

    const auto cursorPos = glm::vec2(
        (mouseCursorPos.x - leftBottomPos.x) / size.x, (mouseCursorPos.y - leftBottomPos.y) / size.y);

    // Determine after which character to put a cursor.
    const auto textScaleFullscreen = getTextHeight() / FontManager::getFontHeightToLoad();
    const auto textHeightOnFullscreen = FontManager::getFontHeightToLoad() * textScaleFullscreen;
    const auto textHeight = textHeightOnFullscreen / size.y;
    const auto lineSpacing = getTextLineSpacing() * textHeight;
    const auto sizeInPixels = glm::vec2(size.x * iWindowWidth, size.y * iWindowHeight);
    const auto sText = getText();

    auto& mtxLoadedGlyphs = getGameInstanceWhileSpawned()->getRenderer()->getFontManager().getLoadedGlyphs();
    std::scoped_lock guard(mtxLoadedGlyphs.first);

    // Prepare a placeholder glyph for unknown glyphs.
    const auto placeHolderGlythIt = mtxLoadedGlyphs.second.find('?');
    if (placeHolderGlythIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException("can't find a glyph for `?`");
    }

    glm::vec2 currentPos = glm::vec2(0.0F, 1.0F); // top-left corner of the UI node
    size_t iOutputOffset = sText.size();          // put after text by default

    // Switch to the first row of text.
    currentPos.y -= textHeight;

    for (size_t iCharIndex = 0; iCharIndex < sText.size(); iCharIndex++) {
        const char& character = sText[iCharIndex];

        // Handle new line.
        if (character == '\n' && getHandleNewLineChars()) {
            if (cursorPos.y >= currentPos.y && cursorPos.y <= currentPos.y + textHeight + lineSpacing &&
                cursorPos.x >= currentPos.x) {
                // The user clicked after the text is ended on this line.
                iOutputOffset = iCharIndex;
                break;
            }

            currentPos.x = 0.0F;
            currentPos.y -= textHeight + lineSpacing;
            continue;
        }

        // Get glyph.
        auto charIt = mtxLoadedGlyphs.second.find(character);
        if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
            charIt = placeHolderGlythIt;
        }
        const auto& glyph = charIt->second;

        const float distanceToNextGlyph =
            (glyph.advance >> 6) / // NOLINT: bitshift by 6 to get value in pixels (2^6 = 64)
            sizeInPixels.x * textScaleFullscreen;
        const float glyphWidth = std::max(
            static_cast<float>(glyph.size.x) / sizeInPixels.x * textScaleFullscreen, distanceToNextGlyph);

        // Handle word wrap.
        if (getIsWordWrapEnabled() && (currentPos.x + distanceToNextGlyph > 1.0F)) {
            if (cursorPos.y >= currentPos.y && cursorPos.y <= currentPos.y + textHeight + lineSpacing &&
                cursorPos.x >= currentPos.x) {
                // The user clicked after the text is ended on this line.
                iOutputOffset = iCharIndex;
                break;
            }

            // Switch to a new line.
            currentPos.x = 0.0F;
            currentPos.y -= textHeight + lineSpacing;
        }

        if (cursorPos.y >= currentPos.y && cursorPos.y <= currentPos.y + textHeight + lineSpacing &&
            cursorPos.x >= currentPos.x && cursorPos.x <= currentPos.x + glyphWidth) {
            iOutputOffset = iCharIndex;
            break;
        }

        // Switch to next glyph.
        currentPos.x += distanceToNextGlyph;
    }

    return iOutputOffset;
}
