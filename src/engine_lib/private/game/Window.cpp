#include "game/Window.h"

// Custom.
#include "render/Renderer.h"
#include "game/GameManager.h"
#include "render/SdlManager.hpp"

// External.
#include "SDL.h"

std::variant<std::unique_ptr<Window>, Error> Window::create(std::string_view sWindowName) {
    // Initialize SDL.
    SdlManager::init();

    // Get display resolution.
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    // Create SDL window.
    const auto pSdlWindow = SDL_CreateWindow(
        sWindowName.data(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        mode.w,
        mode.h,
        SDL_WINDOW_OPENGL);
    if (pSdlWindow == nullptr) [[unlikely]] {
        return Error(SDL_GetError());
    }

    auto pWindow = std::unique_ptr<Window>(new Window(pSdlWindow));

    return pWindow;
}

void Window::processEvents() {
    // Create game manager.
    auto result = GameManager::create(this);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }
    const auto pGameManager = std::get<std::unique_ptr<GameManager>>(std::move(result));

    // Save reference to the renderer.
    const auto pRenderer = pGameManager->getRenderer();

    // Run game loop.
    bool bQuitRequested = false;
    while (!bQuitRequested) {
        // Process available window events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                bQuitRequested = true;
            }

            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameControllerOpen(event.cdevice.which);
            }
            if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
            }

            if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                bQuitRequested = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                bQuitRequested = true;
            }
        }

        pRenderer->drawNextFrame();
    }
}

std::pair<unsigned int, unsigned int> Window::getWindowSize() const {
    int iWidth = 0;
    int iHeight = 0;
    SDL_GetWindowSize(pSdlWindow, &iWidth, &iHeight);

    return {static_cast<unsigned int>(iWidth), static_cast<unsigned int>(iHeight)};
}

SDL_Window* Window::getSdlWindow() const { return pSdlWindow; }

Window::Window(SDL_Window* pCreatedWindow) { pSdlWindow = pCreatedWindow; }

Window::~Window() { SDL_DestroyWindow(pSdlWindow); }
