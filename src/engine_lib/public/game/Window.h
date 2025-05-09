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

/** Manages a game window. */
class Window {
public:
    ~Window();

    /**
     * Creates a new window.
     *
     * @param sWindowName Name of the window.
     * @param bIsHidden   Specify `true` to create an invisible window (generally used in automated testing).
     *
     * @return Error if something went wrong, otherwise created window.
     */
    static std::variant<std::unique_ptr<Window>, Error>
    create(std::string_view sWindowName, bool bIsHidden = false);

    /**
     * Whether the cursor is visible or not (locked in this window).
     *
     * @param bIsVisible 'true' shows cursor (normal behavior),
     * 'false' will hide the cursor and lock it to the window.
     */
    void setCursorVisibility(bool bIsVisible);

    /**
     * Starts the window message queue, rendering and game logic.
     *
     * @remark This function will return control after the window was closed.
     */
    template <typename MyGameInstance>
        requires std::derived_from<MyGameInstance, GameInstance>
    void processEvents();

    /** Closes this window causing game instance, renderer, audio engine and etc. to be destroyed. */
    void close();

    /**
     * Returns size of the window in pixels.
     *
     * @return Size.
     */
    std::pair<unsigned int, unsigned int> getWindowSize() const;

    /**
     * Returns the current cursor position on window.
     *
     * @warning This function must only be called from the main thread. If this function is called
     * outside of the main thread an error will be shown.
     *
     * @return A pair of X and Y coordinates in pixels relative to the upper-left corner of the window.
     */
    std::pair<unsigned int, unsigned int> getCursorPosition() const;

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
     * Tells if the cursor is currently captured or not (see @ref setCursorVisibility).
     *
     * @return Cursor visibility.
     */
    bool isCursorVisible() const;

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
     */
    void onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown) const;

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
     * Initializes the window.
     *
     * @param pCreatedWindow Created SDL window.
     */
    Window(SDL_Window* pCreatedWindow);

    /**
     * Looks for a connected gamepad and return it if found.
     *
     * @return `nullptr` if there's no gamepad connected.
     */
    static SDL_GameController* findConnectedGamepad();

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

    /** Index of a display (monitor) we use for rendering. */
    static constexpr int iUsedDisplayIndex = 0;
};

template <typename MyGameInstance>
    requires std::derived_from<MyGameInstance, GameInstance>
inline void Window::processEvents() {
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

    // Save reference to the renderer.
    const auto pRenderer = pGameManager->getRenderer();

    // Used for tick.
    unsigned long long iCurrentTimeCounter = SDL_GetPerformanceCounter();
    unsigned long long iPrevTimeCounter = 0;

    // Run game loop.
    bQuitRequested = false;
    while (!bQuitRequested) {
        PROFILE_SCOPE("game loop cycle");

        // Process available window events.
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            const auto bReceivedQuitEvent = processWindowEvent(event);

            // Use `OR` instead of assignment because the user can call `Window::close`.
            bQuitRequested |= bReceivedQuitEvent;
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

    // Explicitly destroy game manager - destroy world, nodes and other stuff before setting the `nullptr` to
    // the game manager's unique_ptr because various nodes might access game manager while the world was not
    // destroyed yet (and they are allowed to do that).
    pGameManager->destroy();
    pGameManager = nullptr;
    Logger::get().info("game manager is destroyed");
}
