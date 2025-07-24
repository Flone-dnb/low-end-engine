#include "game/node/ui/TextEditUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "game/Window.h"
#include "render/FontManager.h"
#include "game/World.h"
#include "game/camera/CameraManager.h"

// External.
#include "nameof.hpp"
#include "utf/utf.hpp"

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

void TextEditUiNode::setOnTextChanged(const std::function<void(std::u16string_view)>& onTextChanged) {
    this->onTextChanged = onTextChanged;
}

void TextEditUiNode::setOnEnterPressed(const std::function<void(std::u16string_view)>& onEnterPressed) {
    this->onEnterPressed = onEnterPressed;
}

void TextEditUiNode::setTextSelectionColor(const glm::vec4& textSelectionColor) {
    this->textSelectionColor = textSelectionColor;
}

void TextEditUiNode::onAfterDeserialized() {
    TextUiNode::onAfterDeserialized();

    setIsReceivingInput(!bIsReadOnly);
}

bool TextEditUiNode::onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    TextUiNode::onMouseButtonPressedOnUiNode(button, modifiers);

    if (button != MouseButton::LEFT) {
        return true;
    }

    optionalCursorOffset = convertCursorPosToTextOffset();
    optionalSelection = {};

    bIsTextSelectionStarted = true;
    setFocused();

    return true;
}

bool TextEditUiNode::onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    TextUiNode::onMouseButtonReleasedOnUiNode(button, modifiers);

    endTextSelection();

    return true;
}

void TextEditUiNode::onMouseLeft() {
    TextUiNode::onMouseLeft();

    bIsTextSelectionStarted = false;
}

void TextEditUiNode::onMouseMove(double xOffset, double yOffset) {
    TextUiNode::onMouseMove(xOffset, yOffset);

    if (bIsTextSelectionStarted) {
        // Don't end selection but create a temporary selection to display.
        const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
        if (!optCursorPos.has_value()) {
            Error::showErrorAndThrowException("expected the cursor to be in the viewport");
        }
        const auto cursorPos = *optCursorPos;

        const auto pos = getPosition();
        if (cursorPos.x < pos.x || cursorPos.y < pos.y) {
            // The cursor just stopped hovering other this node.
            bIsTextSelectionStarted = false;
            return;
        }

        const auto iCursorOffset = convertCursorPosToTextOffset();
        if (optionalCursorOffset.has_value() && iCursorOffset != *optionalCursorOffset) {
            optionalSelection = {
                std::min(iCursorOffset, *optionalCursorOffset),
                std::max(iCursorOffset, *optionalCursorOffset)};
        }
    }
}

void TextEditUiNode::endTextSelection() {
    bIsTextSelectionStarted = false;

    const auto iCursorOffset = convertCursorPosToTextOffset();
    if (optionalCursorOffset.has_value() && iCursorOffset != *optionalCursorOffset) {
        optionalSelection = {
            std::min(iCursorOffset, *optionalCursorOffset), std::max(iCursorOffset, *optionalCursorOffset)};
        optionalCursorOffset = iCursorOffset;
    }
}

void TextEditUiNode::changeText(std::u16string_view sNewText) {
    bIsChangingText = true;
    setText(sNewText);
    bIsChangingText = false;
}

