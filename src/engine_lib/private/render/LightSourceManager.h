#pragma once

// Standard.
#include <memory>

class Renderer;
class LightSourceShaderArray;

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

    /** Renderer. */
    Renderer* const pRenderer = nullptr;
};
