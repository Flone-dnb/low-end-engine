#include "render/DebugDrawer.h"

#if defined(ENGINE_DEBUG_TOOLS)

// Custom.
#include "misc/Error.h"
#include "render/Renderer.h"
#include "render/ShaderManager.h"
#include "render/GpuResourceManager.h"
#include "misc/Profiler.hpp"
#include "game/geometry/PrimitiveMeshGenerator.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/FontManager.h"
#include "game/Window.h"
#include "render/wrapper/Texture.h"

// External.
#include "glad/glad.h"

DebugDrawer::DebugDrawer() {
    // Precalculate cube positions.
    {
        const auto cubeGeometry = PrimitiveMeshGenerator::createCube(1.0F);
        const auto& vVertices = cubeGeometry.getVertices();
        const auto& vIndices = cubeGeometry.getIndices();

        // Convert to just triangle positions.
        vCubePositions.reserve(vIndices.size());
        for (const auto& index : vIndices) {
            vCubePositions.push_back(vVertices[index].position);
        }
    }

    // Precalculate icosphere positions.
    {
        const float X = 0.525731112119133606f;
        const float Z = 0.850650808352039932f;
        const float N = 0.0f;

        const std::vector<glm::vec3> vVertices = {
            {-X, N, Z},
            {X, N, Z},
            {-X, N, -Z},
            {X, N, -Z},
            {N, Z, X},
            {N, Z, -X},
            {N, -Z, X},
            {N, -Z, -X},
            {Z, X, N},
            {-Z, X, N},
            {Z, -X, N},
            {-Z, -X, N}};

        static const std::vector<std::array<unsigned short, 3>> vTriangleIndices = {
            {0, 4, 1}, {0, 9, 4},  {9, 5, 4},  {4, 5, 8},  {4, 8, 1},  {8, 10, 1}, {8, 3, 10},
            {5, 3, 8}, {5, 2, 3},  {2, 7, 3},  {7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6},
            {0, 1, 6}, {6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5},  {7, 2, 11}};

        // I think that since we are a low-poly game engine it would be enough to have a low poly icosphere
        // so we won't subdivide the icosphere further.
        // Convert to just triangle positions.
        vIcospherePositions.reserve(vTriangleIndices.size() * 3);
        for (const auto& vTriangle : vTriangleIndices) {
            for (const auto& index : vTriangle) {
                vIcospherePositions.push_back(vVertices[index]);
            }
        }
    }
}

DebugDrawer::~DebugDrawer() {
    if (!bIsDestroyed) [[unlikely]] {
        Error::showErrorAndThrowException(
            "debug drawer is being destroyed but it still uses some render resources");
    }
}

DebugDrawer& DebugDrawer::get() {
    static DebugDrawer drawer;
    return drawer;
}

void DebugDrawer::destroy() {
    pMeshShaderProgram = nullptr;
    pScreenQuadGeometry = nullptr;
    textShaderInfo.pShaderProgram = nullptr;
    rectShaderInfo.pShaderProgram = nullptr;

    vMeshesToDraw.clear();
    vTextToDraw.clear();
    vRectsToDraw.clear();

    bIsDestroyed = true;
}

void DebugDrawer::drawCube(
    float size, const glm::vec3& worldPosition, float timeInSec, const glm::vec3& color) {
    drawMesh(
        get().vCubePositions, glm::translate(worldPosition) * glm::scale(glm::vec3(size)), timeInSec, color);
}

void DebugDrawer::drawSphere(
    float radius, const glm::vec3& worldPosition, float timeInSec, const glm::vec3& color) {
    drawMesh(get().vIcospherePositions, glm::translate(worldPosition), timeInSec, color);
}

