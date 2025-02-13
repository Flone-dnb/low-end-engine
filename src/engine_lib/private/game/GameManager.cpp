#include "GameManager.h"

// Custom.
#include "io/Logger.h"
#include "game/World.h"
#include "game/node/Node.h"

GameManager::GameManager(
    Window* pWindow, std::unique_ptr<Renderer> pRenderer, std::unique_ptr<GameInstance> pGameInstance)
    : pWindow(pWindow) {
    this->pRenderer = std::move(pRenderer);
    this->pGameInstance = std::move(pGameInstance);
}

GameManager::~GameManager() {
    // Log destruction so that it will be slightly easier to read logs.
    Logger::get().info("\n\n\n-------------------- starting game manager destruction... "
                       "--------------------\n\n");
    Logger::get().flushToDisk();

    // Wait for GPU to finish all work. Make sure no GPU resource is used.
    Renderer::waitForGpuToFinishWorkUpToThisPoint();

    // Destroy world if it exists.
    {
        std::scoped_lock guard(mtxWorldData.first);

        if (mtxWorldData.second.pWorld != nullptr) {
            // Destroy world before game instance, so that no node will reference game instance.
            // Not clearing the pointer because some nodes can still reference world.
            mtxWorldData.second.pWorld->destroyWorld();

            // Can safely destroy the world object now.
            mtxWorldData.second.pWorld = nullptr;
        }
    }

    // Destroy game instance.
    pGameInstance = nullptr;

    // After game instance, destroy the renderer.
    pRenderer = nullptr;
}

void GameManager::createWorld(const std::function<void(const std::optional<Error>&)>& onCreated) {
    std::scoped_lock guard(mtxWorldData.first);

    // Create new world on next tick because we might be currently iterating over "tickable" nodes
    // or nodes that receiving input so avoid modifying those arrays in this case.
    mtxWorldData.second.pendingWorldCreationTask =
        WorldCreationTask{.onCreated = onCreated, .pathToNodeTreeToLoad = {}};
}

void GameManager::onGameStarted() {
    // Log game start so that it will be slightly easier to read logs.
    Logger::get().info(
        "\n\n\n------------------------------ game started ------------------------------\n\n");
    Logger::get().flushToDisk();

    pGameInstance->onGameStarted();
}

void GameManager::onBeforeNewFrame(float timeSincePrevCallInSec) {
    {
        std::scoped_lock guard(mtxWorldData.first);

        // Create a new world if needed.
        if (mtxWorldData.second.pendingWorldCreationTask.has_value()) {
            // Create new world.
            if (mtxWorldData.second.pWorld != nullptr) {
                // Explicitly destroying instead of `nullptr` because some nodes can still reference world.
                mtxWorldData.second.pWorld->destroyWorld();
            }
            mtxWorldData.second.pWorld = std::unique_ptr<World>(new World(this));

            // Done.
            mtxWorldData.second.pendingWorldCreationTask->onCreated({});
            mtxWorldData.second.pendingWorldCreationTask = {};
        }
    }

    // Tick game instance.
    pGameInstance->onBeforeNewFrame(timeSincePrevCallInSec);

    {
        // Tick nodes.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld == nullptr) {
            return;
        }
        mtxWorldData.second.pWorld->tickTickableNodes(timeSincePrevCallInSec);
    }
}

void GameManager::onKeyboardInput(KeyboardKey key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    // Trigger raw (no events) input processing function.
    pGameInstance->onKeyboardInput(key, modifiers, bIsPressedDown);

    // Trigger input events.
    triggerActionEvents(key, modifiers, bIsPressedDown);
    triggerAxisEvents(key, modifiers, bIsPressedDown);
}

void GameManager::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    // Trigger raw (no events) input processing function.
    pGameInstance->onMouseInput(button, modifiers, bIsPressedDown);

    // Trigger input events.
    triggerActionEvents(button, modifiers, bIsPressedDown);
}

