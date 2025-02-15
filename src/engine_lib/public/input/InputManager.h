#pragma once

// Standard.
#include <unordered_map>
#include <string>
#include <variant>
#include <optional>
#include <mutex>
#include <vector>

// Custom.
#include "input/KeyboardButton.hpp"
#include "input/MouseButton.hpp"
#include "input/GamepadButton.hpp"
#include "misc/Error.h"

/** Stores the current state of a button that triggers some input action event. */
class ActionEventTriggerButtonState {
public:
    ActionEventTriggerButtonState() = delete;

    /**
     * Initializes the state.
     *
     * @param triggerButton Button that triggers some action event.
     */
    ActionEventTriggerButtonState(std::variant<KeyboardButton, MouseButton, GamepadButton> triggerButton) {
        this->triggerButton = triggerButton;
    }

    /** Whether the button is pressed or not. */
    bool bIsPressed = false;

    /** Button that triggers some action event. */
    std::variant<KeyboardButton, MouseButton, GamepadButton> triggerButton;
};

/** Stores the current state of buttons that triggers some input axis event. */
class AxisEventTriggerButtonsState {
public:
    AxisEventTriggerButtonsState() = delete;

    /**
     * Initializes the state.
     *
     * @param positiveTrigger  Triggers input value of `+1`.
     * @param negativeTrigger Triggers input value of `-1`.
     */
    AxisEventTriggerButtonsState(KeyboardButton positiveTrigger, KeyboardButton negativeTrigger) {
        this->positiveTrigger = positiveTrigger;
        this->negativeTrigger = negativeTrigger;

        bIsPositiveTriggerPressed = false;
        bIsNegativeTriggerPressed = false;
    }

    /** Triggers input value of `+1`. */
    KeyboardButton positiveTrigger;

    /** Triggers input value of `-1`. */
    KeyboardButton negativeTrigger;

    /** Whether @ref positiveTrigger is currently pressed or not. */
    bool bIsPositiveTriggerPressed = false;

    /** Whether @ref negativeTrigger is currently pressed or not. */
    bool bIsNegativeTriggerPressed = false;
};

/** Allows binding IDs with multiple input buttons. */
class InputManager {
    // Triggers input events.
    friend class GameManager;

public:
    InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /**
     * Adds a new action event.
     *
     * Action event allows binding buttons that have only 2 states (pressed/not pressed) to an ID.
     * When one of the specified buttons is pressed you will receive an input action event with the specified
     * ID.
     *
     * This way, for example, you can have an action "jump" bound to a "space bar" button and can easily
     * change the input button "space bar" to something else if the user wants to. For this, just call @ref
     * modifyActionEvent to change one button of the action.
     *
     * @param iActionId       Unique ID of the new action event.
     * @param vTriggerButtons Buttons associated with this input event.
     *
     * @return Returns an error if passed array of trigger buttons is empty or if an
     * action event with this ID is already registered.
     */
    [[nodiscard]] std::optional<Error> addActionEvent(
        unsigned int iActionId,
        const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vTriggerButtons);

    /**
     * Adds a new axis event.
     *
     * Axis events operate on pairs of 2 buttons where one triggers an input value `+1.0`
     * and the other `-1.0`. Input devices with "floating" values (such as gamepad sticks/triggers)
     * are a perfect fit this this type of input events. For example, a gamepad stick can
     * be used in 2 axis events: "move forward" and "move right". But you can also
     * bind keyboard buttons to `+1.0` and `-1.0` input, this way you will support both
     * keyboard and gamepad users: an axis event "move forward" will typically have
     * 2 pairs of triggers: W and gamepad stick up for `+1.0`, S and gamepad stick down for `-1.0`.
     *
     * @param iAxisEventId  Unique ID of the new axis event.
     * @param vTriggerPairs The first button will be associated with `+1.0` input and the second with `-1.0`
     * input.
     *
     * @return Returns an error if passed array of trigger buttons is empty or if an
     * axis event with this ID is already registered.
     */
    [[nodiscard]] std::optional<Error> addAxisEvent(
        unsigned int iAxisEventId,
        const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vTriggerPairs);

