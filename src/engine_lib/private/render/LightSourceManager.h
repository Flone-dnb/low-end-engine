#pragma once

// Standard.
#include <memory>

class Renderer;
class LightSourceShaderArray;
class ShaderProgram;

/** Manages active (spawned and visible) light sources that will be rendered. */
class LightSourceManager {
    // Only renderer is expected to create objects of this type.
    friend class Renderer;

public:
    LightSourceManager() = delete;

    ~LightSourceManager() = default;

    LightSourceManager(const LightSourceManager&) = delete;
    LightSourceManager& operator=(const LightSourceManager&) = delete;
    LightSourceManager(LightSourceManager&&) noexcept = delete;
    LightSourceManager& operator=(LightSourceManager&&) noexcept = delete;

    /**
     * Returns array used by directional lights.
     *
     * @return Shader array.
     */
    LightSourceShaderArray& getDirectionalLightsArray();

    /**
     * Returns array used by spotlights.
     *
     * @return Shader array.
     */
    LightSourceShaderArray& getSpotlightsArray();

    /**
     * Returns array used by point lights.
     *
     * @return Shader array.
     */
    LightSourceShaderArray& getPointLightsArray();

    /**
     * Sets (binds) properties of all light arrays to the specified shader program.
     *
     * @param pShaderProgram Shader program.
     */
    void setArrayPropertiesToShader(ShaderProgram* pShaderProgram);

    /**
     * Returns renderer.
     *
     * @return Renderer.
     */
    Renderer* getRenderer() const;

private:
    /**
     * Constructs a new manager.
     *
     * @param pRenderer Renderer.
     */
    LightSourceManager(Renderer* pRenderer);

    /** Properties of all active directional lights. */
    std::unique_ptr<LightSourceShaderArray> pDirectionalLightsArray;

    /** Properties of all active spotlights. */
    std::unique_ptr<LightSourceShaderArray> pSpotlightsArray;

    /** Properties of all active point lights. */
    std::unique_ptr<LightSourceShaderArray> pPointLightsArray;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;
};