void GameManager::onMouseMove(int iXOffset, int iYOffset) {
    // Trigger game instance logic.
    pGameInstance->onMouseMove(iXOffset, iYOffset);

    // Call on nodes that receive input.
    std::scoped_lock guard(mtxWorldData.first);
    if (mtxWorldData.second.pWorld != nullptr) {
        const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
        const auto pNodes = nodesGuard.getNodes();
        for (const auto& pNode : *pNodes) {
            pNode->onMouseMove(iXOffset, iYOffset);
        }
    }
}

void GameManager::onMouseScrollMove(int iOffset) {
    // Trigger game instance logic.
    pGameInstance->onMouseScrollMove(iOffset);

    // Call on nodes that receive input.
    std::scoped_lock guard(mtxWorldData.first);
    if (mtxWorldData.second.pWorld != nullptr) {
        const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
        const auto pNodes = nodesGuard.getNodes();
        for (const auto& pNode : *pNodes) {
            pNode->onMouseScrollMove(iOffset);
        }
    }
}

void GameManager::onWindowFocusChanged(bool bIsFocused) const {
    pGameInstance->onWindowFocusChanged(bIsFocused);
}

void GameManager::onWindowClose() const { pGameInstance->onWindowClose(); }

void GameManager::triggerActionEvents(
    std::variant<KeyboardKey, MouseButton> key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(inputManager.mtxActionEvents);

    // Make sure this key is registered in some action.
    const auto it = inputManager.actionEvents.find(key);
    if (it == inputManager.actionEvents.end()) {
        return;
    }

    // Copying array of actions because it can be modified in `onInputActionEvent` by user code
    // while we are iterating over it (the user should be able to modify it).
    // This should not be that bad due to the fact that action events is just a small array of ints.
    const auto vActionIds = it->second;
    for (const auto& iActionId : vActionIds) {
        // Get state of the action.
        const auto actionStateIt = inputManager.actionState.find(iActionId);
        if (actionStateIt == inputManager.actionState.end()) [[unlikely]] {
            // Unexpected, nothing to process.
            Logger::get().error(
                std::format("input manager returned 0 states for action event with ID {}", iActionId));
            continue;
        }

        // Stores various keys that can activate this action (for example we can have
        // buttons W and ArrowUp activating the same event named "moveForward").
        std::pair<std::vector<ActionState>, bool /* action state */>& actionStatePair = actionStateIt->second;

        // Find an action key that matches the received one.
        bool bFoundKey = false;
        for (auto& actionKey : actionStatePair.first) {
            if (actionKey.key == key) {
                // Mark key's state.
                actionKey.bIsPressed = bIsPressedDown;
                bFoundKey = true;
                break;
            }
        }

        // Log an error if the key is not found.
        if (!bFoundKey) [[unlikely]] {
            if (std::holds_alternative<KeyboardKey>(key)) {
                Logger::get().error(std::format(
                    "could not find the key `{}` in key states for action event with ID {}",
                    getKeyName(std::get<KeyboardKey>(key)),
                    iActionId));
            } else {
                Logger::get().error(std::format(
                    "could not find mouse button `{}` in key states for action event with ID {}",
                    static_cast<int>(std::get<MouseButton>(key)),
                    iActionId));
            }
        }

        // Save received key state as action state.
        bool bNewActionState = bIsPressedDown;

        if (!bIsPressedDown) {
            // The key is not pressed but this does not mean that we need to broadcast
            // a notification about changed state. See if other button are pressed.
            for (const auto actionKey : actionStatePair.first) {
                if (actionKey.bIsPressed) {
                    // Save action state.
                    bNewActionState = true;
                    break;
                }
            }
        }

        // See if action state is changed.
        if (bNewActionState == actionStatePair.second) {
            continue;
        }

        // Action state was changed, notify the game.

        // Save new action state.
        actionStatePair.second = bNewActionState;

        // Notify game instance.
        pGameInstance->onInputActionEvent(iActionId, modifiers, bNewActionState);

        // Notify nodes that receive input.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld != nullptr) {
            const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
            const auto pNodes = nodesGuard.getNodes();
            for (const auto& pNode : *pNodes) {
                pNode->onInputActionEvent(iActionId, modifiers, bNewActionState);
            }
        }
    }
}

