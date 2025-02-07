#pragma once

// Standard.
#include <memory>
#include <variant>

// Custom.
#include "misc/Error.h"

struct SDL_Window;
class Renderer;

/** Manages a game window. */
class Window {
public:
    ~Window();

    /**
     * Creates a new window.
     *
     * @param sWindowName Name of the window.
     *
     * @return Error if something went wrong, otherwise created window.
     */
    static std::variant<std::unique_ptr<Window>, Error> create(std::string_view sWindowName);

    /**
     * Starts the window message queue, rendering and game logic.
     *
     * @remark This function will return control after the window was closed.
     */
    void processEvents();

    /**
     * Returns size of the window in pixels.
     *
     * @return Size.
     */
    std::pair<unsigned int, unsigned int> getWindowSize() const;

    /**
     * Returns internal SDL window.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return SDL window.
     */
    SDL_Window* getSdlWindow() const;

private:
    /**
     * Initializes the window.
     *
     * @param pCreatedWindow Created SDL window.
     */
    Window(SDL_Window* pCreatedWindow);

    /** Renderer for this window. */
    std::unique_ptr<Renderer> pRenderer;

    /** SDL window. */
    SDL_Window* pSdlWindow = nullptr;
};
