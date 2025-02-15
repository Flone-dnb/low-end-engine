#pragma once

// Standard.
#include <memory>
#include <variant>
#include <thread>

// Custom.
#include "misc/Error.h"
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "game/GameManager.h"
#include "io/Logger.h"

// External.
#include "SDL_timer.h"

class GameManager;

/** Manages a game window. */
class Window {
public:
    ~Window();

    /**
     * Creates a new window.
     *
     * @param sWindowName Name of the window.
     *
     * @return Error if something went wrong, otherwise created window.
     */
    static std::variant<std::unique_ptr<Window>, Error> create(std::string_view sWindowName);

    /**
     * Starts the window message queue, rendering and game logic.
     *
     * @remark This function will return control after the window was closed.
     */
    template <typename MyGameInstance>
        requires std::derived_from<MyGameInstance, GameInstance>
    void processEvents();

    /**
     * Whether the cursor is visible or not (locked in this window).
     *
     * @param bIsVisible 'true' shows cursor (normal behavior),
     * 'false' will hide the cursor and lock it to the window.
     */
    static void setCursorVisibility(bool bIsVisible);

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

private:
    /**
     * Initializes the window.
     *
     * @param pCreatedWindow Created SDL window.
     */
    Window(SDL_Window* pCreatedWindow);

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

    /**
     * Looks for a connected gamepad and return it if found.
     *
     * @return `nullptr` if there's no gamepad connected.
     */
    SDL_GameController* findConnectedGamepad();

    /** ID of the main thread. */
    std::thread::id mainThreadId;

    /** Created game manager. Can be `nullptr` if not created yet or was destroyed. */
    std::unique_ptr<GameManager> pGameManager;

    /** SDL window. */
    SDL_Window* pSdlWindow = nullptr;

    /** `nullptr` if there's a connected gamepad. */
    SDL_GameController* pConnectedGamepad = nullptr;

    /** Used in the window message loop. */
    bool bQuitRequested = false;
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
        error.showError();
        throw std::runtime_error(error.getFullErrorMessage());
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
