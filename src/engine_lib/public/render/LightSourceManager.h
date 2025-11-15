#pragma once

// Standard.
#include <memory>

// Custom.
#include "math/GLMath.hpp"

class LightSourceShaderArray;
class Node;

/** Manages active (spawned and visible) light nodes that will be rendered. */
class LightSourceManager {
    // Only world is expected to create objects of this type.
    friend class World;

public:
    ~LightSourceManager();

    LightSourceManager(const LightSourceManager&) = delete;
    LightSourceManager& operator=(const LightSourceManager&) = delete;
    LightSourceManager(LightSourceManager&&) noexcept = delete;
    LightSourceManager& operator=(LightSourceManager&&) noexcept = delete;

    /**
     * Sets color of the light that will be added to every rendered object.
     *
     * @param color RGB color.
     */
    void setAmbientLightColor(const glm::vec3& color);

    /**
     * Returns ambient light color used for rendering.
     *
     * @return Ambient light color.
     */
    glm::vec3 getAmbientLightColor() const { return ambientLightColor; }

    /**
     * Returns point light array.
     *
     * @return Array.
     */
    LightSourceShaderArray& getPointLightsArray() { return *pPointLightsArray; }

    /**
     * Returns point light array.
     *
     * @return Array.
     */
    LightSourceShaderArray& getSpotlightsArray() { return *pSpotlightsArray; }

    /**
     * Returns point light array.
     *
     * @return Array.
     */
    LightSourceShaderArray& getDirectionalLightsArray() { return *pDirectionalLightsArray; }

private:
    LightSourceManager();

    /** Properties of all active directional lights. */
    std::unique_ptr<LightSourceShaderArray> pDirectionalLightsArray;

    /** Properties of all active spotlights. */
    std::unique_ptr<LightSourceShaderArray> pSpotlightsArray;

    /** Properties of all active point lights. */
    std::unique_ptr<LightSourceShaderArray> pPointLightsArray;

    /** Constant light that will be added. */
    glm::vec3 ambientLightColor = glm::vec3(0.1F, 0.1F, 0.1F);
};
