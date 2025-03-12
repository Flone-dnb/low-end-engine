#include "input/InputManager.h"

// Standard.
#include <charconv>
#include <string>
#include <format>
#include <vector>
#include <unordered_set>

// Custom.
#include "io/ConfigManager.h"

std::optional<Error> InputManager::addActionEvent(
    unsigned int iActionId,
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vTriggerButtons) {
    // Make sure there is an least one key specified to trigger this event.
    if (vTriggerButtons.empty()) [[unlikely]] {
        return Error("vKeys is empty");
    }

    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    // Check if an action with this name already exists.
    const auto vRegisteredActions = getAllActionEvents();
    const auto existingActionId = vRegisteredActions.find(iActionId);
    if (existingActionId != vRegisteredActions.end()) [[unlikely]] {
        return Error(std::format("an action with the ID {} already exists", iActionId));
    }

    // Add action event.
    auto optionalError = overwriteActionEvent(iActionId, vTriggerButtons);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }

    return {};
}

std::optional<Error> InputManager::addAxisEvent(
    unsigned int iAxisEventId,
    const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vKeyboardTriggers,
    const std::vector<GamepadAxis>& vGamepadAxis) {
    // Make sure there is an least one button specified to trigger this event.
    if (vKeyboardTriggers.empty() && vGamepadAxis.empty()) [[unlikely]] {
        return Error("specified array of triggers is empty");
    }

    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Check if an axis event with this name already exists.
    const auto vRegisteredAxisEvents = getAllAxisEvents();
    const auto action = vRegisteredAxisEvents.find(iAxisEventId);
    if (action != vRegisteredAxisEvents.end()) [[unlikely]] {
        return Error(std::format("an axis event with the ID {} already exists", iAxisEventId));
    }

    // Add axis event.
    auto optionalError = overwriteAxisEvent(iAxisEventId, vKeyboardTriggers, vGamepadAxis);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }
    return {};
}

std::optional<Error> InputManager::modifyActionEvent(
    unsigned int iActionId,
    std::variant<KeyboardButton, MouseButton, GamepadButton> oldButton,
    std::variant<KeyboardButton, MouseButton, GamepadButton> newButton) {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    // Get the specified action event buttons.
    const auto actions = getAllActionEvents();
    const auto it = actions.find(iActionId);
    if (it == actions.end()) [[unlikely]] {
        return Error(std::format("no action with the ID {} exists", iActionId));
    }

    auto vActionEventTriggerButtons = getActionEventButtons(iActionId);

    // Replace the old button.
    for (auto& button : vActionEventTriggerButtons) {
        if (button == oldButton) {
            button = newButton;
            break;
        }
    }

    // Overwrite event with the new buttons.
    auto optionalError = overwriteActionEvent(iActionId, vActionEventTriggerButtons);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }

    return {};
}

std::optional<Error> InputManager::modifyAxisEvent(
    unsigned int iAxisEventId,
    std::pair<KeyboardButton, KeyboardButton> oldPair,
    std::pair<KeyboardButton, KeyboardButton> newPair) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Get the specified axis event buttons.
    const auto axes = getAllAxisEvents();
    const auto it = axes.find(iAxisEventId);
    if (it == axes.end()) [[unlikely]] {
        return Error(std::format("no axis event with the ID {} exists", iAxisEventId));
    }

    auto [vKeyboardTriggers, vGamepadTriggers] = getAxisEventTriggers(iAxisEventId);

    // Replace old trigger pair.
    bool bOldTriggerExists = false;
    for (auto& pair : vKeyboardTriggers) {
        if (pair == oldPair) {
            bOldTriggerExists = true;
            pair = newPair;
            break;
        }
    }
    if (!bOldTriggerExists) [[unlikely]] {
        return Error("the specified old trigger pair was not found");
    }

    // Overwrite event with new triggers.
    auto optionalError = overwriteAxisEvent(iAxisEventId, vKeyboardTriggers, vGamepadTriggers);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }

    return {};
}

