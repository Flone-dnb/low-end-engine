#pragma once

// Standard.
#include <memory>
#include <variant>
#include <optional>
#include <array>

// Custom.
#include "misc/Error.h"
#include "render/RenderStatistics.h"
#include "math/GLMath.hpp"

// External.
#include "SDL3/SDL_video.h"

typedef struct __GLsync* GLsync;

class Window;
class FontManager;
class TextureManager;
class CameraProperties;
class ScreenQuadGeometry;
class Framebuffer;
class ShaderManager;
class TextureHandle;
class ShaderProgram;
class VertexArrayObject;

/** Settings for skybox. */
struct SkyboxSettings {
    SkyboxSettings();
    ~SkyboxSettings();

    SkyboxSettings(SkyboxSettings&&) noexcept = default;
    SkyboxSettings& operator=(SkyboxSettings&&) noexcept = default;

    /** Path (relative to the `res` directory) to the fragment shader. */
    std::string sRelativePathToFragmentShader = "engine/shaders/skybox/skybox.frag.glsl";

    /**
     * If `nullptr` (not specified) then a procedural skybox will be used (from the shader code),
     * otherwise a cubemap to use as a skybox.
     */
    std::unique_ptr<TextureHandle> pOptSkyboxCubemap;
};

/** Tint color for rendered image based on distance from camera. */
class DistanceFogSettings {
public:
    /**
     * Sets start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera.
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
     * When skybox is enabled defines how much of height (including the sky) the fog covers.
     * Value in range [0.0; 1.0].
     *
     * @param fogHeight Fog height.
     */
    void setFogHeightOnSky(float fogHeight);

    /**
     * Returns start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera.
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

    /**
     * When skybox is enabled defines how much of height (including the sky) the fog covers.
     * Value in range [0.0; 1.0].
     *
     * @return Fog height.
     */
    float getFogHeightOnSky() const { return fogHeightOnSky; }

private:
    /** Color of the fog. */
    glm::vec3 color = glm::vec3(1.0F, 1.0F, 1.0F);

    /** Start (min fog) and end (max fog) positions in range [0.0; +inf] as distance from camera. */
    glm::vec2 fogRange = glm::vec2(0.0F, 50.0F);

    /**
     * When skybox is enabled defines how much of height (including the sky) the fog covers.
     * Value in range [0.0; 1.0].
     */
    float fogHeightOnSky = 0.25F;
};

/** OpenGL ES renderer. */
class Renderer {
    // Only game manager can create a renderer.
    friend class GameManager;

    // Window calls `drawNextFrame`.
    friend class Window;

public:
    ~Renderer();

    /**
     * Sets the maximum number of FPS that is allowed to be produced in a second.
     *
     * @param iNewFpsLimit Maximum allowed FPS, specify 0 to disable.
     */
    void setFpsLimit(unsigned int iNewFpsLimit);

    /**
     * Enables or disables the skybox.
     *
     * @param newSkyboxSettings Specify empty to disable, otherwise skybox settings.
     */
    void setSkybox(std::optional<SkyboxSettings> newSkyboxSettings);

    /**
     * Sets distance fog settings.
     *
     * @param settings Specify empty to disable distance fog.
     */
    void setDistanceFogSettings(const std::optional<DistanceFogSettings>& settings) {
        optDistanceFogSettings = settings;
    }

    /**
     * Returns the maximum number of FPS that is allowed to be produced in a second.
     *
     * @return 0 if disabled.
     */
    unsigned int getFpsLimit() const { return renderStats.fpsLimitInfo.iFpsLimit; }

    /**
     * Returns the currently used GL depth func.
     *
     * @return Depth func.
     */
    unsigned int getCurrentGlDepthFunc() const { return iCurrentGlDepthFunc; }

    /**
     * Returns settings for distance fog.
     *
     * @return Empty if disabled.
     */
    const std::optional<DistanceFogSettings>& getDistanceFogSettings() const {
        return optDistanceFogSettings;
    }

    /**
     * Returns game's window.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the game's window.
     */
    Window* getWindow() const { return pWindow; }

    /**
     * Returns manager used to load and compile shaders.
     *
     * @remark As a game developer you don't need to use this. Materials use this function automatically.
     *
     * @return Manager.
     */
    ShaderManager& getShaderManager();

