#include "game/Window.h"

// Custom.
#include "render/Renderer.h"
#include "game/GameManager.h"
#include "io/Logger.h"
#include "render/SdlManager.hpp"

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
    pGameManager = std::get<std::unique_ptr<GameManager>>(std::move(result));

    pGameManager->onGameStarted();

    // Save reference to the renderer.
    const auto pRenderer = pGameManager->getRenderer();

    // Used for tick.
    unsigned long long iCurrentTimeCounter = SDL_GetPerformanceCounter();
    unsigned long long iPrevTimeCounter = 0;

    // Run game loop.
    bQuitRequested = false;
    while (!bQuitRequested) {
        // Process available window events.
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            bQuitRequested = processWindowEvent(event);
        }

        // Tick.
        iPrevTimeCounter = iCurrentTimeCounter;
        iCurrentTimeCounter = SDL_GetPerformanceCounter();
        const auto deltaTimeInMs = (iCurrentTimeCounter - iPrevTimeCounter) * 1000 / // NOLINT
                                   static_cast<double>(SDL_GetPerformanceFrequency());
        pGameManager->onBeforeNewFrame(static_cast<float>(deltaTimeInMs * 0.001)); // NOLINT

        // Draw.
        pRenderer->drawNextFrame();
    }

    // Notify game manager about window closed.
    pGameManager->onWindowClose();

    // Destroy game manager.
    pGameManager = nullptr;
    Logger::get().info("game manager is destroyed");
}

void Window::setCursorVisibility(bool bIsVisible) {
    if (SDL_SetRelativeMouseMode(static_cast<SDL_bool>(bIsVisible)) != 0) {
        Logger::get().error(SDL_GetError());
    }
}

void Window::close() { bQuitRequested = true; }

bool Window::processWindowEvent(const SDL_Event& event) {
    switch (event.type) {
    case (SDL_MOUSEMOTION): {
        onMouseMove(event.motion.xrel, event.motion.yrel);
        break;
    }
    case (SDL_MOUSEBUTTONDOWN): {
        onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), true);
        break;
    }
    case (SDL_MOUSEBUTTONUP): {
        onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), false);
        break;
    }
    case (SDL_KEYDOWN): {
        if (event.key.repeat == 0) {
            onKeyboardInput(
                static_cast<KeyboardKey>(event.key.keysym.sym),
                KeyboardModifiers(event.key.keysym.mod),
                true);
        }
        break;
    }
    case (SDL_KEYUP): {
        onKeyboardInput(
            static_cast<KeyboardKey>(event.key.keysym.sym), KeyboardModifiers(event.key.keysym.mod), false);
        break;
    }
    case (SDL_MOUSEWHEEL): {
        onMouseScrollMove(event.wheel.y);
        break;
    }
    case (SDL_WINDOWEVENT): {
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            onWindowFocusChanged(true);
        } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            onWindowFocusChanged(false);
        }
        break;
    }
    case (SDL_CONTROLLERDEVICEADDED): {
        SDL_GameControllerOpen(event.cdevice.which);
        break;
    }
    case (SDL_CONTROLLERDEVICEREMOVED): {
        SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
        break;
    }
    case (SDL_QUIT): {
        return true;
        break;
    }
    }

    return false;
}

void Window::onMouseMove(int iXOffset, int iYOffset) { pGameManager->onMouseMove(iXOffset, iYOffset); }

void Window::onWindowFocusChanged(bool bIsFocused) const { pGameManager->onWindowFocusChanged(bIsFocused); }

void Window::onKeyboardInput(KeyboardKey key, KeyboardModifiers modifiers, bool bIsPressedDown) const {
    pGameManager->onKeyboardInput(key, modifiers, bIsPressedDown);
}

void Window::onMouseScrollMove(int iOffset) const { pGameManager->onMouseScrollMove(iOffset); }

std::pair<unsigned int, unsigned int> Window::getWindowSize() const {
    int iWidth = 0;
    int iHeight = 0;
    SDL_GetWindowSize(pSdlWindow, &iWidth, &iHeight);

    return {static_cast<unsigned int>(iWidth), static_cast<unsigned int>(iHeight)};
}

std::pair<unsigned int, unsigned int> Window::getCursorPosition() const {
    showErrorIfNotOnMainThread();

    int iXPos = 0;
    int iYPos = 0;
    SDL_GetMouseState(&iXPos, &iYPos);

    return {iXPos, iYPos};
}

SDL_Window* Window::getSdlWindow() const { return pSdlWindow; }

GameManager* Window::getGameManager() const { return pGameManager.get(); }

void Window::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) const {
    pGameManager->onMouseInput(button, modifiers, bIsPressedDown);
}

Window::Window(SDL_Window* pCreatedWindow) {
    pSdlWindow = pCreatedWindow;

    // Save ID of this thread (should be main thread).
    mainThreadId = std::this_thread::get_id();
}

void Window::showErrorIfNotOnMainThread() const {
    const auto currentThreadId = std::this_thread::get_id();
    if (currentThreadId != mainThreadId) [[unlikely]] {
        Error error("an attempt was made to call a function that should only be called on the main thread");
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
    }
}

Window::~Window() { SDL_DestroyWindow(pSdlWindow); }