void DebugDrawer::drawMesh(
    const std::vector<glm::vec3>& vTrianglePositions,
    const glm::mat4x4& worldMatrix,
    float timeInSec,
    const glm::vec3& color) {
    if (vTrianglePositions.size() % 3 != 0) [[unlikely]] {
        Error::showErrorAndThrowException("triangle positions array must store 3 positions per triangle");
    }

    // Prepare vertex buffer.
    std::vector<glm::vec3> vEdges;
    vEdges.reserve(vTrianglePositions.size() * 2); // 2 vertices to draw an edge of a triangle
    for (size_t i = 0; i < vTrianglePositions.size(); i += 3) {
        vEdges.push_back(vTrianglePositions[i]);
        vEdges.push_back(vTrianglePositions[i + 1]);

        vEdges.push_back(vTrianglePositions[i + 1]);
        vEdges.push_back(vTrianglePositions[i + 2]);

        vEdges.push_back(vTrianglePositions[i + 2]);
        vEdges.push_back(vTrianglePositions[i]);
    }

    auto pMeshVao =
        GpuResourceManager::createVertexArrayObject(static_cast<unsigned int>(vEdges.size()), false, vEdges);

    get().vMeshesToDraw.push_back(Mesh{
        .color = color, .worldMatrix = worldMatrix, .timeLeftSec = timeInSec, .pVao = std::move(pMeshVao)});
}

void DebugDrawer::drawLines(
    const std::vector<glm::vec3>& vLines,
    const glm::mat4x4& worldMatrix,
    float timeInSec,
    const glm::vec3& color) {
    if (vLines.size() % 2 != 0) [[unlikely]] {
        Error::showErrorAndThrowException("line positions array must store 2 positions per line");
    }

    auto pLinesVao =
        GpuResourceManager::createVertexArrayObject(static_cast<unsigned int>(vLines.size()), false, vLines);

    get().vMeshesToDraw.push_back(Mesh{
        .color = color, .worldMatrix = worldMatrix, .timeLeftSec = timeInSec, .pVao = std::move(pLinesVao)});
}

void DebugDrawer::drawText(
    const std::string& sText,
    float timeInSec,
    const glm::vec3& color,
    const std::optional<glm::vec2>& optForcePosition,
    float textHeight) {
    get().vTextToDraw.push_back(Text{
        .sText = sText,
        .textHeight = textHeight,
        .optForcePosition = optForcePosition,
        .timeLeftSec = timeInSec,
        .color = color});
}

void DebugDrawer::drawScreenRect(
    const glm::vec2& screenPos, const glm::vec2& screenSize, const glm::vec3& color, float timeInSec) {
    get().vRectsToDraw.push_back(ScreenRect{
        .screenPos = screenPos, .screenSize = screenSize, .timeLeftSec = timeInSec, .color = color});
}

void DebugDrawer::drawQuad(
    int iScreenPosUniform,
    int iScreenSizeUniform,
    int iClipRectUniform,
    int iWindowSizeUniform,
    glm::vec2 screenPos,
    glm::vec2 screenSize,
    unsigned int iWindowWidth,
    unsigned int iWindowHeight) const {
    const glm::vec2 windowSize =
        glm::vec2(static_cast<float>(iWindowWidth), static_cast<float>(iWindowHeight));

    const glm::vec4 clipRect = glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);

    glUniform2fv(iScreenPosUniform, 1, glm::value_ptr(screenPos));
    glUniform2fv(iScreenSizeUniform, 1, glm::value_ptr(screenSize));
    glUniform4fv(iClipRectUniform, 1, glm::value_ptr(clipRect));
    glUniform2fv(iWindowSizeUniform, 1, glm::value_ptr(windowSize));

    glDrawElements(GL_TRIANGLES, ScreenQuadGeometry::iIndexCount, GL_UNSIGNED_SHORT, nullptr);
}