    /**
     * Returns manager used to load fonts.
     *
     * @return Manager.
     */
    FontManager& getFontManager();

    /**
     * Returns manager used to load textures.
     *
     * @return Manager.
     */
    TextureManager& getTextureManager();

    /**
     * Returns various statistics about the rendering.
     *
     * @return Stats.
     */
    RenderStatistics& getRenderStatistics();

private:
    /** Groups stuff used to synchronize GPU and CPU. */
    struct FrameSyncData {
        /** 2 frames in-flight seems optimal, more can affect input latency. */
        static constexpr size_t iFramesInFlight = 2;

        /** GL GPU time queries. */
        struct FrameQueries {
            /** GL query ID for measuring GPU time that we spent drawing meshes. */
            unsigned int iGlQueryToDrawMeshes = 0;

            /** GL query ID for measuring GPU time that we spent drawing skybox. */
            unsigned int iGlQueryToDrawSkybox = 0;

            /** GL query ID for measuring GPU time that we spent drawing UI. */
            unsigned int iGlQueryToDrawUi = 0;

            /** GL query ID for measuring GPU time that we spent drawing debug objects. */
            unsigned int iGlQueryToDrawDebug = 0;
        };
        /** GL queries. */
        std::array<FrameQueries, iFramesInFlight> vFrameQueries;

        /** Fence per frame in-flight. */
        std::array<GLsync, iFramesInFlight> vFences;

        /** Current index into @ref vFences. */
        unsigned int iCurrentFrameIndex = 0;
    };

    /** Groups data related to skybox. */
    struct SkyboxData {
        SkyboxData();
        ~SkyboxData();

        /** Not empty if skybox is used. */
        std::optional<SkyboxSettings> optSettings;

        /** Shader program for rendering skybox. */
        std::shared_ptr<ShaderProgram> pShaderProgram;

        /** VAO for skybox rendering. */
        std::unique_ptr<VertexArrayObject> pCubeVao;

        /** Location of the uniform variable. */
        int iViewProjectionMatrixUniform = 0;

        /** Location of the uniform variable. */
        int iIsSkyboxCubemapSetUniform = -1;

        /** Location of the uniform variable. */
        int iFogHeightOnSkyUniform = 0;

        /** Location of the uniform variable. */
        int iFogColorUniform = 0;
    };

    /**
     * Creates a new renderer.
     *
     * @param pWindow Valid pointer to the window.
     *
     * @return Error if something went wrong, otherwise created renderer.
     */
    static std::variant<std::unique_ptr<Renderer>, Error> create(Window* pWindow);

    /**
     * Initializes the renderer.
     *
     * @param pWindow         Valid pointer to the window.
     * @param pCreatedContext Created OpenGL context.
     */
    Renderer(Window* pWindow, SDL_GLContext pCreatedContext);

    /** Called by window after its size changed. */
    void onWindowSizeChanged();

    /** (Re)creates framebuffers used by the renderer. */
    void recreateFramebuffers();

    /**
     * Called by window that owns this renderer to draw a new frame.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    void drawNextFrame(float timeSincePrevCallInSec);

    /**
     * Calculates some frame-related statistics.
     *
     * @remark Must be called after a frame was submitted.
     */
    void calculateFrameStatistics();

    /** Database of all shaders. */
    std::unique_ptr<ShaderManager> pShaderManager;

    /** Texture loading and management. */
    std::unique_ptr<TextureManager> pTextureManager;

    /** .ttf loading and rendering. */
    std::unique_ptr<FontManager> pFontManager;

    /** Fullscreen quad for rendering. */
    std::unique_ptr<ScreenQuadGeometry> pFullscreenQuad;

    /** Empty if disabled. */
    std::optional<DistanceFogSettings> optDistanceFogSettings;

    /** Skybox-related data. */
    SkyboxData skyboxData;

    /** Various statistics about rendering. */
    RenderStatistics renderStats;

    /** GPU-CPU synchronization. */
    FrameSyncData frameSyncData;

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Always valid pointer. */
    Window* const pWindow = nullptr;

    /** Current GL depth func. */
    unsigned int iCurrentGlDepthFunc = 0;
};