std::optional<Error>
InputManager::modifyAxisEvent(unsigned int iAxisEventId, GamepadAxis oldAxis, GamepadAxis newAxis) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Get the specified axis event buttons.
    const auto axes = getAllAxisEvents();
    const auto it = axes.find(iAxisEventId);
    if (it == axes.end()) [[unlikely]] {
        return Error(std::format("no axis event with the ID {} exists", iAxisEventId));
    }

    auto [vKeyboardTriggers, vGamepadTriggers] = getAxisEventTriggers(iAxisEventId);

    // Replace old trigger.
    bool bOldTriggerExists = false;
    for (auto& axis : vGamepadTriggers) {
        if (axis == oldAxis) {
            bOldTriggerExists = true;
            axis = newAxis;
            break;
        }
    }
    if (!bOldTriggerExists) [[unlikely]] {
        return Error("the specified old trigger pair was not found");
    }

    // Overwrite event with new triggers.
    auto optionalError = overwriteAxisEvent(iAxisEventId, vKeyboardTriggers, vGamepadTriggers);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }

    return {};
}

std::optional<Error> InputManager::saveToFile(std::string_view sFileName) {
    // Get a copy of all registered events.
    const auto allActionEvents = getAllActionEvents();
    const auto allAxisEvents = getAllAxisEvents();

    ConfigManager manager;

    // Save action events.
    for (const auto& [iActionId, vActionKeys] : allActionEvents) {
        std::string sActionKeysText;

        // Put all keys in a string.
        for (const auto& button : vActionKeys) {
            if (std::holds_alternative<KeyboardButton>(button)) {
                sActionKeysText += std::format(
                    "{}{},",
                    keyboardButtonPrefixInFile,
                    std::to_string(static_cast<int>(std::get<KeyboardButton>(button))));
            } else if (std::holds_alternative<MouseButton>(button)) {
                sActionKeysText += std::format(
                    "{}{},",
                    mouseButtonPrefixInFile,
                    std::to_string(static_cast<int>(std::get<MouseButton>(button))));
            } else if (std::holds_alternative<GamepadButton>(button)) {
                sActionKeysText += std::format(
                    "{}{},",
                    gamepadButtonPrefixInFile,
                    std::to_string(static_cast<int>(std::get<GamepadButton>(button))));
            } else [[unlikely]] {
                Error::showErrorAndThrowException("unhandled case");
            }
        }

        sActionKeysText.pop_back(); // pop comma

        // Set value.
        manager.setValue<std::string>(
            sActionEventFileSectionName, std::to_string(iActionId), sActionKeysText);
    }

    // Save axis events.
    for (const auto& [iAxisEventId, axisEvents] : allAxisEvents) {
        const auto& [vKeyboardButtons, vGamepadAxis] = axisEvents;

        {
            // Gather keyboard button pairs in an array.
            std::vector<int> vKeyboardButtonCodes;
            vKeyboardButtonCodes.reserve(vKeyboardButtons.size() * 2);
            for (const auto& pair : vKeyboardButtons) {
                vKeyboardButtonCodes.push_back(static_cast<int>(pair.first));
                vKeyboardButtonCodes.push_back(static_cast<int>(pair.second));
            }
            manager.setValue(
                sKeyboardAxisEventFileSectionName, std::to_string(iAxisEventId), vKeyboardButtonCodes);
        }

        {
            // Same thing with gamepad.
            std::vector<int> vGamepadCodes;
            vGamepadCodes.reserve(vGamepadAxis.size());
            for (const auto& gamepadAxis : vGamepadAxis) {
                vGamepadCodes.push_back(static_cast<int>(gamepadAxis));
            }
            manager.setValue(sGamepadAxisEventFileSectionName, std::to_string(iAxisEventId), vGamepadCodes);
        }
    }

    // Save to disk.
    auto optionalError = manager.saveFile(ConfigCategory::SETTINGS, sFileName);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError.value();
    }

    return {};
}

