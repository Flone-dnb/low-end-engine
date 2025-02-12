#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
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

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Always valid pointer. */
    Window* const pWindow = nullptr;
};
