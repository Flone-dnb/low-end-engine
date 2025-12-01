#include "game/Window.h"

// Custom.
#include "misc/InitManager.hpp"
#include "game/GameManager.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

#if defined(__aarch64__) || defined(__ARM64__)
#define IS_ARM64
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

    int iWindowWidth = 0;
    int iWindowHeight = 0;

    {
        int iDisplayCount = 0;
        const auto pDisplays = SDL_GetDisplays(&iDisplayCount);
        if (iDisplayCount == 0) [[unlikely]] {
            Error::showErrorAndThrowException("unable to find at least 1 display");
        }

        // Get display resolution.
        const SDL_DisplayMode* pMode = SDL_GetDesktopDisplayMode(pDisplays[iUsedDisplayIndex]);
        iWindowWidth = pMode->w;
        iWindowHeight = pMode->h;

        SDL_free(pDisplays);
    }

    if (!params.bFullscreen && !params.bMaximized) {
        iWindowWidth = static_cast<int>(params.iWindowWidth);
        iWindowHeight = static_cast<int>(params.iWindowHeight);
    }

#if defined(ENGINE_UI_ONLY)
    // Don't need depth buffer.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
#else
    if (!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to set depth buffer size, error: {}", SDL_GetError()));
    }
#endif

    // Create SDL window.
    SDL_PropertiesID props{SDL_CreateProperties()};
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, params.sWindowTitle.data());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, iWindowWidth);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, iWindowHeight);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    if (params.bFullscreen) {
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
    } else {
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        if (params.bMaximized) {
            SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
        }
    }
    if (params.bHidden) {
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    }
    const auto pSdlWindow = SDL_CreateWindowWithProperties(props);
    if (pSdlWindow == nullptr) [[unlikely]] {
        return Error(SDL_GetError());
    }
    SDL_DestroyProperties(props);

    auto pWindow = std::unique_ptr<Window>(new Window(pSdlWindow, params.bFullscreen));

    // Log resulting window size.
    const auto createdWindowSize = pWindow->getWindowSize();
    Log::info(
        std::format("created a new window of size {}x{}", createdWindowSize.first, createdWindowSize.second));

    return pWindow;
}

void Window::setIsMouseCursorVisible(bool bIsVisible) {
    if (bIsVisible == bIsCursorVisible) {
        return;
    }

    if (!SDL_SetWindowRelativeMouseMode(pSdlWindow, !bIsVisible)) {
        Log::error(SDL_GetError());
        return;
    }

    bIsCursorVisible = bIsVisible;

    pGameManager->onCursorVisibilityChanged(bIsCursorVisible);
}

void Window::close() { bQuitRequested = true; }

bool Window::processWindowEvent(const SDL_Event& event) {
    PROFILE_FUNC

    switch (event.type) {
    case (SDL_EVENT_MOUSE_MOTION): {
        pGameManager->onMouseMove(static_cast<int>(event.motion.xrel), static_cast<int>(event.motion.yrel));
        break;
    }
    case (SDL_EVENT_MOUSE_BUTTON_DOWN): {
        onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), true);
        break;
    }
    case (SDL_EVENT_MOUSE_BUTTON_UP): {
        onMouseInput(
            static_cast<MouseButton>(event.button.button), KeyboardModifiers(SDL_GetModState()), false);
        break;
    }
    case (SDL_EVENT_KEY_DOWN): {
        onKeyboardInput(
            static_cast<KeyboardButton>(event.key.scancode),
            KeyboardModifiers(event.key.mod),
            true,
            event.key.repeat != 0);
        break;
    }
    case (SDL_EVENT_KEY_UP): {
        onKeyboardInput(
            static_cast<KeyboardButton>(event.key.scancode),
            KeyboardModifiers(event.key.mod),
            false,
            event.key.repeat != 0);
        break;
    }
    case (SDL_EVENT_TEXT_INPUT): {
        onKeyboardInputTextCharacter(event.text.text);
        break;
    }
    case (SDL_EVENT_MOUSE_WHEEL): {
        pGameManager->onMouseScrollMove(static_cast<int>(event.wheel.y));
        break;
    }
    case (SDL_EVENT_GAMEPAD_BUTTON_DOWN): {
        pGameManager->onGamepadInput(static_cast<GamepadButton>(event.gbutton.button), true);
        break;
    }
    case (SDL_EVENT_GAMEPAD_BUTTON_UP): {
        pGameManager->onGamepadInput(static_cast<GamepadButton>(event.gbutton.button), false);
        break;
    }
    case (SDL_EVENT_GAMEPAD_AXIS_MOTION): {
        static_assert(std::is_same_v<decltype(event.gaxis.value), short>, "expecting short");
        const auto movementPortion = static_cast<float>(event.gaxis.value) / SHRT_MAX;
        pGameManager->onGamepadAxisMoved(static_cast<GamepadAxis>(event.gaxis.axis), movementPortion);
        break;
    }
    case (SDL_EVENT_WINDOW_FOCUS_GAINED): {
        pGameManager->onWindowFocusChanged(true);
        break;
    }
    case (SDL_EVENT_WINDOW_FOCUS_LOST): {
        pGameManager->onWindowFocusChanged(false);
        break;
    }
    case (SDL_EVENT_WINDOW_RESIZED):
    case (SDL_EVENT_WINDOW_MAXIMIZED):
    case (SDL_EVENT_WINDOW_MINIMIZED): {
        // Save new size.
        int iWidth = 0;
        int iHeight = 0;
        SDL_GetWindowSizeInPixels(pSdlWindow, &iWidth, &iHeight);
        windowSize.first = static_cast<unsigned int>(iWidth);
        windowSize.second = static_cast<unsigned int>(iHeight);

        // Notify.
        pGameManager->onWindowSizeChanged();
        break;
    }
    case (SDL_EVENT_GAMEPAD_ADDED): {
        if (pConnectedGamepad == nullptr) {
            pConnectedGamepad = SDL_OpenGamepad(event.cdevice.which);
            const auto pControllerName = SDL_GetGamepadName(pConnectedGamepad);
            pGameManager->onGamepadConnected(pControllerName != nullptr ? pControllerName : "");
        }
        break;
    }
    case (SDL_EVENT_GAMEPAD_REMOVED): {
        if (pConnectedGamepad != nullptr &&
            event.cdevice.which == SDL_GetJoystickID(SDL_GetGamepadJoystick(pConnectedGamepad))) {
            SDL_CloseGamepad(pConnectedGamepad);
            pConnectedGamepad = nullptr;
            pGameManager->onGamepadDisconnected();
        }
        break;
    }
    case (SDL_EVENT_QUIT): {
        return true;
        break;
    }
    }

    return false;
}