void TextEditUiNode::onKeyboardButtonPressedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) {
    TextUiNode::onKeyboardButtonPressedWhileFocused(button, modifiers);

    if (!optionalCursorOffset.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" expected to have a cursor offset already prepared", getNodeName()));
    }

    if (button == KeyboardButton::ENTER && getHandleNewLineChars()) {
        auto sText = std::u16string(getText());
        sText.insert(*optionalCursorOffset, u"\n");
        changeText(sText);

        (*optionalCursorOffset) += 1;

        if (onTextChanged) {
            onTextChanged(sText);
        }
    } else if (button == KeyboardButton::BACKSPACE) {
        if (optionalSelection.has_value()) {
            auto sText = std::u16string(getText());
            sText.erase(optionalSelection->first, optionalSelection->second - optionalSelection->first);
            changeText(sText);

            optionalCursorOffset = optionalSelection->first;
            optionalSelection = {};
        } else if ((*optionalCursorOffset) > 0) {
            auto sText = std::u16string(getText());
            sText.erase(*optionalCursorOffset - 1, 1);
            changeText(sText);

            (*optionalCursorOffset) -= 1;
        }

        if (onTextChanged) {
            onTextChanged(getText());
        }
    } else if (button == KeyboardButton::RIGHT) {
        optionalCursorOffset = std::min(*optionalCursorOffset + 1, getText().size());
    } else if (button == KeyboardButton::LEFT && (*optionalCursorOffset) > 0) {
        (*optionalCursorOffset) -= 1;
    } else if (
        (button == KeyboardButton::UP || button == KeyboardButton::DOWN) &&
        optionalCursorOffset.has_value() && *optionalCursorOffset > 0) {
        // TODO: using a very simple (and inaccurate) approach.
        const auto sText = getText();
        bool bFoundLineStart = false;
        size_t iCharCountFromLineStart = 0;
        size_t iCurrentCharIndex = 0;
        for (iCurrentCharIndex = *optionalCursorOffset; iCurrentCharIndex > 0; iCurrentCharIndex--) {
            if (sText[iCurrentCharIndex] == '\n') {
                bFoundLineStart = true;
                break;
            }

            iCharCountFromLineStart += 1;
        }
        if (iCurrentCharIndex == 0) {
            bFoundLineStart = true;
        }

        if (bFoundLineStart) {
            if (button == KeyboardButton::UP && iCurrentCharIndex > 0) {
                iCurrentCharIndex -= 1;
                for (; iCurrentCharIndex > 0; iCurrentCharIndex--) {
                    if (sText[iCurrentCharIndex] == '\n') {
                        *optionalCursorOffset = iCurrentCharIndex + iCharCountFromLineStart;
                        break;
                    }
                }
                if (iCurrentCharIndex == 0) {
                    *optionalCursorOffset = iCharCountFromLineStart;
                }
            } else {
                for (iCurrentCharIndex = *optionalCursorOffset; iCurrentCharIndex < sText.size();
                     iCurrentCharIndex++) {
                    if (sText[iCurrentCharIndex] == '\n') {
                        *optionalCursorOffset = iCurrentCharIndex + 1 + iCharCountFromLineStart;
                        break;
                    }
                }
            }
        }
    }

    if (onEnterPressed && button == KeyboardButton::ENTER) {
        onEnterPressed(getText());
    }
}

void TextEditUiNode::onAfterTextChanged() {
    TextUiNode::onAfterTextChanged();

    if (bIsChangingText) {
        return;
    }

    if (optionalCursorOffset.has_value()) {
        (*optionalCursorOffset) = getText().size();
    }
    optionalSelection = {};
}

void TextEditUiNode::onKeyboardInputTextCharacterWhileFocused(const std::string& sTextCharacter) {
    TextUiNode::onKeyboardInputTextCharacterWhileFocused(sTextCharacter);

    if (!optionalCursorOffset.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" expected to have a cursor offset already prepared", getNodeName()));
    }

    if (optionalSelection.has_value()) {
        // Delete selected text.
        auto sText = std::u16string(getText());
        sText.erase(optionalSelection->first, optionalSelection->second - optionalSelection->first);
        changeText(sText);

        optionalCursorOffset = optionalSelection->first;
        optionalSelection = {};
    }

    auto sText = std::u16string(getText());
    if (*optionalCursorOffset > sText.size()) [[unlikely]] {
        // This means we have error somewhere else.
        Error::showErrorAndThrowException("text cursor is out of bounds");
    }

    sText.insert(*optionalCursorOffset, utf::as_u16(sTextCharacter));
    changeText(sText);

    (*optionalCursorOffset) += 1;

    if (onTextChanged) {
        onTextChanged(sText);
    }
}

void TextEditUiNode::onGainedFocus() {
    TextUiNode::onGainedFocus();

    optionalCursorOffset = getText().size();
}

void TextEditUiNode::onLostFocus() {
    TextUiNode::onLostFocus();

    optionalCursorOffset = {};
    optionalSelection = {};
    bIsTextSelectionStarted = false;
}

