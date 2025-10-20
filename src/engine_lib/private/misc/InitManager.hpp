#pragma once

// Standard.
#include <format>

// Custom.
#include "misc/Error.h"

// External.
#include "fpng.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "glad/glad.h"

/** Handles initialization and shutdown of some systems. */
class InitManager {
public:
    ~InitManager() { SDL_Quit(); }

    /** Static initialization of the systems. */
    static void init() { static InitManager sdl; }

private:
    InitManager() {
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
            Error::showErrorAndThrowException(
                std::format("failed to initialize SDL, error: {}", SDL_GetError()));
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // IF CHANGING
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1); // ALSO CHANGE GLAD
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

        fpng::fpng_init();
    }
};