void DebugDrawer::drawDebugObjects(
    Renderer* pRenderer, const glm::mat4& viewProjectionMatrix, float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    // Initialize resources if needed.
    if (pMeshShaderProgram == nullptr) {
        pMeshShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/debug/Mesh.vert.glsl", "engine/shaders/debug/Mesh.frag.glsl");
        iMeshProgramViewProjectionMatrixUniform =
            pMeshShaderProgram->getShaderUniformLocation("viewProjectionMatrix");

        textShaderInfo.pShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl", "engine/shaders/ui/TextNode.frag.glsl");

        // Cache uniform locations for text shader.
        auto& text = textShaderInfo;
        text.iScreenPosUniform = text.pShaderProgram->getShaderUniformLocation("screenPos");
        text.iScreenSizeUniform = text.pShaderProgram->getShaderUniformLocation("screenSize");
        text.iClipRectUniform = text.pShaderProgram->getShaderUniformLocation("clipRect");
        text.iWindowSizeUniform = text.pShaderProgram->getShaderUniformLocation("windowSize");

        rectShaderInfo.pShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl", "engine/shaders/ui/RectUiNode.frag.glsl");

        // Cache uniform locations for rect shader.
        auto& rect = rectShaderInfo;
        rect.iScreenPosUniform = rect.pShaderProgram->getShaderUniformLocation("screenPos");
        rect.iScreenSizeUniform = rect.pShaderProgram->getShaderUniformLocation("screenSize");
        rect.iClipRectUniform = rect.pShaderProgram->getShaderUniformLocation("clipRect");
        rect.iWindowSizeUniform = rect.pShaderProgram->getShaderUniformLocation("windowSize");

        // Prepare screen quad in [0.0; 1.0].
        std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount> vVertices = {
            ScreenQuadGeometry::VertexLayout{.position = glm::vec2(0.0F, 0.0F), .uv = glm::vec2(0.0F, 0.0F)},
            ScreenQuadGeometry::VertexLayout{.position = glm::vec2(0.0F, 1.0F), .uv = glm::vec2(0.0F, 1.0F)},
            ScreenQuadGeometry::VertexLayout{.position = glm::vec2(1.0F, 1.0F), .uv = glm::vec2(1.0F, 1.0F)},
            ScreenQuadGeometry::VertexLayout{.position = glm::vec2(1.0F, 0.0F), .uv = glm::vec2(1.0F, 0.0F)},
        };
        std::array<unsigned short, ScreenQuadGeometry::iIndexCount> vIndices = {0, 1, 2, 0, 2, 3};
        pScreenQuadGeometry = GpuResourceManager::createScreenQuad(vVertices, vIndices);
    }

    glDisable(GL_DEPTH_TEST);
    {
        // Prepare for drawing meshes.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(pMeshShaderProgram->getShaderProgramId());
        glUniformMatrix4fv(
            iMeshProgramViewProjectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));

        for (auto it = vMeshesToDraw.begin(); it != vMeshesToDraw.end();) {
            auto& mesh = *it;

            glBindVertexArray(mesh.pVao->getVertexArrayObjectId());

            pMeshShaderProgram->setMatrix4ToActiveProgram("worldMatrix", mesh.worldMatrix);
            pMeshShaderProgram->setVector3ToActiveProgram("meshColor", mesh.color);

            glDrawArrays(GL_LINES, 0, static_cast<int>(mesh.pVao->getVertexCount()));

            // Update state.
            mesh.timeLeftSec -= timeSincePrevFrameInSec;
            if (mesh.timeLeftSec < 0.0f) {
                it = vMeshesToDraw.erase(it);
                continue;
            }
            it++;
        }

        // Prepare for drawing screen rectangles.
        const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();
        glUseProgram(rectShaderInfo.pShaderProgram->getShaderProgramId());
        glBindVertexArray(pScreenQuadGeometry->getVao().getVertexArrayObjectId());

        for (auto it = vRectsToDraw.begin(); it != vRectsToDraw.end();) {
            auto& rect = *it;
            rectShaderInfo.pShaderProgram->setVector4ToActiveProgram("color", glm::vec4(rect.color, 1.0F));
            rectShaderInfo.pShaderProgram->setBoolToActiveProgram("bIsUsingTexture", false);

            glm::vec2 pos = rect.screenPos;
            glm::vec2 size = rect.screenSize;

            pos *= glm::vec2(static_cast<float>(iWindowWidth), static_cast<float>(iWindowHeight));
            size *= glm::vec2(static_cast<float>(iWindowWidth), static_cast<float>(iWindowHeight));

            drawQuad(
                rectShaderInfo.iScreenPosUniform,
                rectShaderInfo.iScreenSizeUniform,
                rectShaderInfo.iClipRectUniform,
                rectShaderInfo.iWindowSizeUniform,
                pos,
                size,
                iWindowWidth,
                iWindowHeight);

            // Update state.
            rect.timeLeftSec -= timeSincePrevFrameInSec;
            if (rect.timeLeftSec < 0.0f) {
                it = vRectsToDraw.erase(it);
                continue;
            }
            it++;
        }

        // Prepare for drawing text.
        auto& fontManager = pRenderer->getFontManager();
        auto glyphGuard = fontManager.getGlyphs();
        GL_CHECK_ERROR(glUseProgram(textShaderInfo.pShaderProgram->getShaderProgramId()));
        GL_CHECK_ERROR(glBindVertexArray(pScreenQuadGeometry->getVao().getVertexArrayObjectId()));
        glActiveTexture(GL_TEXTURE0); // glyph's bitmap

        // Prepare starting position for the first text (relative to screen's top-left corner).
        // x will be reset on every text so it's defined below
        float screenY = static_cast<float>(iWindowHeight) * 0.1F;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (auto it = vTextToDraw.begin(); it != vTextToDraw.end();) {
            auto& text = *it;

            float screenX = static_cast<float>(iWindowWidth) * 0.025F;
            if (text.optForcePosition.has_value()) {
                screenX = static_cast<float>(iWindowWidth) * text.optForcePosition->x;
            }

            textShaderInfo.pShaderProgram->setVector4ToActiveProgram("textColor", glm::vec4(text.color, 1.0F));

            const auto fontScale = text.textHeight / fontManager.getFontHeightToLoad();
            const float textHeightInPixels =
                static_cast<float>(iWindowHeight) * fontManager.getFontHeightToLoad() * fontScale;

            // Switch to the first row of text.
            const float autoScreenY = screenY; // save to restore later
            if (text.optForcePosition.has_value()) {
                screenY = static_cast<float>(iWindowHeight) * text.optForcePosition->y;
            }
            screenY += textHeightInPixels;

            // Draw each character.
            for (const auto& character : text.sText) {
                // Get glyph info.
                const auto& glyph = glyphGuard.getGlyph(static_cast<unsigned long>(character));
                const float distanceToNextGlyph =
                    static_cast<float>(
                        glyph.advance >> 6) * // bitshift by 6 to get value in pixels (2^6 = 64)
                    fontScale;

                glm::vec2 screenPos = glm::vec2(
                    screenX + static_cast<float>(glyph.bearing.x) * fontScale,
                    screenY - static_cast<float>(glyph.bearing.y) * fontScale);

                float width = static_cast<float>(glyph.size.x) * fontScale;
                float height = static_cast<float>(glyph.size.y) * fontScale;

                // Space character has 0 width so don't submit any rendering.
                if (glyph.size.x != 0) {
                    glBindTexture(GL_TEXTURE_2D, glyph.pTexture->getTextureId());
                    drawQuad(
                        textShaderInfo.iScreenPosUniform,
                        textShaderInfo.iScreenSizeUniform,
                        textShaderInfo.iClipRectUniform,
                        textShaderInfo.iWindowSizeUniform,
                        screenPos,
                        glm::vec2(width, height),
                        iWindowWidth,
                        iWindowHeight);
                }

                // Switch to next glyph.
                screenX += distanceToNextGlyph;
            }

            // Restore auto Y.
            if (text.optForcePosition.has_value()) {
                screenY = autoScreenY;
            }

            // Update state.
            text.timeLeftSec -= timeSincePrevFrameInSec;
            if (text.timeLeftSec < 0.0f) {
                it = vTextToDraw.erase(it);
                continue;
            }
            it++;
        }
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
}

#endif