std::optional<Error>
InputManager::overwriteExistingEventsButtonsFromFile(std::string_view sFileName) { // NOLINT: too complex
    // Load the file.
    ConfigManager manager;
    auto optionalError = manager.loadFile(ConfigCategory::SETTINGS, sFileName);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError;
    }

    // Read sections.
    const auto vSections = manager.getAllSections();
    if (vSections.empty()) [[unlikely]] {
        return Error(std::format("the specified file \"{}\" has no sections", sFileName));
    }

    // Add action events.
    const auto vFileActionEventNames = manager.getAllKeysOfSection(sActionEventFileSectionName);
    if (!vFileActionEventNames.empty()) {
        // Convert action event names to uints.
        std::vector<unsigned int> vFileActionEvents;
        for (const auto& sNumber : vFileActionEventNames) {
            try {
                // First convert as unsigned long and see if it's out of range.
                const auto iUnsignedLong = std::stoul(sNumber);
                if (iUnsignedLong > UINT_MAX) {
                    throw std::out_of_range(sNumber);
                }

                // Add new ID.
                vFileActionEvents.push_back(static_cast<unsigned int>(iUnsignedLong));
            } catch (const std::exception& exception) {
                return Error(
                    std::format("failed to convert \"{}\" to ID, error: {}", sNumber, exception.what()));
            }
        }

        // Get a copy of all registered action events.
        auto currentActionEvents = getAllActionEvents();

        std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

        for (const auto& [iActionId, value] : currentActionEvents) {
            // Look if this registered event exists in the events from file.
            bool bFoundActionInFileActions = false;
            for (const auto& iFileActionId : vFileActionEvents) {
                if (iFileActionId == iActionId) {
                    bFoundActionInFileActions = true;
                    break;
                }
            }
            if (!bFoundActionInFileActions) {
                // We don't have such action event registered so don't import keys.
                continue;
            }

            // Read keys from this action.
            std::string keys =
                manager.getValue<std::string>(sActionEventFileSectionName, std::to_string(iActionId), "");
            if (keys.empty()) {
                continue;
            }

            // Split string and process each key.
            std::vector<std::string> vActionKeys = splitString(keys, ",");
            std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vOutActionKeys;
            for (const auto& key : vActionKeys) {
                if (key[0] == keyboardButtonPrefixInFile) {
                    int iKeyboardKey = -1;
                    auto keyString = key.substr(1);
                    auto [ptr, ec] =
                        std::from_chars(keyString.data(), keyString.data() + keyString.size(), iKeyboardKey);
                    if (ec != std::errc()) {
                        return Error(std::format(
                            "failed to convert '{}' to keyboard key code (error code: {})",
                            keyString,
                            static_cast<int>(ec)));
                    }

                    vOutActionKeys.push_back(static_cast<KeyboardButton>(iKeyboardKey));
                } else if (key[0] == mouseButtonPrefixInFile) {
                    int iMouseButton = -1;
                    auto keyString = key.substr(1);
                    auto [ptr, ec] =
                        std::from_chars(keyString.data(), keyString.data() + keyString.size(), iMouseButton);
                    if (ec != std::errc()) {
                        return Error(std::format(
                            "failed to convert '{}' to mouse button code (error code: {})",
                            keyString,
                            static_cast<int>(ec)));
                    }

                    vOutActionKeys.push_back(static_cast<MouseButton>(iMouseButton));
                } else if (key[0] == gamepadButtonPrefixInFile) {
                    int iGamepadButton = -1;
                    auto keyString = key.substr(1);
                    auto [ptr, ec] = std::from_chars(
                        keyString.data(), keyString.data() + keyString.size(), iGamepadButton);
                    if (ec != std::errc()) {
                        return Error(std::format(
                            "failed to convert '{}' to gamepad button code (error code: {})",
                            keyString,
                            static_cast<int>(ec)));
                    }

                    vOutActionKeys.push_back(static_cast<GamepadButton>(iGamepadButton));
                } else [[unlikely]] {
                    Error::showErrorAndThrowException("unhandled case");
                }
            }

            // Add keys (replace old ones).
            optionalError = overwriteActionEvent(iActionId, vOutActionKeys);
            if (optionalError.has_value()) [[unlikely]] {
                optionalError->addCurrentLocationToErrorStack();
                return std::move(optionalError.value());
            }
        }
    }

    // Add keyboard axis events.
    auto vFileKeyboardAxisEventNames = manager.getAllKeysOfSection(sKeyboardAxisEventFileSectionName);
    const auto vFileGamepadAxisEventNames = manager.getAllKeysOfSection(sGamepadAxisEventFileSectionName);
    if (!vFileKeyboardAxisEventNames.empty() || !vFileGamepadAxisEventNames.empty()) {
        // Group event names.
        auto vFileAxisEventNames = std::move(vFileKeyboardAxisEventNames);
        vFileAxisEventNames.insert(
            vFileAxisEventNames.end(), vFileGamepadAxisEventNames.begin(), vFileGamepadAxisEventNames.end());

        // Convert event ID (string) to unsigned int.
        std::unordered_set<unsigned int> fileAxisEvents;
        for (const auto& sNumber : vFileAxisEventNames) {
            try {
                // First convert as unsigned long and see if it's out of range.
                const auto iUnsignedLong = std::stoul(sNumber);
                if (iUnsignedLong > UINT_MAX) [[unlikely]] {
                    throw std::out_of_range(sNumber);
                }

                // Add new ID.
                fileAxisEvents.insert(static_cast<unsigned int>(iUnsignedLong));
            } catch (const std::exception& exception) {
                return Error(
                    std::format("failed to convert \"{}\" to ID, error: {}", sNumber, exception.what()));
            }
        }

        std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

        // Create a copy of all axis event IDs because we will modify axis event states in the loop below.
        std::vector<unsigned int> vCurrentAxisEventIds;
        vCurrentAxisEventIds.reserve(axisEventStates.size());
        for (const auto& [iAxisEventId, state] : axisEventStates) {
            vCurrentAxisEventIds.push_back(iAxisEventId);
        }

        for (const auto& iAxisEventId : vCurrentAxisEventIds) {
            // Look for this event ID in the file.
            const auto foundEventIt = fileAxisEvents.find(iAxisEventId);
            if (foundEventIt == fileAxisEvents.end()) {
                // We don't have such axis event registered so don't import the triggers.
                continue;
            }

            // Read triggers from file.
            auto vKeyboardTriggersFromFile = manager.getValue<std::vector<int>>(
                sKeyboardAxisEventFileSectionName, std::to_string(iAxisEventId), {});
            auto vGamepadTriggersFromFile = manager.getValue<std::vector<int>>(
                sGamepadAxisEventFileSectionName, std::to_string(iAxisEventId), {});
            if (vKeyboardTriggersFromFile.empty() && vGamepadTriggersFromFile.empty()) {
                continue;
            }

            // Make sure keyboard triggers array has valid size.
            if (vKeyboardTriggersFromFile.size() % 2 != 0) [[unlikely]] {
                return Error(std::format(
                    "keyboard axis event buttons don't seem to store pairs of buttons, unexpected array "
                    "size: {}",
                    vKeyboardTriggersFromFile.size()));
            }

            std::vector<std::pair<KeyboardButton, KeyboardButton>> vNewKeyboardTriggers;
            vNewKeyboardTriggers.reserve(vKeyboardTriggersFromFile.size());
            std::vector<GamepadAxis> vNewGamepadTriggers;
            vNewGamepadTriggers.reserve(vGamepadTriggersFromFile.size());

            // Cast int to enum.
            for (size_t i = 0; i < vKeyboardTriggersFromFile.size(); i += 2) {
                vNewKeyboardTriggers.push_back(
                    {static_cast<KeyboardButton>(vKeyboardTriggersFromFile[i]),
                     static_cast<KeyboardButton>(vKeyboardTriggersFromFile[i + 1])});
            }
            for (const auto& iGamepadAxis : vGamepadTriggersFromFile) {
                vNewGamepadTriggers.push_back(static_cast<GamepadAxis>(iGamepadAxis));
            }

            // Replace old triggers.
            optionalError = overwriteAxisEvent(iAxisEventId, vNewKeyboardTriggers, vNewGamepadTriggers);
            if (optionalError.has_value()) [[unlikely]] {
                optionalError->addCurrentLocationToErrorStack();
                return std::move(optionalError.value());
            }
        }
    }

    return {};
}

