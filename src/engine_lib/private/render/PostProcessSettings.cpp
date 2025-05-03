#include "render/PostProcessSettings.h"

// Standard.
#include <algorithm>

// Custom.
#include "render/GpuResourceManager.h"
#include "misc/Profiler.hpp"
#include "game/camera/CameraProperties.h"

// External.
#include "glad/glad.h"

void DistanceFogSettings::setStartDistance(float distance) {
    startDistance = std::clamp(distance, 0.01F, 0.99F); // NOLINT: to avoid corner cases in shaders
}

void DistanceFogSettings::setColor(const glm::vec3& color) { this->color = color; }

void PostProcessSettings::setAmbientLightColor(const glm::vec3& color) { ambientLightColor = color; }

void PostProcessSettings::setDistanceFogSettings(const std::optional<DistanceFogSettings>& settings) {
    distanceFogSettings = settings;
}

PostProcessSettings::PostProcessSettings(
    ShaderManager* pShaderManager, unsigned int iWidth, unsigned int iHeight) {
    pShaderProgram = pShaderManager->getShaderProgram(
        "engine/shaders/postprocessing/PostProcessingQuad.vert.glsl",
        "engine/shaders/postprocessing/PostProcessing.frag.glsl",
        ShaderProgramUsage::OTHER);

    pFramebuffer = GpuResourceManager::createFramebuffer(iWidth, iHeight, GL_RGB8, 0);
}

void PostProcessSettings::drawPostProcessing(
    const ScreenQuadGeometry& fullscreenQuadGeometry,
    const Framebuffer& readFramebuffer,
    CameraProperties* pCameraProperties) {
    PROFILE_FUNC

    // Set framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->getFramebufferId());
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(pShaderProgram->getShaderProgramId());

    glDisable(GL_DEPTH_TEST);
    {
        // Bind textures on which our scene was rendered.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, readFramebuffer.getColorTextureId());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, readFramebuffer.getDepthStencilTextureId());

        // Set shader parameters.
        pShaderProgram->setFloatToShader("zNear", pCameraProperties->getNearClipPlaneDistance());
        pShaderProgram->setFloatToShader("zFar", pCameraProperties->getFarClipPlaneDistance());
        pShaderProgram->setBoolToShader("bIsDistanceFogEnabled", distanceFogSettings.has_value());
        if (distanceFogSettings.has_value()) {
            pShaderProgram->setVector3ToShader("distanceFogColor", distanceFogSettings->getColor());
            pShaderProgram->setFloatToShader(
                "distanceFogStartDistance", distanceFogSettings->getStartDistance());
        }

        // Draw.
        glBindVertexArray(fullscreenQuadGeometry.getVao().getVertexArrayObjectId());
        glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);

        // Reset texture slots.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glEnable(GL_DEPTH_TEST);
}