void GameManager::triggerAxisEvents(KeyboardKey key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock<std::recursive_mutex> guard(inputManager.mtxAxisEvents);

    // Make sure this key is registered in some axis event.
    const auto it = inputManager.axisEvents.find(key);
    if (it == inputManager.axisEvents.end()) {
        return;
    }

    // Copying array of axis events because it can be modified in `onInputAxisEvent` by user code
    // while we are iterating over it (the user should be able to modify it).
    // This should not be that bad due to the fact that axis events is just a small array of ints.
    const auto axisCopy = it->second;
    for (const auto& [iAxisEventId, bTriggersPlusInput] : axisCopy) {
        // Get state of the action.
        auto axisStateIt = inputManager.axisState.find(iAxisEventId);
        if (axisStateIt == inputManager.axisState.end()) [[unlikely]] {
            // Unexpected.
            Logger::get().error(
                std::format("input manager returned 0 states for axis event with ID {}", iAxisEventId));
            continue;
        }

        // Stores various keys that can activate this action (for example we can have
        // buttons W and ArrowUp activating the same event named "moveForward").
        std::pair<std::vector<AxisState>, int /* last input */>& axisStatePair = axisStateIt->second;

        // Find an action key that matches the received one.
        bool bFound = false;
        for (auto& state : axisStatePair.first) {
            if (bTriggersPlusInput && state.plusKey == key) {
                // Found it, it's registered as plus key.
                // Mark key's state.
                state.bIsPlusKeyPressed = bIsPressedDown;
                bFound = true;
                break;
            }

            if (!bTriggersPlusInput && state.minusKey == key) {
                // Found it, it's registered as minus key.
                // Mark key's state.
                state.bIsMinusKeyPressed = bIsPressedDown;
                bFound = true;
                break;
            }
        }

        // Log an error if the key is not found.
        if (!bFound) [[unlikely]] {
            Logger::get().error(std::format(
                "could not find key `{}` in key states for axis event with ID {}",
                getKeyName(key),
                iAxisEventId));
            continue;
        }

        // Save action's state as key state.
        int iAxisInputState = 0;
        if (bIsPressedDown) {
            iAxisInputState = bTriggersPlusInput ? 1 : -1;
        }

        if (!bIsPressedDown) {
            // The key is not pressed but this does not mean that we need to broadcast
            // a notification about state being equal to 0. See if other button are pressed.
            for (const AxisState& state : axisStatePair.first) {
                if (!bTriggersPlusInput && state.bIsPlusKeyPressed) { // Look for plus button.
                    iAxisInputState = 1;
                    break;
                }

                if (bTriggersPlusInput && state.bIsMinusKeyPressed) { // Look for minus button.
                    iAxisInputState = -1;
                    break;
                }
            }
        }

        // See if axis state is changed.
        if (iAxisInputState == axisStatePair.second) {
            continue;
        }

        // Axis state was changed, notify the game.

        // Save new axis state.
        axisStatePair.second = iAxisInputState;

        // Notify game instance.
        pGameInstance->onInputAxisEvent(iAxisEventId, modifiers, static_cast<float>(iAxisInputState));

        // Notify nodes that receive input.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld != nullptr) {
            const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
            const auto pNodes = nodesGuard.getNodes();
            for (const auto& pNode : *pNodes) {
                pNode->onInputAxisEvent(iAxisEventId, modifiers, static_cast<float>(iAxisInputState));
            }
        }
    }
}

Window* GameManager::getWindow() const { return pWindow; }

InputManager* GameManager::getInputManager() { return &inputManager; }

Renderer* GameManager::getRenderer() const { return pRenderer.get(); }

GameInstance* GameManager::getGameInstance() const { return pGameInstance.get(); }