void InputManager::setGamepadDeadzone(float deadzone) { gamepadDeadzone = deadzone; }

std::pair<std::vector<unsigned int>, std::vector<unsigned int>>
InputManager::isButtonUsed(const std::variant<KeyboardButton, MouseButton, GamepadButton>& button) {
    std::scoped_lock guard(mtxActionEvents, mtxAxisEvents);

    std::vector<unsigned int> vUsedActionEvents;
    std::vector<unsigned int> vUsedAxisEvents;

    // Check action events.
    const auto foundActionEventIt = buttonToActionEvents.find(button);
    if (foundActionEventIt != buttonToActionEvents.end()) {
        vUsedActionEvents = foundActionEventIt->second;
    }

    // Check axis events.
    if (std::holds_alternative<KeyboardButton>(button)) {
        const auto keyboardKey = std::get<KeyboardButton>(button);

        const auto foundAxisEventIt = keyboardButtonToAxisEvents.find(keyboardKey);
        if (foundAxisEventIt != keyboardButtonToAxisEvents.end()) {
            for (const auto& [iAxisId, value] : foundAxisEventIt->second) {
                vUsedAxisEvents.push_back(iAxisId);
            }
        }
    }

    return std::make_pair(vUsedActionEvents, vUsedAxisEvents);
}

