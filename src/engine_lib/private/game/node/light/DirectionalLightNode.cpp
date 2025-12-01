#include "game/node/light/DirectionalLightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "game/World.h"
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

    variables.bools[NAMEOF_MEMBER(&DirectionalLightNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<DirectionalLightNode*>(pThis)->setIsVisible(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<DirectionalLightNode*>(pThis)->isVisible();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(DirectionalLightNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<DirectionalLightNode>(); },
        std::move(variables));
}

DirectionalLightNode::DirectionalLightNode() : DirectionalLightNode("Directional Light Node") {}
DirectionalLightNode::DirectionalLightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void DirectionalLightNode::addToRendering() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

    if (!bIsVisible) {
        return;
    }

    auto& lightSourceManager = getWorldWhileSpawned()->getLightSourceManager();

    pActiveLightHandle =
        lightSourceManager.getDirectionalLightsArray().addLightSourceToRendering(this, &shaderProperties);
}

void DirectionalLightNode::removeFromRendering() { pActiveLightHandle = nullptr; }

void DirectionalLightNode::onSpawning() {
    SpatialNode::onSpawning();

    // Set up to date parameters.
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0f);

    addToRendering();
}

void DirectionalLightNode::onDespawning() {
    SpatialNode::onDespawning();

    removeFromRendering();
}

void DirectionalLightNode::setIsVisible(bool bNewVisible) {
    if (bIsVisible == bNewVisible) {
        return;
    }
    bIsVisible = bNewVisible;

    if (isSpawned()) {
        if (bIsVisible) {
            addToRendering();
        } else {
            removeFromRendering();
        }
    }
}

void DirectionalLightNode::setLightIntensity(float intensity) {
    shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0f, 1.0f);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void DirectionalLightNode::setLightColor(const glm::vec3& color) {
    shaderProperties.colorAndIntensity = glm::vec4(color, shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

void DirectionalLightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    // Update direction for shaders.
    shaderProperties.direction = glm::vec4(getWorldForwardDirection(), 0.0f);

    // Update shader data.
    if (pActiveLightHandle != nullptr) {
        pActiveLightHandle->copyNewProperties(&shaderProperties);
    }
}

DirectionalLightNode::ShaderProperties::ShaderProperties() = default;
