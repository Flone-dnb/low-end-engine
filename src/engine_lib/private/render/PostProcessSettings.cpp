#include "render/PostProcessSettings.h"

// Standard.
#include <algorithm>

// Custom.
#include "render/GpuResourceManager.h"

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

PostProcessSettings::PostProcessSettings(ShaderManager* pShaderManager) {
    pFullScreenQuad = GpuResourceManager::createQuad(false);
    pShaderProgram = pShaderManager->getShaderProgram(
        "engine/shaders/postprocessing/PostProcessing.vert.glsl",
        "engine/shaders/postprocessing/PostProcessing.frag.glsl",
        ShaderProgramUsage::OTHER);
}
