#pragma once

// Standard.
#include <optional>

// Custom.
#include "game/geometry/ScreenQuadGeometry.h"
#include "render/wrapper/ShaderProgram.h"
#include "math/GLMath.hpp"

class ShaderManager;
class Framebuffer;
class CameraProperties;

/** Tint color for rendered image based on distance from camera. */
class DistanceFogSettings {
public:
    /**
     * Sets start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera in
     * world units.
     *
     * @param range Start and end positions.
     */
    void setFogRange(const glm::vec2& range);

    /**
     * Sets color of the fog.
     *
     * @param color RGB color.
     */
    void setColor(const glm::vec3& color);

    /**
     * Returns start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera in
     * world units.
     *
     * @return Fog start and end positions.
     */
    glm::vec2 getFogRange() const { return fogRange; }

    /**
     * Returns color of the fog.
     *
     * @return RGB color.
     */
    glm::vec3 getColor() const { return color; }

private:
    /** Color of the fog. */
    glm::vec3 color = glm::vec3(0.0F, 0.0F, 0.0F);

    /**
     * Start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera in
     * world units.
     */
    glm::vec2 fogRange = glm::vec2(0.0F, 50.0F);
};

/** Settings for post processing of the rendered image. */
class PostProcessManager {
    // Only renderer is supposed to create such objects.
    friend class Renderer;

public:
    PostProcessManager() = delete;

    /**
     * Sets color of the light that will be added to every rendered object.
     *
     * @param color RGB color.
     */
    void setAmbientLightColor(const glm::vec3& color);

    /**
     * Sets distance fog settings.
     *
     * @param settings Specify empty to disable distance fog.
     */
    void setDistanceFogSettings(const std::optional<DistanceFogSettings>& settings);

    /**
     * Returns color of the light that will be added to every rendered object.
     *
     * @return RGB color.
     */
    glm::vec3 getAmbientLightColor() const { return ambientLightColor; }

    /**
     * Returns settings for distance fog.
     *
     * @return Empty if disabled.
     */
    const std::optional<DistanceFogSettings>& getDistanceFogSettings() const { return distanceFogSettings; }

private:
    /**
     * Creates a new object.
     *
     * @param pShaderManager Shader manager.
     * @param iWidth         Width of the post-processing framebuffer.
     * @param iHeight        Height of the post-processing framebuffer.
     */
    PostProcessManager(ShaderManager* pShaderManager, unsigned int iWidth, unsigned int iHeight);

    /**
     * Draws post-processing fullscreen quad on @ref pFramebuffer.
     *
     * @param fullscreenQuadGeometry Fullscreen quad to draw.
     * @param readFramebuffer        Framebuffer to read from.
     * @param pCameraProperties      Properties of the main camera.
     */
    void drawPostProcessing(
        const ScreenQuadGeometry& fullscreenQuadGeometry,
        const Framebuffer& readFramebuffer,
        CameraProperties* pCameraProperties);

    /** Shader program used for rendering fullscreen quad. */
    std::shared_ptr<ShaderProgram> pShaderProgram;

    /** Framebuffer to draw results of the post-processing */
    std::unique_ptr<Framebuffer> pFramebuffer;

    /** Empty if disabled. */
    std::optional<DistanceFogSettings> distanceFogSettings;

    /** Constant light that will be added. */
    glm::vec3 ambientLightColor = glm::vec3(0.1F, 0.1F, 0.1F); // NOLINT: have a bit of ambient by default
};
