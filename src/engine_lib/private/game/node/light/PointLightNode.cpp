#include "game/node/light/PointLightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"
#include "game/World.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "02d0f522-1e32-4a2d-bacd-9efef2d9ae07";
}

std::string PointLightNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string PointLightNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo PointLightNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s["color"] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<PointLightNode*>(pThis)->setLightColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<PointLightNode*>(pThis)->getLightColor();
        }};

    variables.floats["intensity"] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<PointLightNode*>(pThis)->setLightIntensity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<PointLightNode*>(pThis)->getLightIntensity();
        }};

    variables.floats[NAMEOF_MEMBER(&PointLightNode::ShaderProperties::distance).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<PointLightNode*>(pThis)->setLightDistance(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<PointLightNode*>(pThis)->getLightDistance();
            }};

    variables.bools[NAMEOF_MEMBER(&PointLightNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<PointLightNode*>(pThis)->setIsVisible(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<PointLightNode*>(pThis)->isVisible();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(PointLightNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<PointLightNode>(); },
        std::move(variables));
}

PointLightNode::PointLightNode() : PointLightNode("Point Light Node") {}

PointLightNode::PointLightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void PointLightNode::onSpawning() {
    SpatialNode::onSpawning();

    shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);

    if (bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();
        pActiveLightHandle =
            lightSourceManager.getPointLightsArray().addLightSourceToRendering(this, &shaderProperties);
    }

    sphereShapeWorld = Sphere(getWorldLocation(), shaderProperties.distance);
}

void PointLightNode::onDespawning() {
    SpatialNode::onDespawning();

    // Remove from rendering.
    pActiveLightHandle = nullptr;
}

void PointLightNode::setIsVisible(bool bNewVisible) {
    if (bIsVisible == bNewVisible) {
        return;
    }
    bIsVisible = bNewVisible;

    if (isSpawned()) {
        if (bIsVisible) {
            // Add to rendering.
            auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();
            pActiveLightHandle =
                lightSourceManager.getPointLightsArray().addLightSourceToRendering(this, &shaderProperties);
        } else {
            // Remove from rendering.
            pActiveLightHandle = nullptr;
        }
    }
}

void PointLightNode::setLightColor(const glm::vec3& color) {
    shaderProperties.colorAndIntensity = glm::vec4(color, shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void PointLightNode::setLightIntensity(float intensity) {
    shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0F, 1.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void PointLightNode::setLightDistance(float distance) {
    shaderProperties.distance = glm::max(distance, 0.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    sphereShapeWorld = Sphere(getWorldLocation(), shaderProperties.distance);
}

void PointLightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    sphereShapeWorld = Sphere(getWorldLocation(), shaderProperties.distance);
}

PointLightNode::ShaderProperties::ShaderProperties() = default;
