#pragma once

// Standard.
#include <stdexcept>

// External.
#include "SDL/include/SDL.h"
#include "glad/glad.h"

/** Handles SDL initialization and shutdown. */
class SdlManager {
public:
    ~SdlManager() { SDL_Quit(); }

    /** Initializes SDL. */
    static void init() { static SdlManager sdl; }

private:
    SdlManager() {
        // Initialize SDL.
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
            throw std::runtime_error("failed to initialize SDL");
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // IF CHANGING
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1); // ALSO CHANGE GLAD
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    }
};
