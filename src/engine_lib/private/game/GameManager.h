#pragma once

// Standard.
#include <memory>
#include <variant>
#include <mutex>

// Custom.
#include "misc/Error.h"

class Renderer;
class GameInstance;
class World;

/**
 * Controls main game objects: game instance, input manager, renderer,
 * audio engine, physics engine and etc.
 *
 * @remark Owned by Window object.
 */
class GameManager {
    // Only window is allowed to create a game manager.
    friend class Window;

public:
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    ~GameManager();

    /**
     * Returns window that owns this object.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the window that owns this object.
     */
    Window* getWindow() const;

    /**
     * Returns game's renderer.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the game's renderer.
     */
    Renderer* getRenderer() const;

    /**
     * Returns game instance.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the game instance.
     */
    GameInstance* getGameInstance() const;

private:
    /**
     * Creates a new game manager.
     *
     * @param pWindow Window that owns this manager.
     *
     * @return Error if something went wrong, otherwise created game manager.
     */
    static std::variant<std::unique_ptr<GameManager>, Error> create(Window* pWindow);

    /**
     * Initializes the game manager.
     *
     * @remark This function is used internally, use @ref create to create a new game manager.
     *
     * @param pWindow       Window that owns this object.
     * @param pRenderer     Created renderer to use.
     * @param pGameInstance Created game instance to use.
     */
    GameManager(
        Window* pWindow, std::unique_ptr<Renderer> pRenderer, std::unique_ptr<GameInstance> pGameInstance);

    /**
     * Called by owner Window to notify game instance about game being started (everything is set up).
     *
     * @remark Expects game instance to exist.
     */
    void onGameStarted();

    /**
     * Called before a new frame is rendered.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    void onBeforeNewFrame(float timeSincePrevCallInSec);

    /**
     * Called when a window that owns this game instance
     * was requested to close (no new frames will be rendered).
     * Prefer to do your destructor logic here.
     */
    void onWindowClose() const;

    /** Created renderer. */
    std::unique_ptr<Renderer> pRenderer;

    /** Created game instance. */
    std::unique_ptr<GameInstance> pGameInstance;

    /** Game world, stores world's node tree. */
    std::pair<std::recursive_mutex, std::unique_ptr<World>> mtxWorld;

    /** Do not delete this pointer. Window that owns this object, always valid pointer. */
    Window* const pWindow = nullptr;
};
