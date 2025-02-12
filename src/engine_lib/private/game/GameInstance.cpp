#include "game/GameInstance.h"

GameInstance::GameInstance(Window* pWindow) : pWindow(pWindow) {}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>*
GameInstance::getActionEventBindings() {
    return &mtxBindedActionEvents;
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>*
GameInstance::getAxisEventBindings() {
    return &mtxBindedAxisEvents;
}

void GameInstance::onInputActionEvent(
    unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxBindedActionEvents.first);

    // Find this event in the registered events.
    const auto it = mtxBindedActionEvents.second.find(iActionId);
    if (it == mtxBindedActionEvents.second.end()) {
        return;
    }

    // Call user logic.
    it->second(modifiers, bIsPressedDown);
}

void GameInstance::onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input) {
    std::scoped_lock guard(mtxBindedAxisEvents.first);

    // Find this event in the registered events.
    const auto it = mtxBindedAxisEvents.second.find(iAxisEventId);
    if (it == mtxBindedAxisEvents.second.end()) {
        return;
    }

    // Call user logic.
    it->second(modifiers, input);
}

Window* GameInstance::getWindow() const { return pWindow; }
