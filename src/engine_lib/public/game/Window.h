#pragma once

// Standard.
#include <memory>
#include <variant>
#include <thread>
#include <filesystem>

// Custom.
#include "misc/Error.h"
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "game/GameManager.h"
#include "io/Logger.h"
#include "misc/Profiler.hpp"

// External.
#include "SDL_timer.h"
#include "SDL_events.h"

class GameManager;

/** Parameters used to create a window. */
struct WindowBuilderParameters {
    /** Width of a window. */
    unsigned int iWindowWidth = 800;

    /** Height of a window. */
    unsigned int iWindowHeight = 600;

    /** Title of a window. */
    std::string_view sWindowTitle;

    /** Whether to show window after it was created or not. */
    bool bHidden = false;

    /** Whether the window should be maximized after creation or not. */
    bool bMaximized = false;

    /** Whether to show window in fullscreen mode. */
    bool bFullscreen = false;
};

class Window;

/** Builder pattern class for Window. */
class WindowBuilder {
public:
    WindowBuilder() = default;

    /**
     * Defines the size of a window that we will create.
     *
     * @remark Ignored if @ref maximized used.
     *
     * @param iWidth  Width of the window.
     * @param iHeight Height of the window.
     *
     * @return Builder.
     */
    WindowBuilder& size(unsigned int iWidth, unsigned int iHeight);

    /**
     * Defines the name of a window that we will create.
     *
     * @param sWindowTitle Title of the window.
     *
     * @return Builder.
     */
    WindowBuilder& title(std::string_view sWindowTitle);

    /**
     * Hides the window.
     *
     * @return Builder.
     */
    WindowBuilder& hidden();

    /**
     * Whether the window should be maximized after creation or not.
     *
     * @remark Ignored if @ref fullscreen is used.
     *
     * @return Builder.
     */
    WindowBuilder& maximized();

    /**
     * Show window in the fullscreen mode.
     *
     * @remark Note that your application might have better performance
     * and more stable frame pacing if you run it in fullscreen mode.
     *
     * @return Builder.
     */
    WindowBuilder& fullscreen();

    /**
     * Builds/creates a new window with the configured parameters.
     *
     * @return Returns error if something went wrong or created window otherwise.
     *
     * @warning This function should only be called from the main thread.
     */
    std::variant<std::unique_ptr<Window>, Error> build();

private:
    /** Configured window parameters. */
    WindowBuilderParameters params;
};

/** Manages a game window. */
class Window {
    // Only window builder can create windows.
    friend class WindowBuilder;

    // Asks cursor position.
    friend class CameraManager;

public:
    ~Window();

    /**
     * Whether the cursor is visible or not (locked in this window).
     *
     * @param bIsVisible 'true' shows cursor (normal behavior),
     * 'false' will hide the cursor and lock it to the window.
     */
    void setIsMouseCursorVisible(bool bIsVisible);

    /**
     * Starts the window message queue, rendering and game logic.
     *
     * @remark This function will return control after the window was closed.
     *
     * @param bRenderOnlyAfterInput Specify `true` to render a new frame only after the user input
     * was received. In this case ticking functions are not called every game tick but called after input or
     * if some small interval of time has passed. This mode can be useful in some UI only applications.
     * Specify `false` to render a new frame every game tick.
     */
    template <typename MyGameInstance>
        requires std::derived_from<MyGameInstance, GameInstance>
    void processEvents(bool bRenderOnlyAfterInput = false);

    /** Closes this window causing game instance, renderer, audio engine and etc. to be destroyed. */
    void close();

    /**
     * Sets a new size of the window.
     *
     * @param size Width and height in pixels.
     */
    void setWindowSize(const std::pair<unsigned int, unsigned int>& size);

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

    /**
     * Returns game manager.
     *
     * @return `nullptr` if not created yet or was destroyed, otherwise valid pointer.
     */
    GameManager* getGameManager() const;

    /**
     * Tells if the cursor is currently captured or not.
     *
     * @return Cursor visibility.
     */
    bool isMouseCursorVisible() const;

    /**
     * Tells if a gamepad is currently connected or not.
     *
     * @return Gamepad state.
     */
    bool isGamepadConnected() const;

    /**
     * Returns refresh rate of the screen.
     *
     * @return Refresh rate.
     */
    static unsigned int getScreenRefreshRate();

    /**
     * Called when the window (that owns this object) receives keyboard input.
     *
     * @remark Made public so that you can simulate input in your automated tests.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     * @param bIsRepeat      Whether this event if a "repeat button" event (used while holding the button
     * down).
     */
    void onKeyboardInput(
        KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown, bool bIsRepeat) const;

    /**
     * Called when the window (that owns this object) receives an event about text character being inputted.
     *
     * @param sTextCharacter Character that was typed.
     */
    void onKeyboardInputTextCharacter(const std::string& sTextCharacter);

    /**
     * Called when the window (that owns this object) receives mouse input.
     *
     * @remark Made public so that you can simulate input in your automated tests.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    void onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) const;

private:
    /**
     * Creates a new window.
     *
     * @param params Window creation params.
     *
     * @return Error if something went wrong, otherwise created window.
     */
    static std::variant<std::unique_ptr<Window>, Error> create(const WindowBuilderParameters& params);

