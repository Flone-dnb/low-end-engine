#pragma once

// Standard.
#include <format>

// Custom.
#include "misc/Error.h"

// External.
#include "fpng.h"
#define SDL_MAIN_HANDLED
#include "SDL3/SDL.h"
#include "glad/glad.h"

/** Handles initialization and shutdown of some systems. */
class InitManager {
public:
    ~InitManager() { SDL_Quit(); }

    /** Static initialization of the systems. */
    static void init() { static InitManager sdl; }

private:
    InitManager() {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            Error::showErrorAndThrowException(
                std::format("failed to initialize SDL, error: {}", SDL_GetError()));
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // IF CHANGING
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1); // ALSO CHANGE GLAD
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#if defined(DEBUG)
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG)) {
            Error::showErrorAndThrowException(
                std::format("failed to enable debug context: {}", SDL_GetError()));
        }
#endif

        fpng::fpng_init();
    }
};
