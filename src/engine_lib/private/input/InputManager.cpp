#include "input/InputManager.h"

// Standard.
#include <charconv>
#include <string>
#include <format>
#include <vector>

// Custom.
#include "io/Logger.h"
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
    unsigned int iAxisEventId, const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vTriggerPairs) {
    // Make sure there is an least one button specified to trigger this event.
    if (vTriggerPairs.empty()) [[unlikely]] {
        return Error("vAxis is empty");
    }

    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Check if an axis event with this name already exists.
    const auto vRegisteredAxisEvents = getAllAxisEvents();
    const auto action = vRegisteredAxisEvents.find(iAxisEventId);
    if (action != vRegisteredAxisEvents.end()) [[unlikely]] {
        return Error(std::format("an axis event with the ID {} already exists", iAxisEventId));
    }

    // Add axis event.
    auto optionalError = overwriteAxisEvent(iAxisEventId, vTriggerPairs);
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

    auto vAxisKeys = getAxisEventTriggers(iAxisEventId);

    // Replace old trigger pair.
    bool bOldTriggerExists = false;
    for (auto& pair : vAxisKeys) {
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
    auto optionalError = overwriteAxisEvent(iAxisEventId, vAxisKeys);
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

    // Action events.
    for (const auto& [iActionId, vActionKeys] : allActionEvents) {
        std::string sActionKeysText;

        // Put all keys in a string.
        for (const auto& key : vActionKeys) {
            if (std::holds_alternative<KeyboardButton>(key)) {
                sActionKeysText +=
                    std::format("k{},", std::to_string(static_cast<int>(std::get<KeyboardButton>(key))));
            } else {
                sActionKeysText +=
                    std::format("m{},", std::to_string(static_cast<int>(std::get<MouseButton>(key))));
            }
        }

        sActionKeysText.pop_back(); // pop comma

        // Set value.
        manager.setValue<std::string>(
            sActionEventFileSectionName, std::to_string(iActionId), sActionKeysText);
    }

    // Axis events.
    for (const auto& [iAxisEventId, vAxisKeys] : allAxisEvents) {
        std::string sAxisKeysText;

        // Put all keys in a string.
        for (const auto& pair : vAxisKeys) {
            sAxisKeysText += std::format(
                "{}-{},",
                std::to_string(static_cast<int>(pair.first)),
                std::to_string(static_cast<int>(pair.second)));
        }

        sAxisKeysText.pop_back(); // pop comma

        manager.setValue<std::string>(sAxisEventFileSectionName, std::to_string(iAxisEventId), sAxisKeysText);
    }

    auto optional = manager.saveFile(ConfigCategory::SETTINGS, sFileName);
    if (optional.has_value()) {
        optional->addCurrentLocationToErrorStack();
        return std::move(optional.value());
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

    // Check that we only have 1 or 2 sections.
    if (vSections.size() > 2) [[unlikely]] {
        return Error(std::format(
            "the specified file \"{}\" has {} sections, while expected only 1 or 2 sections",
            sFileName,
            vSections.size()));
    }

    // Check TOML section names.
    bool bHasActionEventsSection = false;
    bool bHasAxisEventsSection = false;
    for (const auto& sSectionName : vSections) {
        if (sSectionName == sActionEventFileSectionName) {
            bHasActionEventsSection = true;
        } else if (sSectionName == sAxisEventFileSectionName) {
            bHasAxisEventsSection = true;
        } else {
            return Error(std::format("section \"{}\" has unexpected name", sSectionName));
        }
    }

    // Add action events.
    if (bHasActionEventsSection) {
        auto variant = manager.getAllKeysOfSection(sActionEventFileSectionName);
        if (std::holds_alternative<Error>(variant)) [[unlikely]] {
            auto error = std::get<Error>(std::move(variant));
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto vFileActionEventNames = std::get<std::vector<std::string>>(std::move(variant));

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
                if (key[0] == 'k') {
                    // KeyboardKey.
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
                } else if (key[0] == 'm') {
                    // MouseButton.
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

    // Add axis events.
    if (bHasAxisEventsSection) {
        auto variant = manager.getAllKeysOfSection(sAxisEventFileSectionName);
        if (std::holds_alternative<Error>(variant)) [[unlikely]] {
            auto error = std::get<Error>(std::move(variant));
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto vFileAxisEventNames = std::get<std::vector<std::string>>(std::move(variant));

        // Convert axis event names to uints.
        std::vector<unsigned int> vFileAxisEvents;
        for (const auto& sNumber : vFileAxisEventNames) {
            try {
                // First convert as unsigned long and see if it's out of range.
                const auto iUnsignedLong = std::stoul(sNumber);
                if (iUnsignedLong > UINT_MAX) {
                    throw std::out_of_range(sNumber);
                }

                // Add new ID.
                vFileAxisEvents.push_back(static_cast<unsigned int>(iUnsignedLong));
            } catch (const std::exception& exception) {
                return Error(
                    std::format("failed to convert \"{}\" to ID, error: {}", sNumber, exception.what()));
            }
        }

        // Get a copy of all registered axis events.
        const auto currentAxisEvents = getAllAxisEvents();

        std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

        for (const auto& [iAxisEventId, value] : currentAxisEvents) {
            // Look for this axis event in file.
            bool bFoundAxisEventInFileActions = false;
            for (const auto& iFileAxisEventId : vFileAxisEvents) {
                if (iFileAxisEventId == iAxisEventId) {
                    bFoundAxisEventInFileActions = true;
                    break;
                }
            }
            if (!bFoundAxisEventInFileActions) {
                // We don't have such axis event registered so don't import keys.
                continue;
            }

            // Read keys from this axis.
            std::string keys =
                manager.getValue<std::string>(sAxisEventFileSectionName, std::to_string(iAxisEventId), "");
            if (keys.empty()) {
                continue;
            }

            // Split string and process each key.
            std::vector<std::string> vAxisKeys = splitString(keys, ",");
            std::vector<std::pair<KeyboardButton, KeyboardButton>> vOutAxisKeys;
            for (const auto& key : vAxisKeys) {
                std::vector<std::string> vTriggerPairs = splitString(key, "-");

                if (vTriggerPairs.size() != 2) {
                    return Error(std::format("axis entry '{}' does not have 2 keys", key));
                }

                // Convert the first one.
                int iKeyboardPositiveTrigger = -1;
                auto [ptr1, ec1] = std::from_chars(
                    vTriggerPairs[0].data(),
                    vTriggerPairs[0].data() + vTriggerPairs[0].size(),
                    iKeyboardPositiveTrigger);
                if (ec1 != std::errc()) [[unlikely]] {
                    return Error(std::format(
                        "failed to convert the first key of axis entry '{}' "
                        "to keyboard key code (error code: {})",
                        key,
                        static_cast<int>(ec1)));
                }

                // Convert the second one.
                int iKeyboardNegativeTrigger = -1;
                auto [ptr2, ec2] = std::from_chars(
                    vTriggerPairs[1].data(),
                    vTriggerPairs[1].data() + vTriggerPairs[1].size(),
                    iKeyboardNegativeTrigger);
                if (ec2 != std::errc()) [[unlikely]] {
                    return Error(std::format(
                        "failed to convert the second key of axis entry '{}' "
                        "to keyboard key code (error code: {})",
                        key,
                        static_cast<int>(ec2)));
                }

                vOutAxisKeys.push_back(std::make_pair<KeyboardButton, KeyboardButton>(
                    static_cast<KeyboardButton>(iKeyboardPositiveTrigger),
                    static_cast<KeyboardButton>(iKeyboardNegativeTrigger)));
            }

            // Add keys (replace old ones).
            optionalError = overwriteAxisEvent(iAxisEventId, vOutAxisKeys);
            if (optionalError.has_value()) [[unlikely]] {
                optionalError->addCurrentLocationToErrorStack();
                return std::move(optionalError.value());
            }
        }
    }

    return {};
}

std::pair<std::vector<unsigned int>, std::vector<unsigned int>>
InputManager::isButtonUsed(const std::variant<KeyboardButton, MouseButton, GamepadButton>& button) {
    std::scoped_lock guard(mtxActionEvents, mtxAxisEvents);

    std::vector<unsigned int> vUsedActionEvents;
    std::vector<unsigned int> vUsedAxisEvents;

    // Check action events.
    const auto foundActionEventIt = actionEvents.find(button);
    if (foundActionEventIt != actionEvents.end()) {
        vUsedActionEvents = foundActionEventIt->second;
    }

    // Check axis events.
    if (std::holds_alternative<KeyboardButton>(button)) {
        const auto keyboardKey = std::get<KeyboardButton>(button);

        const auto foundAxisEventIt = axisEvents.find(keyboardKey);
        if (foundAxisEventIt != axisEvents.end()) {
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

    std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vKeys;

    // Look if this action exists, get all keys.
    for (const auto& [key, vEvents] : actionEvents) {
        for (const auto& iEventId : vEvents) {
            if (iEventId == iActionId) {
                vKeys.push_back(key);
            }
        }
    }

    return vKeys;
}

std::vector<std::pair<KeyboardButton, KeyboardButton>>
InputManager::getAxisEventTriggers(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Find only positive triggers.
    std::vector<KeyboardButton> vPositiveTriggers;
    for (const auto& [trigger, vEvents] : axisEvents) {
        for (const auto& [iRegisteredAxisEventId, bTriggersPositiveInput] : vEvents) {
            if (!bTriggersPositiveInput) {
                // We are not interested in this now.
                continue;
            }

            if (iRegisteredAxisEventId == iAxisEventId) {
                vPositiveTriggers.push_back(trigger);
            }
        }
    }

    if (vPositiveTriggers.empty()) {
        return {};
    }

    // Add correct negative triggers to each positive trigger using info from states.
    std::vector<KeyboardButton> vNegativeTriggers;
    const auto vAxisEventStates = axisState[iAxisEventId].first;

    for (const auto& positiveTrigger : vPositiveTriggers) {
        // Find a state that has this positive key.
        std::optional<KeyboardButton> foundNegativeTrigger;
        for (const auto& state : vAxisEventStates) {
            if (state.positiveTrigger == positiveTrigger) {
                foundNegativeTrigger = state.negativeTrigger;
                break;
            }
        }
        if (!foundNegativeTrigger.has_value()) [[unlikely]] {
            Logger::get().error(std::format(
                "can't find negative trigger for positive trigger in axis event with ID {}", iAxisEventId));
            return {};
        }

        // Add found negative trigger from the state.
        vNegativeTriggers.push_back(foundNegativeTrigger.value());
    }

    // Check sizes.
    if (vPositiveTriggers.size() != vNegativeTriggers.size()) [[unlikely]] {
        Logger::get().error(std::format(
            "not equal size of positive and negative triggers, found {} positive trigger(s) and {} "
            "negative trigger(s) for axis event with ID {}",
            vPositiveTriggers.size(),
            vNegativeTriggers.size(),
            iAxisEventId));
        return {};
    }

    // Fill axis.
    std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxisKeys;
    vAxisKeys.reserve(vPositiveTriggers.size());
    for (size_t i = 0; i < vPositiveTriggers.size(); i++) {
        vAxisKeys.push_back(std::make_pair(vPositiveTriggers[i], vNegativeTriggers[i]));
    }

    return vAxisKeys;
}

float InputManager::getCurrentAxisEventState(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Find the specified axis event by ID.
    const auto stateIt = axisState.find(iAxisEventId);
    if (stateIt == axisState.end()) {
        return 0.0F;
    }

    return static_cast<float>(stateIt->second.second);
}

bool InputManager::removeActionEvent(unsigned int iActionId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    bool bFound = false;

    // Look if this action exists and remove all entries.
    for (auto it = actionEvents.begin(); it != actionEvents.end();) {
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

        // Check if we need to remove this key from map.
        if (it->second.empty()) {
            // Remove key for this event as there are no events registered to this key.
            it = actionEvents.erase(it);
        } else {
            ++it;
        }
    }

    // Remove action state.
    const auto it = actionEventsTriggerButtonsState.find(iActionId);
    if (it != actionEventsTriggerButtonsState.end()) {
        actionEventsTriggerButtonsState.erase(it);
    }

    return !bFound;
}

bool InputManager::removeAxisEvent(unsigned int iAxisEventId) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    bool bFound = false;

    // Look for positive and negative triggers.
    for (auto it = axisEvents.begin(); it != axisEvents.end();) {
        // Remove all events with the specified ID.
        size_t iErasedItemCount = 0;
        for (auto eventIt = it->second.begin(); eventIt != it->second.end();) {
            if (eventIt->first == iAxisEventId) {
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
            it = axisEvents.erase(it);
        } else {
            ++it;
        }
    }

    // Remove axis state.
    const auto it = axisState.find(iAxisEventId);
    if (it != axisState.end()) {
        axisState.erase(it);
    }

    return !bFound;
}

std::unordered_map<unsigned int, std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>>
InputManager::getAllActionEvents() {
    std::scoped_lock<std::recursive_mutex> guard(mtxActionEvents);

    std::unordered_map<unsigned int, std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>>
        actions;

    // Get all action names first.
    for (const auto& [key, sActionName] : actionEvents) {
        for (const auto& sName : sActionName) {
            if (!actions.contains(sName)) {
                actions[sName] = {};
            }
        }
    }

    // Fill keys for those actions.
    for (const auto& actionPair : actionEvents) {
        for (const auto& actionName : actionPair.second) {
            actions[actionName].push_back(actionPair.first);
        }
    }

    return actions;
}

std::unordered_map<unsigned int, std::vector<std::pair<KeyboardButton, KeyboardButton>>>
InputManager::getAllAxisEvents() {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    std::unordered_map<unsigned int, std::vector<std::pair<KeyboardButton, KeyboardButton>>> axes;

    for (const auto& [key, keyAxisNames] : axisEvents) {
        for (const auto& [iAxisId, value] : keyAxisNames) {
            if (axes.contains(iAxisId)) {
                continue;
            }

            // Get keys of this axis event.
            auto vKeys = getAxisEventTriggers(iAxisId);
            if (!vKeys.empty()) {
                axes[iAxisId] = std::move(vKeys);
            } else {
                axes[iAxisId] = {};
                Logger::get().error(std::format("no axis event found by ID {}", iAxisId));
            }
        }
    }

    return axes;
}

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

    // Add buttons for actions.
    std::vector<ActionEventTriggerButtonState> vActionState;
    for (const auto& button : vButtons) {
        // Find action events that use this key.
        auto it = actionEvents.find(button);
        if (it == actionEvents.end()) {
            actionEvents[button] = {iActionId};
        } else {
            it->second.push_back(iActionId);
        }

        vActionState.push_back(ActionEventTriggerButtonState(button));
    }

    // Add/overwrite state.
    actionEventsTriggerButtonsState[iActionId] =
        std::make_pair<std::vector<ActionEventTriggerButtonState>, bool>(std::move(vActionState), false);

    return {};
}

std::optional<Error> InputManager::overwriteAxisEvent(
    unsigned int iAxisEventId, const std::vector<std::pair<KeyboardButton, KeyboardButton>>& vTriggerPairs) {
    std::scoped_lock<std::recursive_mutex> guard(mtxAxisEvents);

    // Remove all axis events with this name if exist.
    removeAxisEvent(iAxisEventId);

    // Add new triggers.
    std::vector<AxisEventTriggerButtonsState> vAxisState;
    for (const auto& [positiveTrigger, negativeTrigger] : vTriggerPairs) {
        // Find axis events that use this positive trigger.
        auto it = axisEvents.find(positiveTrigger);
        if (it == axisEvents.end()) {
            axisEvents[positiveTrigger] = std::vector<std::pair<unsigned int, bool>>{{iAxisEventId, true}};
        } else {
            it->second.push_back(std::pair<unsigned int, bool>{iAxisEventId, true});
        }

        // Find axis events that use this negative trigger.
        it = axisEvents.find(negativeTrigger);
        if (it == axisEvents.end()) {
            axisEvents[negativeTrigger] = std::vector<std::pair<unsigned int, bool>>{{iAxisEventId, false}};
        } else {
            it->second.push_back(std::pair<unsigned int, bool>(iAxisEventId, false));
        }

        // Add new triggers to states.
        vAxisState.push_back(AxisEventTriggerButtonsState(positiveTrigger, negativeTrigger));
    }

    // Add/overwrite event state.
    axisState[iAxisEventId] =
        std::make_pair<std::vector<AxisEventTriggerButtonsState>, int>(std::move(vAxisState), 0);

    return {};
}
