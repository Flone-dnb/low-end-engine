#pragma once

class Window;

/**
 * Main game class, exists while the game window is not closed
 * (i.e. while the game is not closed).
 *
 * Reacts to user inputs, window events and etc.
 * Owned by game manager object.
 */
class GameInstance {
    // Game manager will create this object and trigger input events.
    friend class GameManager;

public:
    GameInstance() = delete;

    GameInstance(const GameInstance&) = delete;
    GameInstance& operator=(const GameInstance&) = delete;

    virtual ~GameInstance() = default;

    /**
     * Returns a reference to the window this game instance is using.
     *
     * @return A pointer to the window, should not be deleted.
     */
    Window* getWindow() const;

protected:
    /**
     * Creates a new game instance.
     *
     * @param pWindow Valid pointer to the game's window.
     */
    GameInstance(Window* pWindow);

    /**
     * Called after GameInstance's constructor is finished and created
     * GameInstance object was saved in GameManager (that owns GameInstance).
     *
     * At this point you can create and interact with the game world and etc.
     */
    virtual void onGameStarted() {}

    /**
     * Called before a new frame is rendered.
     *
     * @remark Called before nodes that should be called every frame.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    virtual void onBeforeNewFrame(float timeSincePrevCallInSec) {}

    /**
     * Called when the window that owns this game instance
     * was requested to close (no new frames will be rendered).
     *
     * Prefer to have your destructor logic here, because after this function is finished
     * the world will be destroyed and will be inaccessible (`nullptr`).
     */
    virtual void onWindowClose() {}

    /** Do not delete. Always valid pointer to the game's window. */
    Window* const pWindow = nullptr;
};
