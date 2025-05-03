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
     * Sets distance to start applying the fog.
     *
     * @param distance Value in range [0.0; 1.0] where 0.0 is camera's near plane and 1.0 is far plane
     * position.
     */
    void setStartDistance(float distance);

    /**
     * Sets color of the fog.
     *
     * @param color RGB color.
     */
    void setColor(const glm::vec3& color);

    /**
     * Returns distance in view space to start distance fog.
     *
     * @return Distance in range [0.0; 1.0].
     */
    float getStartDistance() const { return startDistance; }

    /**
     * Returns color of the fog.
     *
     * @return RGB color.
     */
    glm::vec3 getColor() const { return color; }

private:
    /** Color of the fog. */
    glm::vec3 color = glm::vec3(0.0F, 0.0F, 0.0F);

    /** Distance in range [0.0; 1.0] where 0.0 is camera's near plane and 1.0 is far plane position. */
    float startDistance = 0.01F; // NOLINT
};

/** Settings for post processing of the rendered image. */
class PostProcessSettings {
    // Only renderer is supposed to create such objects.
    friend class Renderer;

public:
    PostProcessSettings() = delete;

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
    PostProcessSettings(ShaderManager* pShaderManager, unsigned int iWidth, unsigned int iHeight);

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
