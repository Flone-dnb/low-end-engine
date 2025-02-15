#pragma once

// Standard.
#include <unordered_map>
#include <mutex>
#include <functional>

// Custom.
#include "input/KeyboardKey.hpp"
#include "input/MouseButton.hpp"
#include "input/GamepadButton.hpp"

class Window;
class InputManager;

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
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer.
     */
    Window* getWindow() const;

    /**
     * Returns a reference to the input manager this game instance is using.
     * Input manager allows binding IDs with multiple input keys that
     * you can receive in @ref onInputActionEvent.
     *
     * @warning Do not delete (free) returned pointer.
     *
     * @return Always valid pointer.
     */
    InputManager* getInputManager() const;

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
     * Called when the window (that owns this object) receives keyboard input.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    virtual void onKeyboardInput(KeyboardKey key, KeyboardModifiers modifiers, bool bIsPressedDown) {};

    /**
     * Called when the game receives gamepad input.
     *
     * @param button         Gamepad button.
     * @param bIsPressedDown Whether the button was pressed or released.
     */
    virtual void onGamepadInput(GamepadButton button, bool bIsPressedDown) {};

    /**
     * Called when the window (that owns this object) receives mouse input.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    virtual void onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {};

    /**
     * Called when the window received mouse movement.
     *
     * @param iXOffset Mouse X movement delta in pixels.
     * @param iYOffset Mouse Y movement delta in pixels.
     */
    virtual void onMouseMove(int iXOffset, int iYOffset) {};

    /**
     * Called when the window receives mouse scroll movement.
     *
     * @param iOffset Movement offset. Positive is away from the user, negative otherwise.
     */
    virtual void onMouseScrollMove(int iOffset) {}

    /**
     * Called after a gamepad controller was connected.
     *
     * @param sGamepadName Name of the connected gamepad.
     */
    virtual void onGamepadConnected(std::string_view sGamepadName) {}

    /** Called after a gamepad controller was disconnected. */
    virtual void onGamepadDisconnected() {};

    /**
     * Called when the window focus was changed.
     *
     * @param bIsFocused  Whether the window has gained or lost the focus.
     */
    virtual void onWindowFocusChanged(bool bIsFocused) {}

    /**
     * Called when the window that owns this game instance
     * was requested to close (no new frames will be rendered).
     *
     * Prefer to have your destructor logic here, because after this function is finished
     * the world will be destroyed and will be inaccessible (`nullptr`).
     */
    virtual void onWindowClose() {}

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
     * Returns map of action events that this GameInstance is bound to (must be used with mutex).
     * Bound callbacks will be automatically called when an action event is triggered.
     *
     * @remark Note that nodes can also have their input event bindings and you may prefer
     * to bind to input in specific nodes instead of binding to them in GameInstance.
     * @remark Only events in GameInstance's InputManager (GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     * @remark Called after @ref onKeyboardInput.
     *
     * Example:
     * @code
     * const unsigned int iForwardActionId = 0;
     * const auto pMtxActionEvents = getActionEventBindings();
     *      * std::scoped_lock guard(pMtxActionEvents->first);
     * pMtxActionEvents->second[iForwardActionId] = [&](KeyboardModifiers modifiers, bool
     * bIsPressedDown) {
     *     moveForward(modifiers, bIsPressedDown);
     * };
     * @endcode
     *
     * @return Bound action events.
     */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>*
    getActionEventBindings();

    /**
     * Returns map of axis events that this GameInstance is bound to (must be used with mutex).
     * Bound callbacks will be automatically called when an axis event is triggered.
     *
     * @remark Note that nodes can also have their input event bindings and you may prefer
     * to bind to input in specific nodes instead of binding to them in GameInstance.
     * @remark Only events in GameInstance's InputManager (GameInstance::getInputManager)
     * will be considered to trigger events in the node.
     * @remark Called after @ref onKeyboardInput.
     * @remark Input parameter is a value in range [-1.0f; 1.0f] that describes input.
     *
     * Example:
     * @code
     * const auto iForwardAxisEventId = 0;
     * const auto pMtxAxisEvents = getAxisEventBindings();
     *      * std::scoped_lock guard(pMtxAxisEvents->first);
     * pMtxAxisEvents->second[iForwardAxisEventId] = [&](KeyboardModifiers modifiers, float input) {
     *     moveForward(modifiers, input);
     * };
     * @endcode
     *
     * @return Bound action events.
     */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>*
    getAxisEventBindings();

private:
    /**
     * Called when a window that owns this game instance receives user
     * input and the input key exists as an action event in the InputManager.
     *
     * @remark Called after @ref onKeyboardInput.
     *
     * @param iActionId      Unique ID of the input action event (from input manager).
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onInputActionEvent(unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called when a window that owns this game instance receives user
     * input and the input key exists as an axis event in the InputManager.
     *
     * @remark Called after @ref onKeyboardInput and after @ref onInputActionEvent.
     *
     * @param iAxisEventId  Unique ID of the input axis event (from input manager).
     * @param modifiers     Keyboard modifier keys.
     * @param input         A value in range [-1.0f; 1.0f] that describes input.
     */
    void onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input);

    /** Map of action events that this GameInstance is bound to. Must be used with mutex. */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>
        mtxBindedActionEvents;

    /** Map of axis events that this GameInstance is bound to. Must be used with mutex. */
    std::pair<
        std::recursive_mutex,
        std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>
        mtxBindedAxisEvents;

    /** Do not delete. Always valid pointer to the game's window. */
    Window* const pWindow = nullptr;
};
