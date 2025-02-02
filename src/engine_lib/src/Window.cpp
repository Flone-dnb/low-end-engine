#include "Window.h"

// Custom.
#include "Renderer.h"
#include "SdlManager.hpp"

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

    // Create renderer.
    auto result = Renderer::create(pWindow.get());
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        return error;
    }
    pWindow->pRenderer = std::get<std::unique_ptr<Renderer>>(std::move(result));

    return pWindow;
}

void Window::processEvents() {
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
