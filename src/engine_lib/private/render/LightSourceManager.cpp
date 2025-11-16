#include "render/LightSourceManager.h"

// Custom.
#include "render/shader/LightSourceShaderArray.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/light/PointLightNode.h"

LightSourceManager::~LightSourceManager() {}

LightSourceManager::LightSourceManager() {
    constexpr unsigned int MAX_POINT_LIGHT_COUNT = 16;      // <- same as in shaders
    constexpr unsigned int MAX_SPOT_LIGHT_COUNT = 16;       // <- same as in shaders
    constexpr unsigned int MAX_DIRECTIONAL_LIGHT_COUNT = 2; // <- same as in shaders

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

    // Create array of spotlights.
    pPointLightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(PointLightNode::ShaderProperties),
        MAX_POINT_LIGHT_COUNT,
        "PointLights",
        "iPointLightCount"));
}

void LightSourceManager::setAmbientLightColor(const glm::vec3& color) { ambientLightColor = color; }