size_t TextEditUiNode::convertCursorPosToTextOffset() {
    const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        Error::showErrorAndThrowException("expected the cursor to be in the viewport");
    }
    const auto cursorPos = *optCursorPos;

    const auto pGameInstance = getGameInstanceWhileSpawned();

    const auto size = getSize();
    const auto textCursorPos = (cursorPos - getPosition()) / size;
    const auto [iWindowWidth, iWindowHeight] = pGameInstance->getWindow()->getWindowSize();

    auto& fontManager = pGameInstance->getRenderer()->getFontManager();

    // Determine after which character to put a cursor.
    const auto textScaleFullscreen = getTextHeight() / fontManager.getFontHeightToLoad();
    const auto textHeightOnFullscreen = fontManager.getFontHeightToLoad() * textScaleFullscreen;
    const auto textHeight = textHeightOnFullscreen / size.y;
    const auto lineSpacing = getTextLineSpacing() * textHeight;
    const auto sizeInPixels =
        glm::vec2(size.x * static_cast<float>(iWindowWidth), size.y * static_cast<float>(iWindowHeight));
    const auto sText = getText();

    auto glyphGuard = fontManager.getGlyphs();

    glm::vec2 localCurrentPos = glm::vec2(0.0F, 0.0F); // in range [0.0; 1.0]
    size_t iOutputTextOffset = sText.size();           // put cursor after text by default

    // Switch to the first row of text.
    localCurrentPos.y += textHeight;

    size_t iLinesToSkip = getCurrentScrollOffset();
    size_t iLineIndex = 0;
    for (size_t iCharIndex = 0; iCharIndex < sText.size(); iCharIndex++) {
        const auto& character = sText[iCharIndex];

        // Handle new line.
        if (character == '\n' && getHandleNewLineChars()) {
            if (textCursorPos.y >= localCurrentPos.y - (textHeight + lineSpacing) &&
                textCursorPos.y <= localCurrentPos.y && textCursorPos.x >= localCurrentPos.x) {
                // The user clicked after the text is ended on this line.
                iOutputTextOffset = iCharIndex;
                break;
            }

            localCurrentPos.x = 0.0F;
            if (iLineIndex >= iLinesToSkip) {
                localCurrentPos.y += textHeight + lineSpacing;
            }

            iLineIndex += 1;
            continue;
        }

        const auto& glyph = glyphGuard.getGlyph(character);

        const float distanceToNextGlyph =
            static_cast<float>(glyph.advance >> 6) / // bitshift by 6 to get value in pixels (2^6 = 64)
            sizeInPixels.x * textScaleFullscreen;
        const float glyphWidth = std::max(
            static_cast<float>(glyph.size.x) / sizeInPixels.x * textScaleFullscreen, distanceToNextGlyph);

        // Handle word wrap.
        if (getIsWordWrapEnabled() && (localCurrentPos.x + distanceToNextGlyph > 1.0F)) {
            if (textCursorPos.y >= localCurrentPos.y - (textHeight + lineSpacing) &&
                textCursorPos.y <= localCurrentPos.y && textCursorPos.x >= localCurrentPos.x) {
                // The user clicked after the text is ended on this line.
                iOutputTextOffset = iCharIndex;
                break;
            }

            // Switch to a new line.
            localCurrentPos.x = 0.0F;
            if (iLineIndex >= iLinesToSkip) {
                localCurrentPos.y += textHeight + lineSpacing;
            }

            iLineIndex += 1;
        }

        if (iLineIndex >= iLinesToSkip) {
            if (textCursorPos.y >= localCurrentPos.y - (textHeight + lineSpacing) &&
                textCursorPos.y <= localCurrentPos.y && textCursorPos.x >= localCurrentPos.x &&
                textCursorPos.x <= localCurrentPos.x + glyphWidth) {
                iOutputTextOffset = iCharIndex;
                break;
            }
        }

        // Switch to next glyph.
        localCurrentPos.x += distanceToNextGlyph;
    }

    return iOutputTextOffset;
}
