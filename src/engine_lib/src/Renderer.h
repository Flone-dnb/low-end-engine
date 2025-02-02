#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
#include "Error.h"

// External.
#include "SDL.h"

class Window;

/** OpenGL ES renderer. */
class Renderer {
    // Only window can create a renderer.
    friend class Window;

public:
    ~Renderer();

private:
    /**
     * Creates a new renderer.
     *
     * @param pWindow Window that creates this renderer.
     *
     * @return Error if something went wrong, otherwise created renderer.
     */
    static std::variant<std::unique_ptr<Renderer>, Error> create(Window* pWindow);

    /**
     * Initializes the renderer.
     *
     * @param pWindow         Window that creates this renderer.
     * @param pCreatedContext Created OpenGL context.
     */
    Renderer(Window* pWindow, SDL_GLContext pCreatedContext);

    /** Called by window that owns this renderer to draw a new frame. */
    void drawNextFrame();

    /** OpenGL context. */
    SDL_GLContext pContext = nullptr;

    /** Do not delete (free) this pointer. Window that created this renderer, always valid pointer. */
    Window* const pWindow = nullptr;
};
