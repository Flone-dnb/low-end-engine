#include "render/UiManager.h"

// Standard.
#include <format>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "render/ShaderManager.h"
#include "render/Renderer.h"
#include "misc/Error.h"
#include "render/font/FontManager.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/wrapper/Texture.h"
#include "game/Window.h"
#include "render/GpuResourceManager.h"

// External.
#include "glad/glad.h"

#define ADD_NODE_TO_RENDERING(nodeType)                                                                      \
    /** Find an array of nodes to add the node to according to the node's depth. */                          \
    std::unordered_set<nodeType*>* pNodeArray = nullptr;                                                     \
    const auto iNodeDepth = pNode->getNodeDepthWhileSpawned();                                               \
    for (size_t i = 0; i < vNodesByDepth.size(); i++) {                                                      \
        if (vNodesByDepth[i].first == iNodeDepth) {                                                          \
            pNodeArray = &vNodesByDepth[i].second;                                                           \
            break;                                                                                           \
        } else if (vNodesByDepth[i].first > iNodeDepth) { /** NOLINT: else after break */                    \
            break;                                                                                           \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    if (pNodeArray == nullptr) {                                                                             \
        vNodesByDepth.push_back({iNodeDepth, {pNode}});                                                      \
        std::sort(                                                                                           \
            vNodesByDepth.begin(),                                                                           \
            vNodesByDepth.end(),                                                                             \
            [](const std::pair<size_t, std::unordered_set<nodeType*>>& left,                                 \
               const std::pair<size_t, std::unordered_set<nodeType*>>& right) -> bool {                      \
                return left.first < right.first;                                                             \
            });                                                                                              \
    } else {                                                                                                 \
        const auto [it, isAdded] = pNodeArray->insert(pNode);                                                \
        if (!isAdded) [[unlikely]] {                                                                         \
            Error::showErrorAndThrowException(                                                               \
                std::format("node \"{}\" is already added", pNode->getNodeName()));                          \
        }                                                                                                    \
    }

#define REMOVE_NODE_FROM_RENDERING(nodeType)                                                                 \
    /** Find an array of nodes to remove the node from node's depth. */                                      \
    std::optional<size_t> optionalArrayIndex;                                                                \
    const auto iNodeDepth = pNode->getNodeDepthWhileSpawned();                                               \
    for (size_t i = 0; i < vNodesByDepth.size(); i++) {                                                      \
        if (vNodesByDepth[i].first == iNodeDepth) {                                                          \
            optionalArrayIndex = i;                                                                          \
            break;                                                                                           \
        } else if (vNodesByDepth[i].first > iNodeDepth) { /** NOLINT: else after break */                    \
            break;                                                                                           \
        }                                                                                                    \
    }                                                                                                        \
    if (!optionalArrayIndex.has_value()) [[unlikely]] {                                                      \
        Error::showErrorAndThrowException(                                                                   \
            std::format(                                                                                     \
                "unable to find the node \"{}\" with depth {} to remove from rendering",                     \
                pNode->getNodeName(),                                                                        \
                iNodeDepth));                                                                                \
    }                                                                                                        \
    const auto iArrayIndex = *optionalArrayIndex;                                                            \
                                                                                                             \
    vNodesByDepth[iArrayIndex].second.erase(pNode);                                                          \
    if (vNodesByDepth[iArrayIndex].second.empty()) {                                                         \
        vNodesByDepth.erase(vNodesByDepth.begin() + iArrayIndex);                                            \
    }

void UiManager::onNodeSpawning(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(TextUiNode);

    if (mtxData.second.pTextShaderProgram == nullptr) {
        // Load shader.
        mtxData.second.pTextShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl",
            "engine/shaders/ui/TextNode.frag.glsl",
            ShaderProgramUsage::OTHER);
    }
}

void UiManager::onNodeSpawning(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(RectUiNode);

    if (mtxData.second.pRectShaderProgram == nullptr) {
        // Load shader.
        mtxData.second.pRectShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl",
            "engine/shaders/ui/RectUiNode.frag.glsl",
            ShaderProgramUsage::OTHER);
    }
}

