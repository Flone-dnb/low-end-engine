#include "render/LightSourceManager.h"

// Custom.
#include "render/shader/LightSourceShaderArray.h"
#include "render/shader/ShaderManager.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/SpotlightNode.h"

LightSourceManager::LightSourceManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    // Create array of directional lights.
    pDirectionalLightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(DirectionalLightNode::ShaderProperties),
        static_cast<unsigned int>(
            ShaderManager::getEnginePredefinedMacroValue(EnginePredefinedMacro::MAX_DIRECTIONAL_LIGHT_COUNT)),
        "DirectionalLights",
        "iDirectionalLightCount"));

    // Create array of spotlights.
    pSpotlightsArray = std::unique_ptr<LightSourceShaderArray>(new LightSourceShaderArray(
        this,
        sizeof(SpotlightNode::ShaderProperties),
        static_cast<unsigned int>(
            ShaderManager::getEnginePredefinedMacroValue(EnginePredefinedMacro::MAX_SPOT_LIGHT_COUNT)),
        "Spotlights",
        "iSpotlightCount"));
}

LightSourceShaderArray& LightSourceManager::getDirectionalLightsArray() {
    return *pDirectionalLightsArray.get();
}

LightSourceShaderArray& LightSourceManager::getSpotlightsArray() { return *pSpotlightsArray.get(); }

Renderer* LightSourceManager::getRenderer() const { return pRenderer; }
