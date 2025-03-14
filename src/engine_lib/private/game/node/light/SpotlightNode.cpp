#include "game/node/light/SpotlightNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/LightSourceManager.h"

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