void UiManager::onSpawnedNodeChangedVisibility(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (pNode->isVisible()) {
        ADD_NODE_TO_RENDERING(TextUiNode);
    } else {
        REMOVE_NODE_FROM_RENDERING(TextUiNode);
    }
}

void UiManager::onSpawnedNodeChangedVisibility(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (pNode->isVisible()) {
        ADD_NODE_TO_RENDERING(RectUiNode);
    } else {
        REMOVE_NODE_FROM_RENDERING(RectUiNode);
    }
}

void UiManager::onNodeDespawning(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (!pNode->isVisible()) {
        return;
    }

    REMOVE_NODE_FROM_RENDERING(TextUiNode);

    if (vNodesByDepth.empty()) {
        mtxData.second.pTextShaderProgram = nullptr;
    }
}

void UiManager::onNodeDespawning(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (!pNode->isVisible()) {
        return;
    }

    REMOVE_NODE_FROM_RENDERING(RectUiNode);

    if (vNodesByDepth.empty()) {
        mtxData.second.pRectShaderProgram = nullptr;
    }
}

void UiManager::onNodeChangedDepth(UiNode* pTargetNode) {
    std::scoped_lock guard(mtxData.first);

    if (!pTargetNode->isVisible()) {
        return;
    }

    if (auto pNode = dynamic_cast<TextUiNode*>(pTargetNode)) {
        auto& vNodesByDepth =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;
        {
            REMOVE_NODE_FROM_RENDERING(TextUiNode);
        }
        {
            ADD_NODE_TO_RENDERING(TextUiNode);
        }
    } else if (auto pNode = dynamic_cast<RectUiNode*>(pTargetNode)) {
        auto& vNodesByDepth =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;
        {
            REMOVE_NODE_FROM_RENDERING(RectUiNode);
        }
        {
            ADD_NODE_TO_RENDERING(RectUiNode);
        }
    } else [[unlikely]] {
        Error::showErrorAndThrowException("unhandled case");
    }

#if defined(WIN32) && defined(DEBUG)
    static_assert(
        sizeof(Data::SpawnedVisibleUiNodes) == 48, "add new variables here"); // NOLINT: current size
#endif
}

UiManager::UiManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    uiProjMatrix = glm::ortho(0.0F, static_cast<float>(iWidth), 0.0F, static_cast<float>(iHeight));

    mtxData.second.pScreenQuadGeometry = GpuResourceManager::createQuad(true);
}

void UiManager::drawUi(unsigned int iDrawFramebufferId) {
    PROFILE_FUNC;

    glBindFramebuffer(GL_FRAMEBUFFER, iDrawFramebufferId);

    std::scoped_lock guard(mtxData.first);

    glDisable(GL_DEPTH_TEST);
    for (size_t i = 0; i < mtxData.second.vSpawnedVisibleNodes.size(); i++) {
        drawRectNodes(i);
        drawTextNodes(i);
    }
    glEnable(GL_DEPTH_TEST);
}