std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>
InputManager::getActionEventButtons(unsigned int iActionId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vButtons;

    // Look if this action exists, get all keys.
    for (const auto& [key, vEvents] : buttonToActionEvents) {
        for (const auto& iEventId : vEvents) {
            if (iEventId == iActionId) {
                vButtons.push_back(key);
            }
        }
    }

    return vButtons;
}

std::pair<std::vector<std::pair<KeyboardButton, KeyboardButton>>, std::vector<GamepadAxis>>
InputManager::getAxisEventTriggers(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Get event state.
    const auto eventIt = axisEventStates.find(iAxisEventId);
    if (eventIt == axisEventStates.end()) [[unlikely]] {
        return {};
    }

    // Collect triggers from state.
    std::vector<std::pair<KeyboardButton, KeyboardButton>> vKeyboardTriggers;
    for (const auto& state : eventIt->second.vKeyboardTriggers) {
        vKeyboardTriggers.push_back({state.positiveTrigger, state.negativeTrigger});
    }

    // Collect triggers from state.
    std::vector<GamepadAxis> vGamepadTriggers;
    for (const auto& state : eventIt->second.vGamepadTriggers) {
        vGamepadTriggers.push_back(state.trigger);
    }

    return {vKeyboardTriggers, vGamepadTriggers};
}

float InputManager::getCurrentAxisEventState(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Find the specified axis event by ID.
    const auto stateIt = axisEventStates.find(iAxisEventId);
    if (stateIt == axisEventStates.end()) {
        return 0.0F;
    }

    return stateIt->second.state;
}

