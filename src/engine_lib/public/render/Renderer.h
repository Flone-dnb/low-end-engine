#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
#include "render/ShaderManager.h"
#include "misc/Error.h"
#include "render/RenderStatistics.h"
#include "render/PostProcessManager.h"

// External.
#include "SDL_video.h"

typedef struct __GLsync* GLsync;

class Window;
class FontManager;
class UiManager;
class LightSourceManager;
class TextureManager;
class Framebuffer;
class CameraProperties;

/** OpenGL ES renderer. */
class Renderer {
    // Only game manager can create a renderer.
    friend class GameManager;

    // Window calls `drawNextFrame`.
    friend class Window;

public:
    ~Renderer();

    /**
     * Blocks the current thread until the GPU finishes executing all queued graphics commands up to this
     * point.
     */
    static void waitForGpuToFinishWorkUpToThisPoint();

    /**
     * Sets the maximum number of FPS that is allowed to be produced in a second.
     *
     * @param iNewFpsLimit Maximum allowed FPS, specify 0 to disable.
     */
    void setFpsLimit(unsigned int iNewFpsLimit);

    /**
     * Sets user-adjusted gamma to undo during the gamma correction pass.
     *
     * @param value Gamma value.
     */
    void setGamma(float value);

    /**
     * Returns the maximum number of FPS that is allowed to be produced in a second.
     *
     * @return 0 if disabled.
     */
    unsigned int getFpsLimit() const { return renderStats.fpsLimitInfo.iFpsLimit; }

    /**
     * Returns user-adjusted gamma to undo it during the gamma correction pass.
     *
     * @return Gamma.
     */
    float getGamma() const { return gamma; }

    /**
     * Returns game's window.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the game's window.
     */
    Window* getWindow() const { return pWindow; }

    /**
     * Tells if a config file with user-specified render settings was loaded or not (not exists).
     *
     * @return `true` if exists and was loaded.
     */
    bool isUserRenderConfigLoaded() const { return bUserRenderConfigLoaded; }

    /**
     * Returns manager used to load and compile shaders.
     *
     * @remark As a game developer you don't need to use this. Materials use this function automatically.
     *
     * @return Manager.
     */
    ShaderManager& getShaderManager();

    /**
     * Returns manager used to add/remove light sources to/from rendering.
     *
     * @remark As a game developer you don't need to use this. Light nodes use this function automatically.
     *
     * @return Manager.
     */
    LightSourceManager& getLightSourceManager();

    /**
     * Returns manager used to add/remove UI nodes to/from rendering.
     *
     * @remark As a game developer you don't need to use this. Light nodes use this function automatically.
     *
     * @return Manager.
     */
    UiManager& getUiManager();

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

    /**
     * Returns settings for post processing of the rendered image.
     *
     * @return Settings.
     */
    PostProcessManager& getPostProcessManager();

private:
    /** Groups stuff used to synchronize GPU and CPU. */
    struct FrameSync {
        /** Fence per frame in-flight. */
        std::array<GLsync, 2> vFences; // 2 frames in-flight sees optimal, more can affect input latency

        /** Current index into @ref vFences. */
        unsigned int iCurrentFrameIndex = 0;
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

    /** Called by window that owns this renderer to draw a new frame. */
    void drawNextFrame();

    /**
     * Does post processing on the rendered image.
     *
     * @param pCameraProperties Properties of the main camera.
     */
    void drawPostProcessingScreenQuad(CameraProperties* pCameraProperties);

    /**
     * Draws a fullscreen quad to do gamma correction.
     *
     * @param iDrawFramebufferId Framebuffer to draw to.
     * @param iReadTextureId     Color texture to read from.
     */
    void drawGammaCorrectionScreenQuad(unsigned int iDrawFramebufferId, unsigned int iReadTextureId);

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

    /** UI rendering. */
    std::unique_ptr<UiManager> pUiManager;

    /** .ttf loading and rendering. */
    std::unique_ptr<FontManager> pFontManager;

    /** Light sources to render. */
    std::unique_ptr<LightSourceManager> pLightSourceManager;

    /** Framebuffer for rendering. */
    std::unique_ptr<Framebuffer> pMainFramebuffer;

    /** Settings for post processing of the rendered image. */
    std::unique_ptr<PostProcessManager> pPostProcessManager;

    /** Fullscreen quad for rendering. */
    std::unique_ptr<ScreenQuadGeometry> pFullscreenQuad;

    /** Shader for gamma correction. */
    std::shared_ptr<ShaderProgram> pGammaCorrectionShaderProgram;

    /** Various statistics about rendering. */
    RenderStatistics renderStats;

    /** GPU-CPU synchronization. */
    FrameSync frameSync;

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Always valid pointer. */
    Window* const pWindow = nullptr;

    /** User-adjusted gamma to undo it during the gamma correction pass. */
    float gamma = 2.2F;

    /** `true` if @ref sGlobalRenderConfigFilename exists and parameters from it were loaded. */
    bool bUserRenderConfigLoaded = false;

    /** Name of a configuration file that stores global user settings (used for all projects/games). */
    static constexpr std::string_view sGlobalRenderConfigFilename = "global_render_config.toml";
};