SDL_Gamepad* Window::findConnectedGamepad() {
    int iCount = 0;
    const auto pIds = SDL_GetGamepads(&iCount);

    SDL_Gamepad* pGamepad = nullptr;

    for (int i = 0; i < iCount; i++) {
        if (SDL_IsGamepad(pIds[i])) {
            pGamepad = SDL_OpenGamepad(pIds[i]);
            break;
        }
    }

    SDL_free(pIds);

    return pGamepad;
}

void Window::setWindowSize(const std::pair<unsigned int, unsigned int>& size) {
    if (bIsCreatedAsFullscreenWindow) [[unlikely]] {
        // TODO: implement window recreation and notify renderer
        Error::showErrorAndThrowException("changing size of a fullscreen window is not implemented");
    }

    windowSize = size;

    SDL_SetWindowSize(pSdlWindow, static_cast<int>(size.first), static_cast<int>(size.second));

    if (pGameManager != nullptr) {
        // Notify.
        pGameManager->onWindowSizeChanged();
    }
}

std::pair<unsigned int, unsigned int> Window::getWindowSize() const { return windowSize; }

std::pair<unsigned int, unsigned int> Window::getCursorPosition() const {
    showErrorIfNotOnMainThread();

    float xPos = 0.0f;
    float yPos = 0.0f;
    SDL_GetMouseState(&xPos, &yPos);

    return {static_cast<unsigned int>(xPos), static_cast<unsigned int>(yPos)};
}

SDL_Window* Window::getSdlWindow() const { return pSdlWindow; }

GameManager* Window::getGameManager() const { return pGameManager.get(); }

bool Window::isMouseCursorVisible() const { return bIsCursorVisible; }

bool Window::isGamepadConnected() const { return pConnectedGamepad != nullptr; }

unsigned int Window::getScreenRefreshRate() {
    int iDisplayCount = 0;
    const auto pDisplays = SDL_GetDisplays(&iDisplayCount);
    if (iDisplayCount == 0) [[unlikely]] {
        Error::showErrorAndThrowException("unable to find at least 1 display");
    }

    const SDL_DisplayMode* pMode = SDL_GetDesktopDisplayMode(pDisplays[iUsedDisplayIndex]);
    const auto refreshRate = pMode->refresh_rate;

    SDL_free(pDisplays);

    return static_cast<unsigned int>(refreshRate);
}

void Window::onKeyboardInput(
    KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown, bool bIsRepeat) const {
#if defined(IS_ARM64)
    if (pConnectedGamepad != nullptr) {
        // In some cases while using retro-handhelds (they have built in gamepad) gamepad buttons trigger
        // keyboard input before the actual gamepad button input.
        return;
    }
#endif
    pGameManager->onKeyboardInput(key, modifiers, bIsPressedDown, bIsRepeat);
}

void Window::onKeyboardInputTextCharacter(const std::string& sTextCharacter) {
#if defined(IS_ARM64)
    if (pConnectedGamepad != nullptr) {
        // Same reason as in onKeyboardInput.
        return;
    }
#endif
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
