#include "game/node/ui/TextUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/FontManager.h"
#include "render/Renderer.h"
#include "render/UiNodeManager.h"
#include "game/World.h"
#include "game/Window.h"
#include "render/UiRenderData.h"
#include "game/node/ui/TextEditUiNode.h"
#include "render/wrapper/Texture.h"

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
    setSize(glm::vec2(0.2F, 0.02F));
}
TextUiNode::~TextUiNode() {}

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

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TextUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TextUiNode>(); },
        std::move(variables));
}

void TextUiNode::setText(std::u16string_view sNewText) {
    sText = sNewText;

    if (!bIsCallingOnAfterTextChanged) {
        bIsCallingOnAfterTextChanged = true;
        onAfterTextChanged();
        bIsCallingOnAfterTextChanged = false;
    }

    updateRenderData();
}

void TextUiNode::setTextColor(const glm::vec4& color) {
    this->color = color;
    updateRenderData();
}

void TextUiNode::setTextHeight(float height) {
    this->textHeight = height;
    updateRenderData();
}

void TextUiNode::setTextLineSpacing(float lineSpacing) {
    this->lineSpacing = std::max(lineSpacing, 0.0F);
    updateRenderData();
}

void TextUiNode::setIsWordWrapEnabled(bool bIsEnabled) {
    bIsWordWrapEnabled = bIsEnabled;
    updateRenderData();
}

void TextUiNode::setHandleNewLineChars(bool bHandleNewLineChars) {
    this->bHandleNewLineChars = bHandleNewLineChars;
    updateRenderData();
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

    if (isRenderingAllowed() && isVisible()) {
        initRenderingHandle();
    }
}

void TextUiNode::initRenderingHandle() {
    // TODO: during this process of rewriting UI rendering we have to put this check because
    // these nodes are rendered using a different API
    if (dynamic_cast<TextEditUiNode*>(this) == nullptr) {
        if (pRenderingHandle != nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected the rendering handle to be invalid on node \"{}\"", getNodeName()));
        }

        auto& uiManager = getWorldWhileSpawned()->getUiNodeManager();

        // Add to rendering.
        pRenderingHandle = uiManager.addTextForRendering(getUiLayer());

        // Initialize render data.
        updateRenderData();
    }
}

void TextUiNode::updateRenderData() {
    if (pRenderingHandle == nullptr) {
        return;
    }

    auto& uiManager = getWorldWhileSpawned()->getUiNodeManager();

    auto renderDataGuard = uiManager.getTextRenderData(*pRenderingHandle);
    auto& data = renderDataGuard.getData();

    data.pos = getPosition();
    data.textColor = color;

    // Prepare glyph info.
    data.vGlyphs.clear();

    // If window size will change we will be notified and will re-run this code.
    const auto [iWindowWidth, iWindowHeight] = getGameInstanceWhileSpawned()->getWindow()->getWindowSize();

    auto& fontManager = getGameInstanceWhileSpawned()->getRenderer()->getFontManager();
    auto glyphGuard = fontManager.getGlyphs();

    const float glyphScale = textHeight / fontManager.getFontHeightToLoad();
    const float glyphHeight = fontManager.getFontHeightToLoad() * glyphScale;
    const float glyphLineSpacing = lineSpacing * glyphHeight;

    const auto maxXForWordWrap = getSize().x;
    const auto yEndPos = getSize().y;

    auto offset = glm::vec2(0.0F);

    // Switch to the first row of text.
    offset.y += glyphHeight;

    for (const auto& character : sText) {
        // Handle new line.
        if (character == '\n' && bHandleNewLineChars) {
            // Switch to a new line.
            offset.y += glyphHeight + glyphLineSpacing;
            offset.x = 0.0F;

            if (offset.y > yEndPos) {
                break;
            }

            continue; // don't render \n
        }

        const auto& glyph = glyphGuard.getGlyph(character);

        const float distanceToNextGlyph =
            static_cast<float>(
                static_cast<double>(glyph.advance >> 6) // bitshift by 6 to get value in pixels (2^6 = 64)
                / static_cast<double>(iWindowWidth)) *
            glyphScale;

        // Handle word wrap.
        // TODO: do per-character wrap for now, rework later
        if (offset.x + distanceToNextGlyph > maxXForWordWrap) {
            if (bIsWordWrapEnabled) {
                // Switch to a new line.
                offset.y += glyphHeight + glyphLineSpacing;
                offset.x = 0.0F;

                if (offset.y > yEndPos) {
                    break;
                }
            } else {
                break;
            }
        }

        // Space character has 0 width so don't submit any rendering.
        if (glyph.size.x != 0) {
            const auto relativePos = glm::vec2(
                offset.x + static_cast<float>(
                               static_cast<double>(glyph.bearing.x) / static_cast<double>(iWindowWidth)) *
                               glyphScale,
                offset.y - static_cast<float>(
                               static_cast<double>(glyph.bearing.y) / static_cast<double>(iWindowHeight)) *
                               glyphScale);

            data.vGlyphs.push_back(GlythRenderData{
                .relativePos = relativePos,
                .screenSize = glm::vec2(
                    static_cast<float>(glyph.size.x) * glyphScale,
                    static_cast<float>(glyph.size.y) * glyphScale),
                .iTextureId = glyph.pTexture->getTextureId()});
        }

        // Switch to next glyph.
        offset.x += distanceToNextGlyph;
    }
}

void TextUiNode::onDespawning() {
    UiNode::onDespawning();

    // Remove from rendering (if was rendered).
    pRenderingHandle = nullptr;
}

void TextUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    if (isSpawned()) {
        if (isRenderingAllowed() && isVisible()) {
            initRenderingHandle();
        } else {
            pRenderingHandle = nullptr;
        }
    }
}

void TextUiNode::onWindowSizeChanged() {
    UiNode::onWindowSizeChanged();

    updateRenderData();
}

void TextUiNode::onAfterPositionChanged() {
    UiNode::onAfterPositionChanged();

    updateRenderData();
}

void TextUiNode::onAfterSizeChanged() {
    UiNode::onAfterSizeChanged();

    updateRenderData();
}

void TextUiNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    UiNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    if (isSpawned() && pRenderingHandle != nullptr) {
        // Re-add to rendering with new depth (was calculated in the parent).
        pRenderingHandle = nullptr;
        initRenderingHandle();
    }
}

void TextUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    Error::showErrorAndThrowException(
        std::format("text ui nodes can't have child nodes (text node \"{}\")", getNodeName()));
}
