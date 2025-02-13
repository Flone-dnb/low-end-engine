#pragma once

// Standard.
#include <memory>
#include <variant>
#include <thread>

// Custom.
#include "misc/Error.h"
#include "input/MouseButton.hpp"
#include "input/KeyboardKey.hpp"

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

    /**
     * Called when the window receives mouse input.
     *
     * @remark Made public so you can simulate input in your tests.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    void onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) const;

    /**
     * Called when the window receives keyboard input.
     *
     * @remark Made public so you can simulate input in your tests.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onKeyboardInput(KeyboardKey key, KeyboardModifiers modifiers, bool bIsPressedDown) const;

    /**
     * Called when the window receives mouse scroll movement.
     *
     * @remark Made public so you can simulate input in your tests.
     *
     * @param iOffset Movement offset. Positive is away from the user, negative otherwise.
     */
    void onMouseScrollMove(int iOffset) const;

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
     * Called when the window receives mouse movement.
     *
     * @param iXOffset Mouse X movement delta in pixels.
     * @param iYOffset Mouse Y movement delta in pixels.
     */
    void onMouseMove(int iXOffset, int iYOffset);

    /**
     * Called when the window focus was changed.
     *
     * @param bIsFocused Whether the window has gained or lost the focus.
     */
    void onWindowFocusChanged(bool bIsFocused) const;

    /** ID of the main thread. */
    std::thread::id mainThreadId;

    /** Created game manager. Can be `nullptr` if not created yet or was destroyed. */
    std::unique_ptr<GameManager> pGameManager;

    /** SDL window. */
    SDL_Window* pSdlWindow = nullptr;

    /** Used in the window message loop. */
    bool bQuitRequested = false;
};