    /**
     * Change action event's trigger button.
     *
     * @param iActionId Unique ID of the action event to modify.
     * @param oldButton Button of the specified action event that you want to replace.
     * @param newButton New button that should replace the old one.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> modifyActionEvent(
        unsigned int iActionId,
        std::variant<KeyboardButton, MouseButton, GamepadButton> oldButton,
        std::variant<KeyboardButton, MouseButton, GamepadButton> newButton);

    /**
     * Change axis event's trigger button.
     *
     * @param iAxisEventId Unique ID of the axis event to modify.
     * @param oldPair      A pair of triggers of the specified axis event that you want to replace.
     * @param newPair      A new pair of new triggers that should replace the old pair.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> modifyAxisEvent(
        unsigned int iAxisEventId,
        std::pair<KeyboardButton, KeyboardButton> oldPair,
        std::pair<KeyboardButton, KeyboardButton> newPair);

    /**
     * Saves added action/axis events to a file.
     *
     * @param sFileName   Name of the file to save, prefer to have only ASCII characters in the
     * file name. We will save it to a predefined directory using SETTINGS category,
     * the .toml extension will be added if the passed name does not have it.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> saveToFile(std::string_view sFileName);

    /**
     * Loads action/axis events from a file.
     *
     * @warning This function will only read action/axis events that exist
     * in this input manager. Buttons from the specified file will just replace the current buttons of
     * existing action/axis event.
     *
     * The usual workflow for working with input goes like this:
     * - add your action/axis events with some default buttons,
     * - the user may change the buttons of action/axis events during game,
     * - save changed events using @ref saveToFile,
     * - on next startup add your action/axis events with some default buttons,
     * - use @ref overwriteExistingEventsButtonsFromFile to load previously changed buttons.
     *
     * @param sFileName Name of the file to load. It will be loaded from a predefined directory using
     * the `SETTINGS` category (see @ref ProjectPaths), the `.toml` extension will be added if the passed name
     * does not have it.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> overwriteExistingEventsButtonsFromFile(std::string_view sFileName);

    /**
     * Returns action and axis event IDs that the specified button is used in.
     *
     * You can use this function to detect conflicting buttons. For example:
     * when the user wants to modify some event and you receive a `newButton`,
     * look if this `newButton` is already used somewhere and if it's, show a message to the user.
     *
     * @param button A button to see where it's used.
     *
     * @return A pair of action and axis event IDs that the specified button is used in.
     */
    std::pair<std::vector<unsigned int>, std::vector<unsigned int>>
    isButtonUsed(const std::variant<KeyboardButton, MouseButton, GamepadButton>& button);

    /**
     * Looks for an action event with the specified ID and returns a copy of this event's trigger buttons.
     * Changes made to the returned array will not be applied to the event, use @ref modifyActionEvent for
     * this purpose.
     *
     * @param iActionId Unique ID of an action event to look for.
     *
     * @return Keys associated with the action event.
     */
    std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>
    getActionEventButtons(unsigned int iActionId);

    /**
     * Looks for an axis event with the specified ID and returns a copy of this event's trigger buttons.
     * Changes made to the returned vector will not be applied to the axis, use @ref modifyAxisEvent for this
     * purpose.
     *
     * @param iAxisEventId Unique ID of the axis event to look for.
     *
     * @return Triggers associated with the axis event.
     */
    std::vector<std::pair<KeyboardButton, KeyboardButton>> getAxisEventTriggers(unsigned int iAxisEventId);

    /**
     * Returns the current value of an axis event.
     * This value is equal to the last value passed to GameInstance::onInputAxisEvent.
     *
     * @param iAxisEventId Unique ID of the axis event that you used in @ref addAxisEvent.
     *
     * @return Zero if axis event with this ID does not exist, last input value otherwise.
     */
    float getCurrentAxisEventState(unsigned int iAxisEventId);

