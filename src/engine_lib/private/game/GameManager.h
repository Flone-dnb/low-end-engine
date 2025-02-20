#pragma once

// Standard.
#include <memory>
#include <variant>
#include <mutex>
#include <functional>
#include <filesystem>

// Custom.
#include "input/KeyboardButton.hpp"
#include "input/MouseButton.hpp"
#include "input/InputManager.h"
#include "misc/Error.h"
#include "game/GameInstance.h"
#include "render/Renderer.h"

class Renderer;
class GameInstance;
class World;
class Node;

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
     * Creates a new world that contains only one node - root node.
     *
     * @remark Replaces the old world (if existed).
     *
     * @param onCreated Callback function that will be called after the world is
     * created. Use GameInstance member functions as callback functions for created worlds, because all nodes
     * and other game objects will be destroyed while the world is changing.
     */
    void createWorld(const std::function<void()>& onCreated);

    /**
     * Returns the total number of spawned nodes that receive input.
     *
     * @return Node count.
     */
    size_t getReceivingInputNodeCount();

    /**
     * Returns total amount of currently spawned nodes.
     *
     * @return Total nodes spawned right now.
     */
    size_t getTotalSpawnedNodeCount();

    /**
     * Returns the current amount of spawned nodes that are marked as "should be called every frame".
     *
     * @return Amount of spawned nodes that should be called every frame.
     */
    size_t getCalledEveryFrameNodeCount();

    /**
     * Returns a pointer to world's root node.
     *
     * @return `nullptr` if world is not created or was destroyed, otherwise valid pointer.
     */
    Node* getWorldRootNode();

    /**
     * Returns window that owns this object.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer to the window that owns this object.
     */
    Window* getWindow() const;

    /**
     * Returns game's input manager, manages input events of game instance and nodes.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer.
     */
    InputManager* getInputManager();

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
    /** Groups data used during world creation. */
    struct WorldCreationTask {
        /** Callback to call after the world is created or loaded. */
        std::function<void()> onCreated;

        /** If empty then create an empty world (only root node), otherwise load the node tree. */
        std::optional<std::filesystem::path> pathToNodeTreeToLoad;
    };

    /** Groups world-related data. */
    struct WorldData {
        /**
         * Not empty if we need to create/load a new world.
         *
         * @remark We don't create/load worlds right away but instead create/load them on the next tick
         * because when a world creation task is received we might be iterating over "tickable" nodes
         * or nodes that receive input so we avoid modifying those arrays in that case.
         */
        std::optional<WorldCreationTask> pendingWorldCreationTask;

        /** Current world. */
        std::unique_ptr<World> pWorld;
    };

    /**
     * Creates a new game manager.
     *
     * @param pWindow Window that owns this manager.
     *
     * @return Error if something went wrong, otherwise created game manager.
     */
    template <typename MyGameInstance>
        requires std::derived_from<MyGameInstance, GameInstance>
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
     * Called when the window (that owns this object) receives keyboard input.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called when the game receives gamepad input.
     *
     * @param button         Gamepad button.
     * @param bIsPressedDown Whether the button was pressed or released.
     */
    void onGamepadInput(GamepadButton button, bool bIsPressedDown);

    /**
     * Called when the game receives gamepad axis movement.
     *
     * @param axis     Gamepad axis that was moved.
     * @param position Axis position in range [-1.0; 1.0].
     */
    void onGamepadAxisMoved(GamepadAxis axis, float position);

    /**
     * Called when the window (that owns this object) receives mouse input.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    void onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called when the window received mouse movement.
     *
     * @param iXOffset Mouse X movement delta in pixels.
     * @param iYOffset Mouse Y movement delta in pixels.
     */
    void onMouseMove(int iXOffset, int iYOffset);

    /**
     * Called when the window receives mouse scroll movement.
     *
     * @param iOffset Movement offset. Positive is away from the user, negative otherwise.
     */
    void onMouseScrollMove(int iOffset);

    /**
     * Called after a gamepad controller was connected.
     *
     * @param sGamepadName Name of the connected gamepad.
     */
    void onGamepadConnected(std::string_view sGamepadName);

    /** Called after a gamepad controller was disconnected. */
    void onGamepadDisconnected();

    /**
     * Called when the window focus was changed.
     *
     * @param bIsFocused  Whether the window has gained or lost the focus.
     */
    void onWindowFocusChanged(bool bIsFocused) const;

    /**
     * Called when a window that owns this game instance
     * was requested to close (no new frames will be rendered).
     * Prefer to do your destructor logic here.
     */
    void onWindowClose() const;

    /**
     * Triggers action events from input.
     *
     * @param button         Button activated the event.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    void triggerActionEvents(
        std::variant<KeyboardButton, MouseButton, GamepadButton> button,
        KeyboardModifiers modifiers,
        bool bIsPressedDown);

    /**
     * Triggers axis events from keyboard input.
     *
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void triggerAxisEvents(KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Triggers axis events from gamepad axis input.
     *
     * @param gamepadAxis Gamepad axis that was moved.
     * @param position    Axis position in range [-1.0; 1.0].
     */
    void triggerAxisEvents(GamepadAxis gamepadAxis, float position);

    /** Binds action/axis names with input keys. */
    InputManager inputManager;

    /** Created renderer. */
    std::unique_ptr<Renderer> pRenderer;

    /** Created game instance. */
    std::unique_ptr<GameInstance> pGameInstance;

    /** Game world, stores world's node tree. */
    std::pair<std::recursive_mutex, WorldData> mtxWorldData;

    /** Do not delete this pointer. Window that owns this object, always valid pointer. */
    Window* const pWindow = nullptr;
};

template <typename MyGameInstance>
    requires std::derived_from<MyGameInstance, GameInstance>
inline std::variant<std::unique_ptr<GameManager>, Error> GameManager::create(Window* pWindow) {
    // Create renderer.
    auto result = Renderer::create(pWindow);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        return error;
    }
    auto pRenderer = std::get<std::unique_ptr<Renderer>>(std::move(result));

    // Create game instance.
    auto pGameInstance = std::make_unique<MyGameInstance>(pWindow);

    return std::unique_ptr<GameManager>(
        new GameManager(pWindow, std::move(pRenderer), std::move(pGameInstance)));
}