void UiManager::drawRectNodes(size_t iLayer) {
    std::scoped_lock guard(mtxData.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vRectNodes;
    if (vNodesByDepth.empty()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

        // Set shader program.
        auto& pShaderProgram = mtxData.second.pRectShaderProgram;
        if (pShaderProgram == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the shader to be loaded at this point");
        }
        glUseProgram(pShaderProgram->getShaderProgramId());

        glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

        pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pRectNode : nodes) {
                auto pos = pRectNode->getPosition();
                auto size = pRectNode->getSize();

                // Set shader parameters.
                pShaderProgram->setVector4ToShader("color", pRectNode->getColor());
                if (pRectNode->pTexture != nullptr) {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, pRectNode->pTexture->getTextureId());
                } else {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", false);
                }

                pos = glm::vec2(pos.x * iWindowWidth, pos.y * iWindowHeight);
                size = glm::vec2(size.x * iWindowWidth, size.y * iWindowHeight);

                // Update VBO.
                const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                    glm::vec4(pos.x, pos.y + size.y, 0.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y, 1.0F, 1.0F),
                    glm::vec4(pos.x, pos.y, 0.0F, 1.0F),

                    glm::vec4(pos.x, pos.y + size.y, 0.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y + size.y, 1.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y, 1.0F, 1.0F)};

                // Copy new vertex data to VBO.
                glBindBuffer(
                    GL_ARRAY_BUFFER, mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Render quad.
                glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawTextNodes(size_t iLayer) {
    auto& mtxLoadedGlyphs = pRenderer->getFontManager().getLoadedGlyphs();
    std::scoped_lock guard(mtxData.first, mtxLoadedGlyphs.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vTextNodes;
    if (vNodesByDepth.empty()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

        // Set shader program.
        auto& pShaderProgram = mtxData.second.pTextShaderProgram;
        if (pShaderProgram == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the shader to be loaded at this point");
        }
        glUseProgram(pShaderProgram->getShaderProgramId());

        glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

        pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);
        glActiveTexture(GL_TEXTURE0); // glyph's bitmap

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pTextNode : nodes) {
                // Prepare some variables.
                const auto textRelativePos = pTextNode->getPosition();

                float screenX = textRelativePos.x * iWindowWidth;
                float screenY = textRelativePos.y * iWindowHeight;
                const auto scale = pTextNode->getSize().y / FontManager::getFontHeightToLoad();

                const float textHeightInPixels = iWindowHeight * FontManager::getFontHeightToLoad() * scale;
                const float lineSpacingInPixels = pTextNode->getTextLineSpacing() * textHeightInPixels;

                // Set color.
                pShaderProgram->setVector4ToShader("textColor", pTextNode->getTextColor());

                // Render each character.
                for (const auto& character : pTextNode->getText()) {
                    // Handle new line.
                    if (character == '\n') {
                        screenY -= textHeightInPixels + lineSpacingInPixels;
                        screenX = textRelativePos.x * iWindowWidth;
                        continue;
                    }

                    // Get glyph.
                    auto charIt = mtxLoadedGlyphs.second.find(character);
                    if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                        // No glyph found for this character, use ? instead.
                        // DON'T log a warning - you will slow everything down due to log flushing.
                        charIt = mtxLoadedGlyphs.second.find('?');
                        if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                            Error::showErrorAndThrowException(
                                std::format(
                                    "unable to find loaded glyph to render the character \"{}\" and also "
                                    "can't find a glyph for `?` to render it instead",
                                    character));
                        }
                    }
                    const auto& glyph = charIt->second;

                    float xpos = screenX + glyph.bearing.x * scale;
                    float ypos = screenY - (glyph.size.y - glyph.bearing.y) * scale;

                    float width = static_cast<float>(glyph.size.x) * scale;
                    float height = static_cast<float>(glyph.size.y) * scale;

                    // Update VBO.
                    const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F),
                        glm::vec4(xpos, ypos, 0.0F, 1.0F),

                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos + height, 1.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F)};

                    glBindTexture(GL_TEXTURE_2D, glyph.pTexture->getTextureId());

                    // Copy new vertex data to VBO.
                    glBindBuffer(
                        GL_ARRAY_BUFFER,
                        mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    // Render quad.
                    glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);

                    // Advance current pos to the next glyph (note that advance is number of 1/64 pixels).
                    screenX +=
                        (glyph.advance >> 6) * // NOLINT: bitshift by 6 to get value in pixels (2^6 = 64)
                        scale;
                }
            }
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_BLEND);
}

UiManager::~UiManager() {
    std::scoped_lock guard(mtxData.first);

    size_t iNodeCount = 0;
    for (const auto& nodes : mtxData.second.vSpawnedVisibleNodes) {
        iNodeCount += nodes.getTotalNodeArray();
    }
    if (iNodeCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "UI manager is being destroyed but there are still {} spawned and visible nodes",
                iNodeCount));
    }
}
