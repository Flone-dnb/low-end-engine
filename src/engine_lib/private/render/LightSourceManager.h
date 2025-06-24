#pragma once

// Standard.
#include <memory>

class PostProcessManager;
class LightSourceShaderArray;
class ShaderProgram;

/** Manages active (spawned and visible) light nodes that will be rendered. */
class LightSourceManager {
    // Only world is expected to create objects of this type.
    friend class World;

public:
    LightSourceManager() = delete;

    ~LightSourceManager();

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

private:
    /**
     * Constructs a new manager.
     *
     * @param pPostProcessManager Post-process manager.
     */
    LightSourceManager(PostProcessManager* pPostProcessManager);

    /** Properties of all active directional lights. */
    std::unique_ptr<LightSourceShaderArray> pDirectionalLightsArray;

    /** Properties of all active spotlights. */
    std::unique_ptr<LightSourceShaderArray> pSpotlightsArray;

    /** Properties of all active point lights. */
    std::unique_ptr<LightSourceShaderArray> pPointLightsArray;

    /** Post-process manager. */
    PostProcessManager* const pPostProcessManager = nullptr;
};
