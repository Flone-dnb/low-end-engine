#include "game/GameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/GameManager.h"

GameInstance::GameInstance(Window* pWindow) : pWindow(pWindow) {}

void GameInstance::createWorld(const std::function<void(Node*)>& onCreated, bool bDestroyOldWorlds) {
    pWindow->getGameManager()->createWorld(onCreated, bDestroyOldWorlds);
}

void GameInstance::loadNodeTreeAsWorld(
    const std::filesystem::path& pathToNodeTreeFile, const std::function<void(Node*)>& onLoaded) {
    pWindow->getGameManager()->loadNodeTreeAsWorld(pathToNodeTreeFile, onLoaded);
}

void GameInstance::addTaskToThreadPool(const std::function<void()>& task) const {
    pWindow->getGameManager()->addTaskToThreadPool(task);
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, bool)>>>&
GameInstance::getActionEventBindings() {
    return mtxBindedActionEvents;
}

std::pair<
    std::recursive_mutex,
    std::unordered_map<unsigned int, std::function<void(KeyboardModifiers, float)>>>&
GameInstance::getAxisEventBindings() {
    return mtxBindedAxisEvents;
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

size_t GameInstance::getTotalSpawnedNodeCount() const {
    return pWindow->getGameManager()->getTotalSpawnedNodeCount();
}

size_t GameInstance::getCalledEveryFrameNodeCount() const {
    return pWindow->getGameManager()->getCalledEveryFrameNodeCount();
}

size_t GameInstance::getReceivingInputNodeCount() const {
    return pWindow->getGameManager()->getReceivingInputNodeCount();
}

Window* GameInstance::getWindow() const { return pWindow; }

Renderer* GameInstance::getRenderer() const { return pWindow->getGameManager()->getRenderer(); }

InputManager* GameInstance::getInputManager() const { return pWindow->getGameManager()->getInputManager(); }

bool GameInstance::isGamepadConnected() const { return pWindow->isGamepadConnected(); }
