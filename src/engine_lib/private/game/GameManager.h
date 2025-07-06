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
#include "misc/ThreadPool.h"
#include "game/World.h"

class Node;
class CameraManager;
class SoundManager;

/**
 * Controls main game objects: game instance, input manager, renderer,
 * audio engine, physics engine and etc.
 *
 * @remark Owned by Window object.
 */
class GameManager {
    // Only window is allowed to create a game manager.
    friend class Window;

    // World notifies about worlds being destroyed.
    friend class World;

public:
    /** Groups data used during world creation. */
    struct WorldCreationTask {
        /** Info about a task to load node tree as world. */
        struct LoadNodeTreeTask {
            ~LoadNodeTreeTask();

            /** Path to the file that stores the node tree to load. */
            std::filesystem::path pathToNodeTreeToLoad;

            /** `nullptr` if the node tree from @ref pathToNodeTreeToLoad is still being deserialized. */
            std::unique_ptr<Node> pLoadedNodeTreeRoot;

            /**
             * Tells if we started a thread pool task to deserialize @ref pathToNodeTreeToLoad into @ref
             * pLoadedNodeTreeRoot.
             */
            bool bIsAsyncTaskStarted = false;
        };

        ~WorldCreationTask();

        /** Callback to call after the world is created or loaded. */
        std::function<void(Node*)> onCreated;

        /** If nullptr then create an empty world (only root node), otherwise load the node tree. */
        std::unique_ptr<LoadNodeTreeTask> pOptionalNodeTreeLoadTask;

        /** `true` to destroy all existing worlds before creating a new one. */
        bool bDestroyOldWorlds = true;
    };

    /** Groups world-related data. */
    struct WorldData {
        ~WorldData();

        /**
         * Not nullptr if we need to create/load a new world.
         *
         * @remark We don't create/load worlds right away but instead create/load them on the next tick
         * because when a world creation task is received we might be iterating over "tickable" nodes
         * or nodes that receive input so we avoid modifying those arrays in that case.
         */
        std::unique_ptr<WorldCreationTask> pPendingWorldCreationTask;

        /** Currently active worlds. */
        std::vector<std::unique_ptr<World>> vWorlds;
    };

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    ~GameManager();

    /**
     * Creates a new world that contains only one node - root node.
     *
     * @remark Replaces the old world (if existed).
     *
     * @param onCreated         Callback function that will be called after the world is
     * created with world's root node passed as the only argument. Use GameInstance member functions as
     * callback functions for created worlds, because all nodes and other game objects will be destroyed
     * while the world is changing.
     * @param bDestroyOldWorlds `true` to destroy all previously existing worlds before creating the new
     * world.
     */
    void createWorld(const std::function<void(Node*)>& onCreated, bool bDestroyOldWorlds = true);

    /**
     * Asynchronously loads and deserializes a node tree as the new world. Node tree's root node will be
     * used as world's root node.
     *
     * @warning Use GameInstance member functions as callback functions for loaded worlds, because all
     * nodes and other game objects will be destroyed while the world is changing.
     *
     * @remark Replaces the old world (if existed).
     *
     * @param pathToNodeTreeFile Path to the file that contains a node tree to load, the ".toml"
     * extension will be automatically added if not specified.
     * @param onLoaded           Callback function that will be called on the main thread after the world
     * is loaded with root node passed as the only argument.
     */
    void loadNodeTreeAsWorld(
        const std::filesystem::path& pathToNodeTreeFile, const std::function<void(Node*)>& onLoaded);

    /**
     * Adds a function to be executed asynchronously on the thread pool.
     *
     * @warning If you are using a member functions/fields inside of the task you need to make
     * sure that the owner object of these member functions/fields will not be deleted until
     * this task is finished.
     *
     * @remark In the task you don't need to check if the game is being destroyed,
     * the engine makes sure all tasks are finished before the game is destroyed.
     *
     * @param task Function to execute.
     */
    void addTaskToThreadPool(const std::function<void()>& task);

    /**
     * Destroys the specified world and all nodes spawned in that world.
     *
     * @param pWorldToDestroy World to destroy.
     */
    void destroyWorld(World* pWorldToDestroy);

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

    /**
     * Returns sound manager.
     *
     * @return Sound manager.
     */
    SoundManager& getSoundManager() const;

    /**
     * Returns info about existing worlds.
     *
     * @return Worlds info.
     */
    std::pair<std::recursive_mutex, WorldData>& getWorlds() { return mtxWorldData; }

private:
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
     * Called before a world is destroyed.
     *
     * @param pRootNode Root node of a world about to be destroyed.
     */
    void onBeforeWorldDestroyed(Node* pRootNode);

    /** Destroys worlds from @ref mtxWorldData. */
    void destroyWorlds();

    /**
     * Must be called before destructor to destroy the world, all nodes and various systems
     *
     * @remark We destroy world before setting the `nullptr` to the game manager unique_ptr because
     * during game destruction (inside of this function) various nodes might access game manager
     * (and they are allowed while the world was not destroyed).
     */
    void destroy();

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
     * @param bIsRepeat      Whether this event if a "repeat button" event (used while holding the button
     * down).
     */
    void
    onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown, bool bIsRepeat);

    /**
     * Called when the window (that owns this object) receives an event about text character being
     * inputted.
     *
     * @param sTextCharacter Character that was typed.
     */
    void onKeyboardInputTextCharacter(const std::string& sTextCharacter);

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
     * Called when window's cursor changes its visibility.
     *
     * @param bVisibleNow `true` if the cursor is visible now.
     */
    void onCursorVisibilityChanged(bool bVisibleNow);

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

    /** Thread pool for various async tasks. */
    ThreadPool threadPool;

    /** Binds action/axis names with input keys. */
    InputManager inputManager;

    /** Created renderer. */
    std::unique_ptr<Renderer> pRenderer;

    /** Created game instance. */
    std::unique_ptr<GameInstance> pGameInstance;

    /** Manages sound and sound effects. */
    std::unique_ptr<SoundManager> pSoundManager;

    /** Game world, stores world's node tree. */
    std::pair<std::recursive_mutex, WorldData> mtxWorldData;

    /** Determines if @ref destroy was called or not. */
    bool bIsDestroyed = false;

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