    /**
     * Initializes the window.
     *
     * @param pCreatedWindow Created SDL window.
     * @param bIsFullscreen  `true` if the window is created as a fullscreen window.
     */
    Window(SDL_Window* pCreatedWindow, bool bIsFullscreen);

    /**
     * Looks for a connected gamepad and return it if found.
     *
     * @return `nullptr` if there's no gamepad connected.
     */
    static SDL_GameController* findConnectedGamepad();

    /**
     * Returns the current cursor position on window.
     *
     * @warning This function must only be called from the main thread. If this function is called
     * outside of the main thread an error will be shown.
     *
     * @return A pair of X and Y coordinates in pixels relative to the upper-left corner of the window.
     */
    std::pair<unsigned int, unsigned int> getCursorPosition() const;

    /** Checks whether the current thread is the main thread or not and if not shows an error. */
    void showErrorIfNotOnMainThread() const;

    /**
     * Processes the specified window event.
     *
     * @param event Event to process.
     *
     * @return `true` if should quit, `false` otherwise.
     */
    bool processWindowEvent(const SDL_Event& event);

    /** ID of the main thread. */
    std::thread::id mainThreadId;

    /** Created game manager. Can be `nullptr` if not created yet or was destroyed. */
    std::unique_ptr<GameManager> pGameManager;

    /** SDL window. */
    SDL_Window* pSdlWindow = nullptr;

    /** `nullptr` if there's a connected gamepad. */
    SDL_GameController* pConnectedGamepad = nullptr;

    /** Width and height of the window. */
    std::pair<unsigned int, unsigned int> windowSize;

    /** Current state of @ref setCursorVisibility. */
    bool bIsCursorVisible = true;

    /** Used in the window message loop. */
    bool bQuitRequested = false;

    /** `true` if this window was created as a fullscreen window. */
    const bool bIsCreatedAsFullscreenWindow = true;

    /** Index of a display (monitor) we use for rendering. */
    static constexpr int iUsedDisplayIndex = 0;
};

template <typename MyGameInstance>
    requires std::derived_from<MyGameInstance, GameInstance>
inline void Window::processEvents(bool bRenderOnlyAfterInput) {
    // Look for already connected gamepad.
    pConnectedGamepad = findConnectedGamepad();

    // Create game manager.
    auto result = GameManager::create<MyGameInstance>(this);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        error.showErrorAndThrowException();
    }
    pGameManager = std::get<std::unique_ptr<GameManager>>(std::move(result));

    pGameManager->onGameStarted();

    // Notify game about controller state.
    if (pConnectedGamepad != nullptr) {
        const auto pControllerName = SDL_GameControllerName(pConnectedGamepad);
        pGameManager->onGamepadConnected(pControllerName != nullptr ? pControllerName : "");
    }

    const auto pRenderer = pGameManager->getRenderer();

    // Used for tick.
    unsigned long long iCurrentTimeCounter = SDL_GetPerformanceCounter();
    unsigned long long iPrevTimeCounter = 0;

    // Some variables in case we need to only render after input.
    float timeSinceLastTickSec = 0.0F;
    constexpr float forceTickIntervalSec = 0.25F;

    // Run game loop.
    bQuitRequested = false;
    while (!bQuitRequested) {
        PROFILE_SCOPE("game loop cycle");

        // Process available window events.
        SDL_Event event;
        bool bHaveEventsToProcess = false;
        while (SDL_PollEvent(&event) != 0) {
            bHaveEventsToProcess = true;
            const auto bReceivedQuitEvent = processWindowEvent(event);

            // Use `OR` instead of assignment because the user can call `Window::close`.
            bQuitRequested |= bReceivedQuitEvent;
        }

        // Calculate delta time.
        iPrevTimeCounter = iCurrentTimeCounter;
        iCurrentTimeCounter = SDL_GetPerformanceCounter();
        const auto deltaTimeInMs = static_cast<double>(iCurrentTimeCounter - iPrevTimeCounter) * 1000.0 /
                                   static_cast<double>(SDL_GetPerformanceFrequency());
        const auto deltaTimeInSec = static_cast<float>(deltaTimeInMs * 0.001);
        timeSinceLastTickSec += deltaTimeInSec;

        if (!bRenderOnlyAfterInput || bHaveEventsToProcess || timeSinceLastTickSec >= forceTickIntervalSec) {
            // Process game tick.
            pGameManager->onBeforeNewFrame(deltaTimeInSec);
            timeSinceLastTickSec = 0.0F;

            // Draw frame.
            pRenderer->drawNextFrame(deltaTimeInSec);
        }

        if (bRenderOnlyAfterInput) {
            // Wait for next input.
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    // Notify game manager about window closed.
    pGameManager->onWindowClose();

    // Explicitly destroy game manager - destroy world, nodes and other stuff before setting the `nullptr` to
    // the game manager's unique_ptr because various nodes might access game manager while the world was not
    // destroyed yet (and they are allowed to do that).
    pGameManager->destroy();
    pGameManager = nullptr;
    Logger::get().info("game manager is destroyed");
}
