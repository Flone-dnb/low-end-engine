#include "render/LightSourceManager.h"

// Custom.
#include "render/shader/LightSourceShaderArray.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/light/PointLightNode.h"
#include "render/shader/ShaderArrayIndexManager.h"
#include "render/LightSourceLimits.hpp"
#include "render/wrapper/Texture.h"
#include "render/GpuResourceManager.h"

// External.
#include "glad/glad.h"

LightSourceManager::~LightSourceManager() {}

LightSourceManager::LightSourceManager() {
    // Create array of directional lights.
    pDirectionalLightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(DirectionalLightNode::ShaderProperties),
        MAX_DIRECTIONAL_LIGHT_COUNT,
        "DirectionalLights",
        "iDirectionalLightCount"));

    // Create array of spotlights.
    pSpotlightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(SpotlightNode::ShaderProperties),
        MAX_SPOT_LIGHT_COUNT,
        "Spotlights",
        "iSpotlightCount"));
    pSpotShadowArrayIndexManager = std::unique_ptr<ShaderArrayIndexManager>(
        new ShaderArrayIndexManager("spotlight shadow maps", MAX_SPOT_LIGHT_COUNT));
    pSpotlightShadowMapArray =
        GpuResourceManager::createTextureArray(512, 512, GL_DEPTH_COMPONENT16, MAX_SPOT_LIGHT_COUNT, true);

    // Create array of spotlights.
    pPointLightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(PointLightNode::ShaderProperties),
        MAX_POINT_LIGHT_COUNT,
        "PointLights",
        "iPointLightCount"));
}

void LightSourceManager::setAmbientLightColor(const glm::vec3& color) { ambientLightColor = color; }
