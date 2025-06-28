#include "game/Window.h"

// Custom.
#include "misc/InitManager.hpp"
#include "game/GameManager.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

WindowBuilder& WindowBuilder::size(unsigned int iWidth, unsigned int iHeight) {
    params.iWindowWidth = iWidth;
    params.iWindowHeight = iHeight;

    return *this;
}

WindowBuilder& WindowBuilder::title(std::string_view sWindowTitle) {
    params.sWindowTitle = sWindowTitle;

    return *this;
}

WindowBuilder& WindowBuilder::hidden() {
    params.bHidden = true;

    return *this;
}

WindowBuilder& WindowBuilder::maximized() {
    params.bMaximized = true;

    return *this;
}

WindowBuilder& WindowBuilder::fullscreen() {
    params.bFullscreen = true;

    return *this;
}

std::variant<std::unique_ptr<Window>, Error> WindowBuilder::build() { return Window::create(params); }

std::variant<std::unique_ptr<Window>, Error> Window::create(const WindowBuilderParameters& params) {
    InitManager::init();

    // Get display resolution.
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(iUsedDisplayIndex, &mode);
    int iWindowWidth = mode.w;
    int iWindowHeight = mode.h;

    if (!params.bFullscreen && !params.bMaximized) {
        iWindowWidth = static_cast<int>(params.iWindowWidth);
        iWindowHeight = static_cast<int>(params.iWindowHeight);
    }

    // Prepare flags.
    unsigned int iWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    if (params.bFullscreen) {
        iWindowFlags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_MAXIMIZED;
    } else {
        iWindowFlags |= SDL_WINDOW_RESIZABLE;
        if (params.bMaximized) {
            iWindowFlags |= SDL_WINDOW_MAXIMIZED;
        }
    }
    if (params.bHidden) {
        iWindowFlags |= SDL_WINDOW_HIDDEN;
    }

    // We don't need depth buffer for window's framebuffer (we have a depth buffer in our custom framebuffer).
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    // Create SDL window.
    const auto pSdlWindow = SDL_CreateWindow(
        params.sWindowTitle.data(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        iWindowWidth,
        iWindowHeight,
        iWindowFlags);
    if (pSdlWindow == nullptr) [[unlikely]] {
        return Error(SDL_GetError());
    }

    auto pWindow = std::unique_ptr<Window>(new Window(pSdlWindow, params.bFullscreen));

    // Log resulting window size.
    const auto createdWindowSize = pWindow->getWindowSize();
    Logger::get().info(
        std::format("created a new window of size {}x{}", createdWindowSize.first, createdWindowSize.second));

    return pWindow;
}

void Window::setCursorVisibility(bool bIsVisible) {
    if (SDL_SetRelativeMouseMode(static_cast<SDL_bool>(!bIsVisible)) != 0) {
        Logger::get().error(SDL_GetError());
        return;
    }

    bIsCursorVisible = bIsVisible;
}

void Window::close() { bQuitRequested = true; }

bool Window::processWindowEvent(const SDL_Event& event) {
    PROFILE_FUNC

    switch (event.type) {
    case (SDL_MOUSEMOTION): {
        pGameManager->onMouseMove(event.motion.xrel, event.motion.yrel);
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
        onKeyboardInput(
            static_cast<KeyboardButton>(event.key.keysym.scancode),
            KeyboardModifiers(event.key.keysym.mod),
            true,
            event.key.repeat != 0);
        break;
    }
    case (SDL_KEYUP): {
        onKeyboardInput(
            static_cast<KeyboardButton>(event.key.keysym.scancode),
            KeyboardModifiers(event.key.keysym.mod),
            false,
            event.key.repeat != 0);
        break;
    }
    case (SDL_TEXTINPUT): {
        onKeyboardInputTextCharacter(event.text.text);
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
        } else if (
            event.window.event == SDL_WINDOWEVENT_RESIZED ||
            event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
            event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
            // Save new size.
            int iWidth = 0;
            int iHeight = 0;
            SDL_GetWindowSizeInPixels(pSdlWindow, &iWidth, &iHeight);
            windowSize.first = static_cast<unsigned int>(iWidth);
            windowSize.second = static_cast<unsigned int>(iHeight);

            // Notify renderer.
            pGameManager->pRenderer->onWindowSizeChanged();
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

void Window::setWindowSize(const std::pair<unsigned int, unsigned int>& size) {
    if (bIsCreatedAsFullscreenWindow) [[unlikely]] {
        // TODO: implement window recreation and notify renderer
        Error::showErrorAndThrowException("changing size of a fullscreen window is not implemented");
    }

    windowSize = size;

    SDL_SetWindowSize(pSdlWindow, static_cast<int>(size.first), static_cast<int>(size.second));

    if (pGameManager != nullptr) {
        // Notify renderer.
        pGameManager->pRenderer->onWindowSizeChanged();
    }
}

std::pair<unsigned int, unsigned int> Window::getWindowSize() const { return windowSize; }

std::pair<unsigned int, unsigned int> Window::getCursorPosition() const {
    showErrorIfNotOnMainThread();

    int iXPos = 0;
    int iYPos = 0;
    SDL_GetMouseState(&iXPos, &iYPos);

    return {iXPos, iYPos};
}

SDL_Window* Window::getSdlWindow() const { return pSdlWindow; }

GameManager* Window::getGameManager() const { return pGameManager.get(); }

bool Window::isCursorVisible() const { return bIsCursorVisible; }

bool Window::isGamepadConnected() const { return pConnectedGamepad != nullptr; }

unsigned int Window::getScreenRefreshRate() {
    // Get display resolution.
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(iUsedDisplayIndex, &mode);

    return static_cast<unsigned int>(mode.refresh_rate);
}

void Window::onKeyboardInput(
    KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown, bool bIsRepeat) const {
    pGameManager->onKeyboardInput(key, modifiers, bIsPressedDown, bIsRepeat);
}

void Window::onKeyboardInputTextCharacter(const std::string& sTextCharacter) {
    pGameManager->onKeyboardInputTextCharacter(sTextCharacter);
}

void Window::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) const {
    pGameManager->onMouseInput(button, modifiers, bIsPressedDown);
}

Window::Window(SDL_Window* pCreatedWindow, bool bIsFullscreen) : bIsCreatedAsFullscreenWindow(bIsFullscreen) {
    pSdlWindow = pCreatedWindow;

    // Save ID of this thread (should be main thread).
    mainThreadId = std::this_thread::get_id();

    // Save size.
    int iWidth = 0;
    int iHeight = 0;
    SDL_GetWindowSizeInPixels(pSdlWindow, &iWidth, &iHeight);
    windowSize.first = static_cast<unsigned int>(iWidth);
    windowSize.second = static_cast<unsigned int>(iHeight);
}

void Window::showErrorIfNotOnMainThread() const {
    const auto currentThreadId = std::this_thread::get_id();
    if (currentThreadId != mainThreadId) [[unlikely]] {
        Error::showErrorAndThrowException(
            "an attempt was made to call a function that should only be called on the main thread");
    }
}

Window::~Window() { SDL_DestroyWindow(pSdlWindow); }
