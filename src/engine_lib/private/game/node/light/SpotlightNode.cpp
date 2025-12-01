#include "game/node/light/SpotlightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"
#include "render/shader/ShaderArrayIndexManager.h"
#include "render/wrapper/Framebuffer.h"
#include "render/GpuResourceManager.h"
#include "game/World.h"

// External.
#include "glad/glad.h"
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "003ba11d-bc89-4e1b-becf-b35f9e9c5d12";

    constexpr float minLightDistance = 0.15f;
    constexpr float shadowNearClipPlane = 0.1f;
    static_assert(minLightDistance > shadowNearClipPlane);
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

    variables.bools[NAMEOF_MEMBER(&SpotlightNode::bCastShadows).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<SpotlightNode*>(pThis)->setCastShadows(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<SpotlightNode*>(pThis)->isCastingShadows();
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

    shaderProperties.position = glm::vec4(getWorldLocation(), 1.0f);
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0f);
    shaderProperties.cosInnerConeAngle = glm::cos(glm::radians(innerConeAngle));
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));

    if (bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();
        pActiveLightHandle =
            lightSourceManager.getSpotlightsArray().addLightSourceToRendering(this, &shaderProperties);

        if (bCastShadows) {
            createShadowMapData();
        }
    }

    recalculateConeShape();
}

void SpotlightNode::onDespawning() {
    SpatialNode::onDespawning();

    // Remove from rendering.
    pActiveLightHandle = nullptr;
    pShadowMapData = nullptr;
}

void SpotlightNode::setCastShadows(bool bEnable) {
    if (bCastShadows == bEnable) {
        return;
    }

    bCastShadows = bEnable;

    if (isSpawned() && bIsVisible) {
        if (bCastShadows) {
            createShadowMapData();
        } else {
            pShadowMapData = nullptr;

            // Update shader data.
            shaderProperties.iShadowMapIndex = -1;
            if (pActiveLightHandle != nullptr) {
                pActiveLightHandle->copyNewProperties(&shaderProperties);
            }
        }
    }
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

            if (bCastShadows) {
                createShadowMapData();
            }
        } else {
            // Remove from rendering.
            pActiveLightHandle = nullptr;
            pShadowMapData = nullptr;
        }
    }
}

void SpotlightNode::createShadowMapData() {
    if (pShadowMapData != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" expected shadow map data to be not used yet", getNodeName()));
    }

    auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();

    pShadowMapData = std::make_unique<ShadowMapData>();
    pShadowMapData->pIndex = lightSourceManager.getSpotShadowArrayIndexManager().reserveIndex();
    pShadowMapData->pFramebuffer = GpuResourceManager::createShadowMapFramebuffer(
        lightSourceManager.getSpotlightShadowMapArray(), pShadowMapData->pIndex->getActualIndex());

    const auto worldLocation = getWorldLocation();

    shaderProperties.iShadowMapIndex = static_cast<int>(pShadowMapData->pIndex->getActualIndex());
    pShadowMapData->viewMatrix =
        glm::lookAt(worldLocation, worldLocation + getWorldForwardDirection(), getWorldUpDirection());
    recalculateShadowProjMatrix();

    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
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
    shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0f, 1.0f);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void SpotlightNode::setLightDistance(float distance) {
    shaderProperties.distance = glm::max(distance, 0.0f);
    if (pShadowMapData != nullptr) {
        recalculateShadowProjMatrix();
    }

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    recalculateConeShape();
}

void SpotlightNode::setLightInnerConeAngle(float inInnerConeAngle) {
    // Save new parameter.
    innerConeAngle = std::clamp(inInnerConeAngle, 0.0f, maxConeAngle);

    // Make sure outer cone is equal or bigger than inner cone.
    outerConeAngle = std::clamp(outerConeAngle, innerConeAngle, maxConeAngle);

    // Calculate cosine for shaders.
    shaderProperties.cosInnerConeAngle = glm::cos(glm::radians(innerConeAngle));
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));
    if (pShadowMapData != nullptr) {
        recalculateShadowProjMatrix();
    }

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    recalculateConeShape();
}

void SpotlightNode::setLightOuterConeAngle(float inOuterConeAngle) {
    outerConeAngle = std::clamp(inOuterConeAngle, innerConeAngle, maxConeAngle);

    // Calculate cosine for shaders.
    shaderProperties.cosOuterConeAngle = glm::cos(glm::radians(outerConeAngle));
    if (pShadowMapData != nullptr) {
        recalculateShadowProjMatrix();
    }

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    recalculateConeShape();
}

void SpotlightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    const auto worldLocation = getWorldLocation();

    shaderProperties.position = glm::vec4(worldLocation, 1.0f);
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0f);

    if (pShadowMapData != nullptr) {
        pShadowMapData->viewMatrix =
            glm::lookAt(worldLocation, worldLocation + getWorldForwardDirection(), getWorldUpDirection());
        recalculateShadowProjMatrix();
    }

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }

    recalculateConeShape();
}

void SpotlightNode::recalculateConeShape() {
    const auto coneBottomRadius = glm::tan(glm::radians(outerConeAngle)) * shaderProperties.distance;

    coneWorld =
        Cone(getWorldLocation(), shaderProperties.distance, getWorldForwardDirection(), coneBottomRadius);
}

void SpotlightNode::recalculateShadowProjMatrix() {
    const auto farClipPlane = shaderProperties.distance;

    static_assert(maxConeAngle <= 90.0f, "change FOV for shadow map capture");
    const auto fovYRadians =
        glm::radians(outerConeAngle * 2.0f); // x2 to convert [0..90] degree to [0..180] FOV

    const float aspectRatio = 1.0f;

    shaderProperties.viewProjectionMatrix =
        glm::perspective(fovYRadians, aspectRatio, shadowNearClipPlane, farClipPlane) *
        pShadowMapData->viewMatrix;

    const auto worldLocation = getWorldLocation();
    const auto worldDirection = getWorldForwardDirection();

    pShadowMapData->frustumWorld = Frustum::create(
        worldLocation,
        worldDirection,
        getWorldUpDirection(),
        shadowNearClipPlane,
        farClipPlane,
        fovYRadians,
        aspectRatio);
}

SpotlightNode::ShaderProperties::ShaderProperties() = default;
