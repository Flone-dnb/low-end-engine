#include "game/node/light/SpotlightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"
#include "game/World.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "003ba11d-bc89-4e1b-becf-b35f9e9c5d12";
}

std::string SpotlightNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SpotlightNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SpotlightNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s["color"] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setLightColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<SpotlightNode*>(pThis)->getLightColor();
        }};

    variables.floats["intensity"] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setLightIntensity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SpotlightNode*>(pThis)->getLightIntensity();
        }};

    variables.floats["distance"] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setLightDistance(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SpotlightNode*>(pThis)->getLightDistance();
        }};

    variables.floats[NAMEOF_MEMBER(&SpotlightNode::innerConeAngle).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setLightInnerConeAngle(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SpotlightNode*>(pThis)->getLightInnerConeAngle();
        }};

    variables.floats[NAMEOF_MEMBER(&SpotlightNode::outerConeAngle).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setLightOuterConeAngle(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SpotlightNode*>(pThis)->getLightOuterConeAngle();
        }};

    variables.bools[NAMEOF_MEMBER(&SpotlightNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setIsVisible(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<SpotlightNode*>(pThis)->isVisible();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SpotlightNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SpotlightNode>(); },
        std::move(variables));
}

SpotlightNode::SpotlightNode() : SpotlightNode("Spotlight Node") {}

SpotlightNode::SpotlightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void SpotlightNode::onSpawning() {
    SpatialNode::onSpawning();

    shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);
    shaderProperties.cosInnerConeAngle = glm::cos(glm::radians(innerConeAngle));
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));

    if (bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();
        pActiveLightHandle =
            lightSourceManager.getSpotlightsArray().addLightSourceToRendering(this, &shaderProperties);
    }
}

void SpotlightNode::onDespawning() {
    SpatialNode::onDespawning();

    // Remove from rendering.
    pActiveLightHandle = nullptr;
}

void SpotlightNode::setIsVisible(bool bNewVisible) {
    if (bIsVisible == bNewVisible) {
        return;
    }
    bIsVisible = bNewVisible;

    if (isSpawned()) {
        if (bIsVisible) {
            // Add to rendering.
            auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();
            pActiveLightHandle =
                lightSourceManager.getSpotlightsArray().addLightSourceToRendering(this, &shaderProperties);
        } else {
            // Remove from rendering.
            pActiveLightHandle = nullptr;
        }
    }
}

void SpotlightNode::setLightColor(const glm::vec3& color) {
    shaderProperties.colorAndIntensity = glm::vec4(color, shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::setLightIntensity(float intensity) {
    shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0F, 1.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::setLightDistance(float distance) {
    shaderProperties.distance = glm::max(distance, 0.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::setLightInnerConeAngle(float inInnerConeAngle) {
    // Save new parameter.
    innerConeAngle = std::clamp(inInnerConeAngle, 0.0F, maxConeAngle);

    // Make sure outer cone is equal or bigger than inner cone.
    outerConeAngle = std::clamp(outerConeAngle, innerConeAngle, maxConeAngle);

    // Calculate cosine for shaders.
    shaderProperties.cosInnerConeAngle = glm::cos(glm::radians(innerConeAngle));
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::setLightOuterConeAngle(float inOuterConeAngle) {
    outerConeAngle = std::clamp(inOuterConeAngle, innerConeAngle, maxConeAngle);

    // Calculate cosine for shaders.
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

SpotlightNode::ShaderProperties::ShaderProperties() = default;