    /**
     * Removes an action event with the specified ID.
     *
     * @param iActionId Unique ID of the action to remove.
     *
     * @return `false` if the action was found and removed, `true` if not.
     */
    bool removeActionEvent(unsigned int iActionId);

    /**
     * Removes an axis event with the specified ID.
     *
     * @param iAxisEventId Unique ID of the axis event to remove.
     *
     * @return `false` if the axis was found and removed, `true` if not.
     */
    bool removeAxisEvent(unsigned int iAxisEventId);

    /**
     * Returns all action events.
     *
     * @return A copy of all action events.
     */
    std::unordered_map<unsigned int, std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>>
    getAllActionEvents();

    /**
     * Returns all axis events.
     *
     * @return A copy of all axis events.
     */
    std::unordered_map<unsigned int, std::vector<std::pair<KeyboardButton, KeyboardButton>>>
    getAllAxisEvents();

    /**
     * Splits the string using a delimiter.
     *
     * @param sStringToSplit String to split.
     * @param sDelimiter     Delimiter.
     *
     * @return Splitted string.
     */
    static std::vector<std::string>
    splitString(const std::string& sStringToSplit, const std::string& sDelimiter);

private:
    /**
     * Adds a new action event or if an action event with this ID already exists it will be removed
     * to register this new action event instead.
     *
     * @warning If this action event is triggered with an old button right now
     * (when you call this function), there is a chance that this action will be triggered
     * using old buttons for the last time (even if after you removed this action).
     * This is because when we receive input button we make a copy of all actions
     * associated with the button and then call these actions, because we operate
     * on a copy, removed elements will be reflected only on the next user input.
     *
     * @param iActionId   Unique ID of an action event to overwrite.
     * @param vButtons    Buttons associated with this action.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> overwriteActionEvent(
        unsigned int iActionId,
        const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vButtons);

    /**
     * Adds a new axis event or if an axis event with this ID already exists it will be removed
     * to register this new axis event instead.
     *
     * @warning See the warning from @ref overwriteActionEvent about old buttons still triggering input.
     *
     * @param iAxisEventId  Unique ID of an axis event to overwrite.
     * @param vTriggerPairs A pair of triggers associated with this axis event,
     * first button will be associated with `+1.0` input and the second with `-1.0` input.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> overwriteAxisEvent(
        unsigned int iAxisEventId,
        const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vTriggerPairs);

    /** Map that stores pairs of "button that triggers event" - "registered input event IDs". */
    std::unordered_map<std::variant<KeyboardButton, MouseButton, GamepadButton>, std::vector<unsigned int>>
        actionEvents;

    /**
     * Stores pairs of "action event ID" and a pair of "registered buttons that trigger the event" - "input
     * event state (pressed/not pressed)".
     */
    std::unordered_map<unsigned int, std::pair<std::vector<ActionEventTriggerButtonState>, bool>>
        actionEventsTriggerButtonsState;

    /**
     * Stores pairs of "event trigger" - "array of registered input events with an input value that should be
     * triggered (either +1 (`true`) or -1 (`false`))".
     */
    std::unordered_map<KeyboardButton, std::vector<std::pair<unsigned int, bool>>> axisEvents;

    /**
     * Stores pairs of "axis event ID" and a pair of "registered triggers" - "input event state in range
     * [-1.0; 1.0]".
     */
    std::unordered_map<unsigned int, std::pair<std::vector<AxisEventTriggerButtonsState>, int>> axisState;

    /** Mutex for actions editing. */
    std::recursive_mutex mtxActionEvents;

    /** Mutex for axis editing. */
    std::recursive_mutex mtxAxisEvents;

    /** Section name to store action events, used in .toml files. */
    static inline const std::string_view sActionEventFileSectionName = "action event";

    /** Section name to store axis events, used in .toml files. */
    static inline const std::string_view sAxisEventFileSectionName = "axis event";
};
