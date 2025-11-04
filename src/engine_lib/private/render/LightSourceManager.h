#pragma once

// Standard.
#include <memory>

// Custom.
#include "math/GLMath.hpp"

class PostProcessManager;
class LightSourceShaderArray;
class ShaderProgram;
class ActiveLightSourceHandle;
class Node;

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

    /**
     * Called by spawned light sources that need to be rendered.
     *
     * @param pLightSource Light to add to rendering.
     * @param pProperties  Initial light properties data to copy to shaders.
     *
     * @return `nullptr` if the maximum number of visible lights was reached (try again later), otherwise
     * handle of the specified light node.
     */
    std::unique_ptr<ActiveLightSourceHandle>
    addLightSourceToRendering(Node* pLightSource, const void* pProperties);

    /**
     * Returns ambient light color used for rendering.
     *
     * @return Ambient light color.
     */
    glm::vec3 getAmbientLightColor() const;

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
