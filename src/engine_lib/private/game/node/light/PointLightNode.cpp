#include "game/node/light/PointLightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"

PointLightNode::PointLightNode() : PointLightNode("Point Light Node") {}

PointLightNode::PointLightNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void PointLightNode::onSpawning() {
    SpatialNode::onSpawning();

    std::scoped_lock guard(mtxProperties.first);

    // Set up to date parameters.
    mtxProperties.second.shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);

    if (mtxProperties.second.bIsVisible) {
        // Add to rendering.
        auto& lightSourceManager = getGameInstanceWhileSpawned()->getRenderer()->getLightSourceManager();
        mtxProperties.second.pActiveLightHandle =
            lightSourceManager.getPointLightsArray().addLightSourceToRendering(
                this, &mtxProperties.second.shaderProperties);
    }
}

void PointLightNode::onDespawning() {
    SpatialNode::onDespawning();

    std::scoped_lock guard(mtxProperties.first);

    if (mtxProperties.second.bIsVisible) {
        // Remove from rendering.
        mtxProperties.second.pActiveLightHandle = nullptr;
    }
}

void PointLightNode::setIsVisible(bool bIsVisible) {
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
                lightSourceManager.getPointLightsArray().addLightSourceToRendering(
                    this, &mtxProperties.second.shaderProperties);
        } else {
            // Remove from rendering.
            mtxProperties.second.pActiveLightHandle = nullptr;
        }
    }
}

void PointLightNode::setLightColor(const glm::vec3& color) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity =
        glm::vec4(color, mtxProperties.second.shaderProperties.colorAndIntensity.w);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void PointLightNode::setLightIntensity(float intensity) {
    std::scoped_lock guard(mtxProperties.first);

    mtxProperties.second.shaderProperties.colorAndIntensity.w = std::clamp(intensity, 0.0F, 1.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

void PointLightNode::setLightDistance(float distance) {
    std::scoped_lock guard(mtxProperties.first);

    // Save new parameter.
    mtxProperties.second.shaderProperties.distance = glm::max(distance, 0.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}

bool PointLightNode::isVisible() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.bIsVisible;
}

glm::vec3 PointLightNode::getLightColor() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity;
}

float PointLightNode::getLightIntensity() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.colorAndIntensity.w;
}

float PointLightNode::getLightDistance() {
    std::scoped_lock guard(mtxProperties.first);
    return mtxProperties.second.shaderProperties.distance;
}

void PointLightNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    std::scoped_lock guard(mtxProperties.first);

    // Update direction for shaders.
    mtxProperties.second.shaderProperties.position = glm::vec4(getWorldLocation(), 1.0F);

    // Update shader data.
    if (mtxProperties.second.pActiveLightHandle != nullptr) {
        mtxProperties.second.pActiveLightHandle->copyNewProperties(&mtxProperties.second.shaderProperties);
    }
}
