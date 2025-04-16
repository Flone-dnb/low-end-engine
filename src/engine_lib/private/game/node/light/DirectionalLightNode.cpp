#include "game/node/light/DirectionalLightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "bd598071-6b07-41b4-87ae-67fa13f4670c";
}

std::string DirectionalLightNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string DirectionalLightNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo DirectionalLightNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s["color"] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<DirectionalLightNode*>(pThis)->setLightColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<DirectionalLightNode*>(pThis)->getLightColor();
        }};

    variables.floats["intensity"] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<DirectionalLightNode*>(pThis)->setLightIntensity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<DirectionalLightNode*>(pThis)->getLightIntensity();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(DirectionalLightNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<DirectionalLightNode>(); },
        std::move(variables));
}

DirectionalLightNode::DirectionalLightNode() : DirectionalLightNode("Directional Light Node") {}

DirectionalLightNode::DirectionalLightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void DirectionalLightNode::onSpawning() {
    SpatialNode::onSpawning();

    std::scoped_lock guard(mtxProperties.first);

    // Set up to date parameters.
    mtxProperties.second.shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);

    if (mtxProperties.second.bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getGameInstanceWhileSpawned()->getRenderer()->getLightSourceManager();
        mtxProperties.second.pActiveLightHandle =
            lightSourceManager.getDirectionalLightsArray().addLightSourceToRendering(
                this, &mtxProperties.second.shaderProperties);
    }
}

void DirectionalLightNode::onDespawning() {
    SpatialNode::onDespawning();

    std::scoped_lock guard(mtxProperties.first);

    if (mtxProperties.second.bIsVisible) {
        // Remove from rendering.
        mtxProperties.second.pActiveLightHandle = nullptr;
    }
}

void DirectionalLightNode::setIsVisible(bool bIsVisible) {
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
                lightSourceManager.getDirectionalLightsArray().addLightSourceToRendering(
                    this, &mtxProperties.second.shaderProperties);
        } else {
            // Remove from rendering.
            mtxProperties.second.pActiveLightHandle = nullptr;
        }
    }
}

glm::vec3 DirectionalLightNode::getLightColor() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity;
}

float DirectionalLightNode::getLightIntensity() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity.w;
}

bool DirectionalLightNode::isVisible() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.bIsVisible;
}

void DirectionalLightNode::setLightIntensity(float intensity) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0F, 1.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void DirectionalLightNode::setLightColor(const glm::vec3& color) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity =
        glm::vec4(color, mtxProperties.second.shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void DirectionalLightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    std::scoped_lock guard(mtxProperties.first);

    // Update direction for shaders.
    mtxProperties.second.shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

// `default` constructor in .cpp file - this is a fix for a gcc bug:
// error: default member initializer for required before the end of its enclosing class
DirectionalLightNode::ShaderProperties::ShaderProperties() = default;
DirectionalLightNode::Properties::Properties() = default;
