#include "render/UiManager.h"

// Standard.
#include <format>

// Custom.
#include "game/node/ui/TextNode.h"
#include "render/shader/ShaderManager.h"
#include "render/Renderer.h"
#include "misc/Error.h"
#include "render/font/FontManager.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/wrapper/Texture.h"
#include "game/Window.h"
#include "render/GpuResourceManager.h"

// External.
#include "nameof.hpp"
#include "glad/glad.h"

UiManager::~UiManager() {
    std::scoped_lock guard(mtxData.first);

    const auto iNodeCount = mtxData.second.spawnedVisibleTextNodes.size();
    if (iNodeCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("UI manager is being destroyed but there are still {} spawned nodes", iNodeCount));
    }
}

void UiManager::renderUi() {
    auto& mtxLoadedGlyphs = pRenderer->getFontManager().getLoadedGlyphs();
    std::scoped_lock guard(mtxData.first, mtxLoadedGlyphs.first);

    int depthFunc = 0;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glDepthFunc(GL_ALWAYS); // disable depth tests for UI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        // Render text.
        if (!mtxData.second.spawnedVisibleTextNodes.empty()) {
            const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

            // Set shader program.
            auto& pShaderProgram = mtxData.second.vLoadedShaders[static_cast<size_t>(UiShaderType::TEXT)];
            if (pShaderProgram == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected text shader to be loaded at this point");
            }
            glUseProgram(pShaderProgram->getShaderProgramId());

            // Set shader properties.
            pShaderProgram->setMatrix4ToShader(NAMEOF(projectionMatrix).c_str(), projectionMatrix);

            glActiveTexture(GL_TEXTURE0);                                                // glyph's bitmap
            glBindVertexArray(mtxData.second.pQuadVaoForText->getVertexArrayObjectId()); // quad vertices

            // Render all text nodes.
            for (const auto& pTextNode : mtxData.second.spawnedVisibleTextNodes) {
                // Prepare some variables.
                const auto textRelativePos = pTextNode->getPosition();

                float screenX = textRelativePos.x * iWindowWidth;
                float screenY = textRelativePos.y * iWindowHeight;
                const auto scale = pTextNode->getTextSize() / FontManager::getFontHeightToLoad();

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
                    const auto charIt = mtxLoadedGlyphs.second.find(character);
                    if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                        Error::showErrorAndThrowException(
                            std::format(
                                "unable to find loaded glyph to render the character \"{}\"", character));
                    }
                    const auto& glyph = charIt->second;

                    float xpos = screenX + glyph.bearing.x * scale;
                    float ypos = screenY - (glyph.size.y - glyph.bearing.y) * scale;

                    float width = static_cast<float>(glyph.size.x) * scale;
                    float height = static_cast<float>(glyph.size.y) * scale;

                    // Update VBO.
                    const std::array<glm::vec4, iTextQuadVertexCount> vVertices = {
                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F),
                        glm::vec4(xpos, ypos, 0.0F, 1.0F),

                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos + height, 1.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F)};

                    glBindTexture(GL_TEXTURE_2D, glyph.pTexture->getTextureId());

                    // Copy new vertex data to VBO.
                    glBindBuffer(GL_ARRAY_BUFFER, mtxData.second.pQuadVaoForText->getVertexBufferObjectId());
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    // Render quad.
                    glDrawArrays(GL_TRIANGLES, 0, iTextQuadVertexCount);

                    // Advance current pos to the next glyph (note that advance is number of 1/64 pixels).
                    screenX +=
                        (glyph.advance >> 6) * // NOLINT: bitshift by 6 to get value in pixels (2^6 = 64)
                        scale;
                }
            }

            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    glDisable(GL_BLEND);
    glDepthFunc(depthFunc);
}

void UiManager::onNodeSpawning(TextNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;

    if (pNode->isVisible()) {
        // Add to rendering.
        const auto [it, isAdded] = data.spawnedVisibleTextNodes.insert(pNode);
        if (!isAdded) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("node \"{}\" is already added", pNode->getNodeName()));
        }
    }

    auto& pShaderProgram = data.vLoadedShaders[static_cast<size_t>(UiShaderType::TEXT)];
    if (pShaderProgram == nullptr) {
        // Load shader.
        pShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/text/Text.vert.glsl",
            "engine/shaders/text/Text.frag.glsl",
            ShaderProgramUsage::OTHER);
    }
}

void UiManager::onSpawnedNodeChangedVisibility(TextNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;

    if (pNode->isVisible()) {
        // Add to rendering.
        const auto [it, isAdded] = data.spawnedVisibleTextNodes.insert(pNode);
        if (!isAdded) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("node \"{}\" is already added", pNode->getNodeName()));
        }
    } else {
        // Remove from rendering.
        const auto it = data.spawnedVisibleTextNodes.find(pNode);
        if (it == data.spawnedVisibleTextNodes.end()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("node \"{}\" is not found", pNode->getNodeName()));
        }
    }
}

void UiManager::onNodeDespawning(TextNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;

    if (pNode->isVisible()) {
        // Remove from rendering.
        const auto it = data.spawnedVisibleTextNodes.find(pNode);
        if (it == data.spawnedVisibleTextNodes.end()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("node \"{}\" is not found", pNode->getNodeName()));
        }
        data.spawnedVisibleTextNodes.erase(it);

        if (data.spawnedVisibleTextNodes.empty()) {
            auto& pShaderProgram = data.vLoadedShaders[static_cast<size_t>(UiShaderType::TEXT)];
            // Unload shader (if was loaded).
            pShaderProgram = nullptr;
        }
    }
}

UiManager::UiManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    projectionMatrix = glm::ortho(0.0F, static_cast<float>(iWidth), 0.0F, static_cast<float>(iHeight));

    std::scoped_lock guard(GpuResourceManager::mtx);

    // Create VAO quad for rendering text.
    unsigned int iVao = 0;
    unsigned int iVbo = 0;
    glGenVertexArrays(1, &iVao);
    glGenBuffers(1, &iVbo);

    glBindVertexArray(iVao);
    glBindBuffer(GL_ARRAY_BUFFER, iVbo);
    {
        // Allocate vertices, each vec4 will store screen space position (in XY) and UVs (in ZW).
        glBufferData(GL_ARRAY_BUFFER, iTextQuadVertexCount * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mtxData.second.pQuadVaoForText = std::unique_ptr<VertexArrayObject>(new VertexArrayObject(iVao, iVbo));
}
