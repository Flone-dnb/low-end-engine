#include "game/Window.h"

// Custom.
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
        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (pSdlWindow == nullptr) [[unlikely]] {
        return Error(SDL_GetError());
    }

    auto pWindow = std::unique_ptr<Window>(new Window(pSdlWindow));

    return pWindow;
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
        pGameManager->onMouseMove(event.motion.xrel, event.motion.yrel);
        break;
    }
    case (SDL_MOUSEBUTTONDOWN): {
        pGameManager->onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), true);
        break;
    }
    case (SDL_MOUSEBUTTONUP): {
        pGameManager->onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), false);
        break;
    }
    case (SDL_KEYDOWN): {
        if (event.key.repeat == 0) {
            pGameManager->onKeyboardInput(
                static_cast<KeyboardButton>(event.key.keysym.sym),
                KeyboardModifiers(event.key.keysym.mod),
                true);
        }
        break;
    }
    case (SDL_KEYUP): {
        pGameManager->onKeyboardInput(
            static_cast<KeyboardButton>(event.key.keysym.sym),
            KeyboardModifiers(event.key.keysym.mod),
            false);
        break;
    }
    case (SDL_MOUSEWHEEL): {
        pGameManager->onMouseScrollMove(event.wheel.y);
        break;
    }
    case (SDL_CONTROLLERBUTTONDOWN): {
        pGameManager->onGamepadInput(static_cast<GamepadButton>(event.cbutton.button), true);
        break;
    }
    case (SDL_CONTROLLERBUTTONUP): {
        pGameManager->onGamepadInput(static_cast<GamepadButton>(event.cbutton.button), false);
        break;
    }
    case (SDL_CONTROLLERAXISMOTION): {
        static_assert(std::is_same_v<decltype(event.caxis.value), short>, "expecting short");
        const auto movementPortion = static_cast<float>(event.caxis.value) / SHRT_MAX;
        pGameManager->onGamepadAxisMoved(static_cast<GamepadAxis>(event.caxis.axis), movementPortion);
        break;
    }
    case (SDL_WINDOWEVENT): {
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            pGameManager->onWindowFocusChanged(true);
        } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            pGameManager->onWindowFocusChanged(false);
        }
        break;
    }
    case (SDL_CONTROLLERDEVICEADDED): {
        if (pConnectedGamepad == nullptr) {
            pConnectedGamepad = SDL_GameControllerOpen(event.cdevice.which);
            const auto pControllerName = SDL_GameControllerName(pConnectedGamepad);
            pGameManager->onGamepadConnected(pControllerName != nullptr ? pControllerName : "");
        }
        break;
    }
    case (SDL_CONTROLLERDEVICEREMOVED): {
        if (pConnectedGamepad != nullptr &&
            event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pConnectedGamepad))) {
            SDL_GameControllerClose(pConnectedGamepad);
            pConnectedGamepad = nullptr;
            pGameManager->onGamepadDisconnected();
        }
        break;
    }
    case (SDL_QUIT): {
        return true;
        break;
    }
    }

    return false;
}

SDL_GameController* Window::findConnectedGamepad() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i) == SDL_TRUE) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

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