bool InputManager::removeActionEvent(unsigned int iActionId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    bool bFound = false;

    // Look if this action exists and remove all entries.
    for (auto it = buttonToActionEvents.begin(); it != buttonToActionEvents.end();) {
        // Remove all events with the specified ID.
        size_t iErasedItemCount = 0;
        for (auto eventIt = it->second.begin(); eventIt != it->second.end();) {
            if (*eventIt == iActionId) {
                eventIt = it->second.erase(eventIt);
                iErasedItemCount += 1;
            } else {
                ++eventIt;
            }
        }

        // See if we removed something.
        if (iErasedItemCount > 0) {
            // Only change to `true`, don't do: `bFound = iSizeAfter < iSizeBefore` as this might
            // change `bFound` from `true` to `false`.
            bFound = true;
        }

        // If there are no events associated with this keyboard button just remove this entry from the map.
        if (it->second.empty()) {
            it = buttonToActionEvents.erase(it);
        } else {
            ++it;
        }
    }

    // Remove action state.
    const auto it = actionEventStates.find(iActionId);
    if (it != actionEventStates.end()) {
        actionEventStates.erase(it);
    }

    return !bFound;
}

bool InputManager::removeAxisEvent(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    bool bFound = false;

    // Look for keyboard buttons that use this event.
    for (auto it = keyboardButtonToAxisEvents.begin(); it != keyboardButtonToAxisEvents.end();) {
        // See if this button is using the event we want to remove.
        size_t iErasedItemCount = 0;
        for (auto eventIt = it->second.begin(); eventIt != it->second.end();) {
            if (eventIt->first == iAxisEventId) {
                // Erase this button-event association.
                eventIt = it->second.erase(eventIt);
                iErasedItemCount += 1;
            } else {
                ++eventIt;
            }
        }

        // See if we removed something.
        if (iErasedItemCount > 0) {
            // Only change to `true`, don't do: `bFound = iSizeAfter < iSizeBefore` as this might
            // change `bFound` from `true` to `false`.
            bFound = true;
        }

        // Check if we need to remove this key from map.
        if (it->second.empty()) {
            // Remove key for this event as there are no events registered to this key.
            it = keyboardButtonToAxisEvents.erase(it);
        } else {
            ++it;
        }
    }

    // Look for gamepad axes that use this event.
    for (auto it = gamepadAxisToAxisEvents.begin(); it != gamepadAxisToAxisEvents.end();) {
        // See if this button is using the event we want to remove.
        size_t iErasedItemCount = 0;
        for (auto eventIdIt = it->second.begin(); eventIdIt != it->second.end();) {
            if ((*eventIdIt) == iAxisEventId) {
                // Erase this axis-event association.
                eventIdIt = it->second.erase(eventIdIt);
                iErasedItemCount += 1;
            } else {
                ++eventIdIt;
            }
        }

        // See if we removed something.
        if (iErasedItemCount > 0) {
            // Only change to `true`, don't do: `bFound = iSizeAfter < iSizeBefore` as this might
            // change `bFound` from `true` to `false`.
            bFound = true;
        }

        // If there are no events associated with this gamepad axis just remove this entry from the map.
        if (it->second.empty()) {
            it = gamepadAxisToAxisEvents.erase(it);
        } else {
            ++it;
        }
    }

    // Remove axis event state.
    const auto it = axisEventStates.find(iAxisEventId);
    if (it != axisEventStates.end()) {
        axisEventStates.erase(it);
    }

    return !bFound;
}

std::unordered_map<unsigned int, std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>>
InputManager::getAllActionEvents() {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    std::unordered_map<unsigned int, std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>>
        actions;

    for (const auto& [iActionEventId, state] : actionEventStates) {
        actions[iActionEventId] = getActionEventButtons(iActionEventId);
    }

    return actions;
}

std::unordered_map<
    unsigned int,
    std::pair<std::vector<std::pair<KeyboardButton, KeyboardButton>>, std::vector<GamepadAxis>>>
InputManager::getAllAxisEvents() {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    std::unordered_map<
        unsigned int,
        std::pair<std::vector<std::pair<KeyboardButton, KeyboardButton>>, std::vector<GamepadAxis>>>
        triggers;

    for (const auto& [iAxisEventId, state] : axisEventStates) {
        triggers[iAxisEventId] = getAxisEventTriggers(iAxisEventId);
    }

    return triggers;
}

float InputManager::getGamepadDeadzone() const { return gamepadDeadzone; }

