#include "game/node/light/SpotlightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"

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

    variables.floats[NAMEOF_MEMBER(&SpotlightNode::Properties::innerConeAngle).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SpotlightNode*>(pThis)->setLightInnerConeAngle(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SpotlightNode*>(pThis)->getLightInnerConeAngle();
            }};

    variables.floats[NAMEOF_MEMBER(&SpotlightNode::Properties::outerConeAngle).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SpotlightNode*>(pThis)->setLightOuterConeAngle(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SpotlightNode*>(pThis)->getLightOuterConeAngle();
            }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(SpotlightNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SpotlightNode>(); },
        std::move(variables));
}

SpotlightNode::SpotlightNode() : SpotlightNode("Spotlight Node") {}

SpotlightNode::SpotlightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void SpotlightNode::onSpawning() {
    SpatialNode::onSpawning();

    std::scoped_lock guard(mtxProperties.first);

    // Set up to date parameters.
    mtxProperties.second.shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);
    mtxProperties.second.shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);
    mtxProperties.second.shaderProperties.cosInnerConeAngle =
        glm::cos(glm::radians(mtxProperties.second.innerConeAngle));
    mtxProperties.second.shaderProperties.cosOuterConeAngle =
        glm::cos(glm::radians(mtxProperties.second.outerConeAngle));

    if (mtxProperties.second.bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getGameInstanceWhileSpawned()->getRenderer()->getLightSourceManager();
        mtxProperties.second.pActiveLightHandle =
            lightSourceManager.getSpotlightsArray().addLightSourceToRendering(
                this, &mtxProperties.second.shaderProperties);
    }
}

void SpotlightNode::onDespawning() {
    SpatialNode::onDespawning();

    std::scoped_lock guard(mtxProperties.first);

    if (mtxProperties.second.bIsVisible) {
        // Remove from rendering.
        mtxProperties.second.pActiveLightHandle = nullptr;
    }
}

void SpotlightNode::setIsVisible(bool bIsVisible) {
    std::scoped_lock guard(mtxProperties.first);

    if (mtxProperties.second.bIsVisible == bIsVisible) {
        return;
    }
    mtxProperties.second.bIsVisible = bIsVisible;

    if (isSpawned()) {
        if (mtxProperties.second.bIsVisible) {
            // Add to rendering.
            auto& lightSourceManager = getGameInstanceWhileSpawned()->getRenderer()->getLightSourceManager();
            mtxProperties.second.pActiveLightHandle =
                lightSourceManager.getSpotlightsArray().addLightSourceToRendering(
                    this, &mtxProperties.second.shaderProperties);
        } else {
            // Remove from rendering.
            mtxProperties.second.pActiveLightHandle = nullptr;
        }
    }
}

void SpotlightNode::setLightColor(const glm::vec3& color) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity =
        glm::vec4(color, mtxProperties.second.shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void SpotlightNode::setLightIntensity(float intensity) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0F, 1.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void SpotlightNode::setLightDistance(float distance) {
    std::scoped_lock guard(mtxProperties.first);

    // Save new parameter.
    mtxProperties.second.shaderProperties.distance = glm::max(distance, 0.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void SpotlightNode::setLightInnerConeAngle(float innerConeAngle) {
    std::scoped_lock guard(mtxProperties.first);
    auto& props = mtxProperties.second;

    // Save new parameter.
    props.innerConeAngle = std::clamp(innerConeAngle, 0.0F, Properties::maxConeAngle);

    // Make sure outer cone is equal or bigger than inner cone.
    props.outerConeAngle = std::clamp(props.outerConeAngle, props.innerConeAngle, Properties::maxConeAngle);

    // Calculate cosine for shaders.
    props.shaderProperties.cosInnerConeAngle = glm::cos(glm::radians(props.innerConeAngle));
    props.shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(props.outerConeAngle));

    // Update shader data.
    if (props.pActiveLightHandle != nullptr) {
        props.pActiveLightHandle->copyNewProperties(&props.shaderProperties);
    }
}

void SpotlightNode::setLightOuterConeAngle(float outerConeAngle) {
    std::scoped_lock guard(mtxProperties.first);
    auto& props = mtxProperties.second;

    // Save new parameter.
    props.outerConeAngle = std::clamp(outerConeAngle, props.innerConeAngle, Properties::maxConeAngle);

    // Calculate cosine for shaders.
    props.shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(props.outerConeAngle));

    // Update shader data.
    if (props.pActiveLightHandle != nullptr) {
        props.pActiveLightHandle->copyNewProperties(&props.shaderProperties);
    }
}

glm::vec3 SpotlightNode::getLightColor() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity;
}

float SpotlightNode::getLightIntensity() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity.w;
}

float SpotlightNode::getLightDistance() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.distance;
}

void SpotlightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    std::scoped_lock guard(mtxProperties.first);

    // Update direction for shaders.
    mtxProperties.second.shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);
    mtxProperties.second.shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

float SpotlightNode::getLightInnerConeAngle() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.innerConeAngle;
}
float SpotlightNode::getLightOuterConeAngle() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.outerConeAngle;
}

bool SpotlightNode::isVisible() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.bIsVisible;
}

// `default` constructor in .cpp file - this is a fix for a gcc bug:
// error: default member initializer for required before the end of its enclosing class
SpotlightNode::ShaderProperties::ShaderProperties() = default;
SpotlightNode::Properties::Properties() = default;
