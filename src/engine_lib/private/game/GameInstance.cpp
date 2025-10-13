#include "game/GameInstance.h"

// Custom.
#include "game/Window.h"
#include "game/GameManager.h"

GameInstance::GameInstance(Window* pWindow) : pWindow(pWindow) {}

void GameInstance::createWorld(
    const std::function<void(Node*)>& onCreated, bool bDestroyOldWorlds, const std::string& sName) {
    pWindow->getGameManager()->createWorld(onCreated, bDestroyOldWorlds, sName);
}

void GameInstance::loadNodeTreeAsWorld(
    const std::filesystem::path& pathToNodeTreeFile,
    const std::function<void(Node*)>& onLoaded,
    bool bDestroyOldWorlds,
    const std::string& sName) {
    pWindow->getGameManager()->loadNodeTreeAsWorld(pathToNodeTreeFile, onLoaded, bDestroyOldWorlds, sName);
}

void GameInstance::addTaskToThreadPool(const std::function<void()>& task) const {
    pWindow->getGameManager()->addTaskToThreadPool(task);
}

void GameInstance::destroyWorld(World* pWorldToDestroy, const std::function<void()>& onAfterDestroyed) const {
    pWindow->getGameManager()->destroyWorld(pWorldToDestroy, onAfterDestroyed);
}

void GameInstance::onInputActionEvent(
    unsigned int iActionId, KeyboardModifiers modifiers, bool bIsPressedDown) {
    // Find this event in the registered events.
    const auto it = boundActionEvents.find(iActionId);
    if (it == boundActionEvents.end()) {
        return;
    }

    // Call user logic.
    if (bIsPressedDown && it->second.onPressed != nullptr) {
        it->second.onPressed(modifiers);
    } else if (it->second.onReleased != nullptr) {
        it->second.onReleased(modifiers);
    }
}

void GameInstance::onInputAxisEvent(unsigned int iAxisEventId, KeyboardModifiers modifiers, float input) {
    // Find this event in the registered events.
    const auto it = boundAxisEvents.find(iAxisEventId);
    if (it == boundAxisEvents.end()) {
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

ScriptManager& GameInstance::getScriptManager() const {
    return pWindow->getGameManager()->getScriptManager();
}

bool GameInstance::isGamepadConnected() const { return pWindow->isGamepadConnected(); }
