#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
#include "render/GpuResourceManager.h"
#include "render/shader/ShaderManager.h"
#include "render/LightSourceManager.h"
#include "misc/Error.h"

// External.
#include "SDL_video.h"

class Window;

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
     * Returns manager for creating GPU resources.
     *
     * @return Manager.
     */
    GpuResourceManager& getGpuResourceManager();

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

private:
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

    /** Called by window that owns this renderer to draw a new frame. */
    void drawNextFrame();

    /** Creates GPU resources. */
    GpuResourceManager gpuResourceManager;

    /** Database of all shaders. */
    ShaderManager shaderManager;

    /** Light sources to render. */
    std::unique_ptr<LightSourceManager> pLightSourceManager;

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Always valid pointer. */
    Window* const pWindow = nullptr;
};
