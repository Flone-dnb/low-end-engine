#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
#include "render/ShaderManager.h"
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
     * Returns shader manager used to load and compile shaders.
     *
     * @remark As a game developer you don't need to use this. Materials use this function under the
     * hood.
     *
     * @return Manager.
     */
    ShaderManager& getShaderManager();

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

    /** Database of all shaders. */
    ShaderManager shaderManager;

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Always valid pointer. */
    Window* const pWindow = nullptr;
};