std::vector<std::string>
InputManager::splitString(const std::string& sStringToSplit, const std::string& sDelimiter) {
    std::vector<std::string> vResult;
    size_t last = 0;
    size_t next = 0;

    while ((next = sStringToSplit.find(sDelimiter, last)) != std::string::npos) {
        vResult.push_back(sStringToSplit.substr(last, next - last));
        last = next + 1;
    }

    vResult.push_back(sStringToSplit.substr(last));
    return vResult;
}

std::optional<Error> InputManager::overwriteActionEvent(
    unsigned int iActionId,
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vButtons) {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    // Remove all buttons associated with this action event if it exists.
    removeActionEvent(iActionId);

    std::vector<ActionEventTriggerButtonState> vTriggerButtonStates;
    for (const auto& button : vButtons) {
        // See if this button is used in some other action event.
        const auto it = buttonToActionEvents.find(button);
        if (it == buttonToActionEvents.end()) {
            // Add a new pair of "button" to "events".
            buttonToActionEvents[button] = {iActionId};
        } else {
            // Add this event to the array of events that use this button.
            it->second.push_back(iActionId);
        }

        vTriggerButtonStates.push_back(ActionEventTriggerButtonState(button));
    }

    // Add/overwrite state.
    actionEventStates[iActionId] =
        ActionEventState{.vTriggerButtonStates = std::move(vTriggerButtonStates), .bEventState = false};

    return {};
}

std::optional<Error> InputManager::overwriteAxisEvent(
    unsigned int iAxisEventId,
    const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vKeyboardTriggers,
    const std::vector<GamepadAxis>& vGamepadTriggers) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Remove all axis events with this name if exist.
    removeAxisEvent(iAxisEventId);

    // Add new keyboard triggers.
    std::vector<AxisEventTriggerButtonsState> vKeyboardTriggerStates;
    for (const auto& [positiveTrigger, negativeTrigger] : vKeyboardTriggers) {
        // Create a new button-event association for positive trigger.
        auto it = keyboardButtonToAxisEvents.find(positiveTrigger);
        if (it == keyboardButtonToAxisEvents.end()) {
            keyboardButtonToAxisEvents[positiveTrigger] =
                std::vector<std::pair<unsigned int, bool>>{{iAxisEventId, true}};
        } else {
            it->second.push_back(std::pair<unsigned int, bool>{iAxisEventId, true});
        }

        // Create a new button-event association for negative trigger.
        it = keyboardButtonToAxisEvents.find(negativeTrigger);
        if (it == keyboardButtonToAxisEvents.end()) {
            keyboardButtonToAxisEvents[negativeTrigger] =
                std::vector<std::pair<unsigned int, bool>>{{iAxisEventId, false}};
        } else {
            it->second.push_back(std::pair<unsigned int, bool>(iAxisEventId, false));
        }

        // Add new triggers to states.
        vKeyboardTriggerStates.push_back(AxisEventTriggerButtonsState(positiveTrigger, negativeTrigger));
    }

    // Add new gamepad triggers.
    std::vector<AxisEventTriggerAxisState> vGamepadTriggerStates;
    for (const auto& gamepadAxis : vGamepadTriggers) {
        // Create a new axis-event association.
        auto it = gamepadAxisToAxisEvents.find(gamepadAxis);
        if (it == gamepadAxisToAxisEvents.end()) {
            gamepadAxisToAxisEvents[gamepadAxis] = {iAxisEventId};
        } else {
            it->second.push_back(iAxisEventId);
        }

        // Add new triggers to states.
        vGamepadTriggerStates.push_back(AxisEventTriggerAxisState(gamepadAxis));
    }

    // Add/overwrite event state.
    axisEventStates[iAxisEventId] = AxisEventState{
        .vKeyboardTriggers = std::move(vKeyboardTriggerStates),
        .vGamepadTriggers = std::move(vGamepadTriggerStates),
        .state = 0.0F};

    return {};
}
