#include "render/PostProcessManager.h"

// Standard.
#include <algorithm>

// Custom.
#include "render/GpuResourceManager.h"
#include "misc/Profiler.hpp"
#include "game/camera/CameraProperties.h"
#include "game/GameManager.h"
#include "game/Window.h"

// External.
#include "glad/glad.h"

void DistanceFogSettings::setFogRange(const glm::vec2& range) {
    fogRange.x = std::max(range.x, 0.0F);
    fogRange.y = std::max(range.y, fogRange.x);
}

void DistanceFogSettings::setColor(const glm::vec3& color) { this->color = color; }

void DistanceFogSettings::setFogHeightOnSky(float fogHeight) { fogHeightOnSky = std::max(0.0F, fogHeight); }

void PostProcessManager::setAmbientLightColor(const glm::vec3& color) { ambientLightColor = color; }

void PostProcessManager::setDistanceFogSettings(const std::optional<DistanceFogSettings>& settings) {
    distanceFogSettings = settings;
}

void PostProcessManager::setSkySettings(const std::optional<SkySettings>& settings) {
    skySettings = settings;
}

PostProcessManager::PostProcessManager(GameManager* pGameManager) {
    pShaderProgram = pGameManager->getRenderer()->getShaderManager().getShaderProgram(
        "engine/shaders/postprocessing/PostProcessingQuad.vert.glsl",
        "engine/shaders/postprocessing/PostProcessing.frag.glsl");

    onWindowSizeChanged(pGameManager->getWindow());
}

void PostProcessManager::onWindowSizeChanged(Window* pWindow) {
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();
    pFramebuffer = GpuResourceManager::createFramebuffer(iWindowWidth, iWindowHeight, GL_RGB8, 0);
}

void PostProcessManager::drawPostProcessing(
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
        {
            // Distance fog.
            pShaderProgram->setBoolToShader("bIsDistanceFogEnabled", distanceFogSettings.has_value());
            if (distanceFogSettings.has_value()) {
                pShaderProgram->setVector3ToShader("distanceFogColor", distanceFogSettings->getColor());
                pShaderProgram->setVector2ToShader("distanceFogRange", distanceFogSettings->getFogRange());
                pShaderProgram->setFloatToShader("fogHeightOnSky", distanceFogSettings->getFogHeightOnSky());
            }

            // Procedural sky.
            pShaderProgram->setBoolToShader("bIsSkyEnabled", skySettings.has_value());
            if (skySettings.has_value()) {
                pShaderProgram->setVector3ToShader("skyColorAboveHorizon", skySettings->colorAboveHorizon);
                pShaderProgram->setVector3ToShader("skyColorOnHorizon", skySettings->colorOnHorizon);
                pShaderProgram->setVector3ToShader("skyColorBelowHorizon", skySettings->colorBelowHorizon);
            }

            pShaderProgram->setMatrix4ToShader(
                "invProjMatrix", pCameraProperties->getInverseProjectionMatrix());
            pShaderProgram->setMatrix4ToShader("invViewMatrix", pCameraProperties->getInverseViewMatrix());
            pShaderProgram->setVector3ToShader("cameraDirection", pCameraProperties->getForwardDirection());
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
