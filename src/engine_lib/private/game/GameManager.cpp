#include "GameManager.h"

// Custom.
#include "io/Logger.h"
#include "game/World.h"
#include "misc/ReflectedTypeDatabase.h"
#include "game/camera/CameraManager.h"
#include "game/node/Node.h"
#include "misc/Profiler.hpp"
#include "render/UiManager.h"
#include "game/Window.h"

// External.
#if defined(ENGINE_PROFILER_ENABLED)
#include "tracy/public/common/TracySystem.hpp"
#endif

GameManager::GameManager(
    Window* pWindow, std::unique_ptr<Renderer> pRenderer, std::unique_ptr<GameInstance> pGameInstance)
    : pWindow(pWindow) {
#if defined(ENGINE_PROFILER_ENABLED)
    tracy::SetThreadName("main thread");
    Logger::get().info("profiler enabled");
#endif

    this->pRenderer = std::move(pRenderer);
    this->pGameInstance = std::move(pGameInstance);
    pCameraManager = std::make_unique<CameraManager>(pRenderer.get());

    ReflectedTypeDatabase::registerEngineTypes();
}

void GameManager::destroyCurrentWorld() {
    // Wait for GPU to finish all work (just in case).
    Renderer::waitForGpuToFinishWorkUpToThisPoint();

    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pWorld != nullptr) {
        // Not clearing the pointer because some nodes can still reference world.
        mtxWorldData.second.pWorld->destroyWorld();

        // Can safely destroy the world object now.
        mtxWorldData.second.pWorld = nullptr;
    }
}

void GameManager::destroy() {
    // Log destruction so that it will be slightly easier to read logs.
    Logger::get().info(
        "\n\n\n-------------------- starting game manager destruction... "
        "--------------------\n\n");
    Logger::get().flushToDisk();

    // Destroy world before game instance, so that no node will reference game instance.
    destroyCurrentWorld();

    threadPool.stop();

    // Make sure all nodes were destroyed.
    const auto iAliveNodeCount = Node::getAliveNodeCount();
    if (iAliveNodeCount != 0) {
        Logger::get().error(
            std::format("the world was destroyed but there are still {} node(s) alive", iAliveNodeCount));
    }

    // Destroy game instance before renderer.
    pGameInstance = nullptr;

    // After game instance, destroy the renderer.
    pRenderer = nullptr;

    // Done.
    bIsDestroyed = true;
}

GameManager::~GameManager() {
    // Manager must be destroyed by the Window before being set to `nullptr`.
    if (bIsDestroyed) {
        return;
    }

    Error::showErrorAndThrowException("unexpected state");
}

void GameManager::createWorld(const std::function<void()>& onCreated) {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pendingWorldCreationTask.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "world is already being created/loaded, wait until the world is loaded");
    }

    // Create new world on the next tick because we might be currently iterating over "tickable" nodes
    // or nodes that receiving input so avoid modifying those arrays in this case.
    mtxWorldData.second.pendingWorldCreationTask =
        WorldCreationTask{.onCreated = onCreated, .optionalNodeTreeLoadTask = {}};
}

void GameManager::loadNodeTreeAsWorld(
    const std::filesystem::path& pathToNodeTreeFile, const std::function<void()>& onLoaded) {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pendingWorldCreationTask.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "world is already being created/loaded, wait until the world is loaded");
    }

    // Create new world on the next tick because we might be currently iterating over "tickable" nodes
    // or nodes that receiving input so avoid modifying those arrays in this case.
    mtxWorldData.second.pendingWorldCreationTask = WorldCreationTask{
        .onCreated = onLoaded,
        .optionalNodeTreeLoadTask = WorldCreationTask::LoadNodeTreeTask{
            .pathToNodeTreeToLoad = pathToNodeTreeFile, .pLoadedNodeTreeRoot = nullptr}};
}

void GameManager::addTaskToThreadPool(const std::function<void()>& task) {
    if (threadPool.isStopped()) {
        // Being destroyed, don't queue any new tasks.
        return;
    }

    threadPool.addTask(task);
}

size_t GameManager::getReceivingInputNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pWorld == nullptr) {
        return 0;
    }

    const auto nodeGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
    return nodeGuard.getNodes()->size();
}

size_t GameManager::getTotalSpawnedNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pWorld == nullptr) {
        return 0;
    }

    return mtxWorldData.second.pWorld->getTotalSpawnedNodeCount();
}

size_t GameManager::getCalledEveryFrameNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pWorld == nullptr) {
        return 0;
    }

    return mtxWorldData.second.pWorld->getCalledEveryFrameNodeCount();
}

Node* GameManager::getWorldRootNode() {
    std::scoped_lock guard(mtxWorldData.first);

    if (mtxWorldData.second.pWorld == nullptr) {
        return nullptr;
    }

    return mtxWorldData.second.pWorld->getRootNode();
}

void GameManager::onGameStarted() {
    // Log game start so that it will be slightly easier to read logs.
    Logger::get().info(
        "\n\n\n------------------------------ game started ------------------------------\n\n");
    Logger::get().flushToDisk();

    pGameInstance->onGameStarted();
}

void GameManager::onBeforeNewFrame(float timeSincePrevCallInSec) {
    PROFILE_FUNC

    {
        PROFILE_SCOPE("check world creation task")

        std::scoped_lock guard(mtxWorldData.first);

        auto& worldCreationTask = mtxWorldData.second.pendingWorldCreationTask;

        // Create a new world if needed.
        if (worldCreationTask.has_value()) {
            if (!worldCreationTask->optionalNodeTreeLoadTask.has_value()) {
                destroyCurrentWorld();
                mtxWorldData.second.pWorld = std::unique_ptr<World>(new World(this));

                // Clear task before calling the callback because inside of the callback the user might
                // queue new world creation.
                auto callback = std::move(worldCreationTask->onCreated);
                worldCreationTask = {};
                callback();
            } else {
                if (!worldCreationTask->optionalNodeTreeLoadTask->bIsAsyncTaskStarted) {
                    worldCreationTask->optionalNodeTreeLoadTask->bIsAsyncTaskStarted = true;

                    addTaskToThreadPool(
                        [this,
                         pathToNodeTree = mtxWorldData.second.pendingWorldCreationTask
                                              ->optionalNodeTreeLoadTask->pathToNodeTreeToLoad]() {
                            // Deserialize node tree.
                            auto result = Node::deserializeNodeTree(pathToNodeTree);
                            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                                auto error = std::get<Error>(std::move(result));
                                error.addCurrentLocationToErrorStack();
                                error.showErrorAndThrowException();
                            }
                            auto pRootNode = std::get<std::unique_ptr<Node>>(std::move(result));

                            std::scoped_lock guard(mtxWorldData.first);

                            // Create new world on the next tick in the main thread.
                            mtxWorldData.second.pendingWorldCreationTask->optionalNodeTreeLoadTask
                                ->pLoadedNodeTreeRoot = std::move(pRootNode);
                        });
                } else if (worldCreationTask->optionalNodeTreeLoadTask->pLoadedNodeTreeRoot != nullptr) {
                    // Loaded.
                    destroyCurrentWorld();
                    mtxWorldData.second.pWorld = std::unique_ptr<World>(new World(
                        this, std::move(worldCreationTask->optionalNodeTreeLoadTask->pLoadedNodeTreeRoot)));

                    // Same thing, clear task before callback.
                    auto callback = std::move(worldCreationTask->onCreated);
                    worldCreationTask = {};
                    callback();
                }
            }
        }
    }

    {
        PROFILE_SCOPE("tick game instance")
        pGameInstance->onBeforeNewFrame(timeSincePrevCallInSec);
    }

    {
        PROFILE_SCOPE("tick nodes")

        // Tick nodes.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld == nullptr) {
            return;
        }
        mtxWorldData.second.pWorld->tickTickableNodes(timeSincePrevCallInSec);
    }
}

void GameManager::onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
        // Trigger raw (no events) input processing function.
        pGameInstance->onKeyboardInput(key, modifiers, bIsPressedDown);

        // Trigger input events.
        triggerActionEvents(key, modifiers, bIsPressedDown);
        triggerAxisEvents(key, modifiers, bIsPressedDown);
    }

    // Notify UI.
    pRenderer->getUiManager().onKeyboardInput(key, modifiers, bIsPressedDown);
}

void GameManager::onGamepadInput(GamepadButton button, bool bIsPressedDown) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
        // Trigger raw (no events) input processing function.
        pGameInstance->onGamepadInput(button, bIsPressedDown);

        // Trigger action events.
        triggerActionEvents(button, KeyboardModifiers(0), bIsPressedDown);
    }
}

void GameManager::onGamepadAxisMoved(GamepadAxis axis, float position) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
        // Trigger raw (no events) input processing function.
        pGameInstance->onGamepadAxisMoved(axis, position);

        // Trigger axis events.
        triggerAxisEvents(axis, position);
    }
}

void GameManager::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
        // Trigger raw (no events) input processing function.
        pGameInstance->onMouseInput(button, modifiers, bIsPressedDown);

        // Trigger input events.
        triggerActionEvents(button, modifiers, bIsPressedDown);
    }

    if (getWindow()->isCursorVisible()) {
        // Notify UI.
        pRenderer->getUiManager().onMouseInput(button, modifiers, bIsPressedDown);
    }
}

void GameManager::onMouseMove(int iXOffset, int iYOffset) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
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

    if (getWindow()->isCursorVisible()) {
        // Notify UI.
        pRenderer->getUiManager().onMouseMove(iXOffset, iYOffset);
    }
}

void GameManager::onMouseScrollMove(int iOffset) {
    if (!pRenderer->getUiManager().hasModalUiNodeTree()) {
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

    if (getWindow()->isCursorVisible()) {
        // Notify UI.
        pRenderer->getUiManager().onMouseScrollMove(iOffset);
    }
}

void GameManager::onGamepadConnected(std::string_view sGamepadName) {
    Logger::get().info(std::format("gamepad \"{}\" was connected", sGamepadName));

    pGameInstance->onGamepadConnected(sGamepadName);
}

void GameManager::onGamepadDisconnected() {
    Logger::get().info("gamepad was disconnected");

    pGameInstance->onGamepadDisconnected();
}

void GameManager::onWindowFocusChanged(bool bIsFocused) const {
    pGameInstance->onWindowFocusChanged(bIsFocused);
}

void GameManager::onWindowClose() const { pGameInstance->onWindowClose(); }

void GameManager::triggerActionEvents(
    std::variant<KeyboardButton, MouseButton, GamepadButton> button,
    KeyboardModifiers modifiers,
    bool bIsPressedDown) {
    std::scoped_lock guard(inputManager.mtxActionEvents);

    // Make sure this button is registered in some action.
    const auto it = inputManager.buttonToActionEvents.find(button);
    if (it == inputManager.buttonToActionEvents.end()) {
        // That's okay, this button is not used in input events.
        return;
    }

    // Copying array of actions because it can be modified in `onInputActionEvent` by user code
    // while we are iterating over it (the user should be able to modify it).
    // This should not be that bad due to the fact that action events is just a small array of ints.
    const auto vActionIds = it->second;
    for (const auto& iActionId : vActionIds) {
        // Get state of the event.
        const auto actionStateIt = inputManager.actionEventStates.find(iActionId);
        if (actionStateIt == inputManager.actionEventStates.end()) [[unlikely]] {
            // Unexpected, nothing to process.
            Logger::get().error(
                std::format("input manager returned 0 states for action event with ID {}", iActionId));
            continue;
        }

        // Various keys can activate an action (for example we can have buttons W and ArrowUp activating the
        // same event named "moveForward") but an action event will have a single state (pressed/not pressed)
        // depending on all buttons that can trigger the event.
        auto& [vStates, bActionEventLastState] = actionStateIt->second;

        // Find a button that matches the received one.
        bool bFoundKey = false;
        for (auto& state : vStates) {
            if (state.triggerButton == button) {
                // Found it, mark a new state for this button.
                state.bIsPressed = bIsPressedDown;
                bFoundKey = true;
                break;
            }
        }

        // We should have found a trigger button.
        if (!bFoundKey) [[unlikely]] {
            if (std::holds_alternative<KeyboardButton>(button)) {
                Logger::get().error(
                    std::format(
                        "could not find the keyboard button `{}` in trigger buttons for action event with ID "
                        "{}",
                        getKeyboardButtonName(std::get<KeyboardButton>(button)),
                        iActionId));
            } else if (std::holds_alternative<KeyboardButton>(button)) {
                Logger::get().error(
                    std::format(
                        "could not find the mouse button `{}` in trigger buttons for action event with ID {}",
                        static_cast<int>(std::get<MouseButton>(button)),
                        iActionId));
            } else if (std::holds_alternative<GamepadButton>(button)) {
                Logger::get().error(
                    std::format(
                        "could not find the gamepad button `{}` in trigger buttons for action event with ID "
                        "{}",
                        static_cast<int>(std::get<GamepadButton>(button)),
                        iActionId));
            } else [[unlikely]] {
                Error::showErrorAndThrowException("unhandled case");
            }
        }

        // Prepare to save the received button state as the new state of this action event.
        bool bNewActionState = bIsPressedDown;

        if (!bIsPressedDown) {
            // We received "button released" input but this does not mean that we need to broadcast
            // a notification about changed action event state. First, see if other button are pressed.
            for (const auto state : vStates) {
                if (state.bIsPressed) {
                    // At least one of the trigger buttons is still pressed so the action event state
                    // should still be "pressed".
                    bNewActionState = true;
                    break;
                }
            }
        }

        // See if action state is changed.
        if (bNewActionState == bActionEventLastState) {
            continue;
        }

        // Save new action state.
        bActionEventLastState = bNewActionState;

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

void GameManager::triggerAxisEvents(KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock<std::recursive_mutex> guard(inputManager.mtxAxisEvents);

    // Make sure this button is registered in some axis event.
    const auto axisEventsToTriggerIt = inputManager.keyboardButtonToAxisEvents.find(button);
    if (axisEventsToTriggerIt == inputManager.keyboardButtonToAxisEvents.end()) {
        return;
    }

    // Copying array of axis events because it can be modified in `onInputAxisEvent` by user code
    // while we are iterating over it (the user should be able to modify it).
    // This should not be that bad due to the fact that an axis event is just a very small array of values.
    const auto axisEventsToTrigger = axisEventsToTriggerIt->second;
    for (const auto& [iAxisEventId, bTriggersPositiveInput] : axisEventsToTrigger) {
        // Get state of the event.
        auto axisStateIt = inputManager.axisEventStates.find(iAxisEventId);
        if (axisStateIt == inputManager.axisEventStates.end()) [[unlikely]] {
            // Unexpected.
            Logger::get().error(
                std::format("input manager returned 0 states for axis event with ID {}", iAxisEventId));
            continue;
        }

        // Stores various triggers that can activate this event (for example we can have
        // buttons W and ArrowUp activating the same event named "moveForward").
        auto& axisEventState = axisStateIt->second;

        // Find an event trigger that matches the received one.
        bool bFound = false;
        for (auto& triggerState : axisEventState.vKeyboardTriggers) {
            if (bTriggersPositiveInput && triggerState.positiveTrigger == button) {
                // Found it, update state of this trigger.
                triggerState.bIsPositiveTriggerPressed = bIsPressedDown;
                bFound = true;
                break;
            }

            if (!bTriggersPositiveInput && triggerState.negativeTrigger == button) {
                // Found it, update state of this trigger.
                triggerState.bIsNegativeTriggerPressed = bIsPressedDown;
                bFound = true;
                break;
            }
        }

        // Log an error if the key is not found.
        if (!bFound) [[unlikely]] {
            Logger::get().error(
                std::format(
                    "could not find key `{}` in key states for axis event with ID {}",
                    getKeyboardButtonName(button),
                    iAxisEventId));
            continue;
        }

        // Prepare a new state for this event.
        float axisState = 0.0F;
        if (bIsPressedDown) {
            axisState = bTriggersPositiveInput ? 1.0F : -1.0F;
        }

        if (!bIsPressedDown) {
            // The key is not pressed but this does not mean that we need to broadcast
            // a notification about state being equal to 0. See if other button are pressed.
            for (const auto& state : axisEventState.vKeyboardTriggers) {
                if (!bTriggersPositiveInput && state.bIsPositiveTriggerPressed) {
                    axisState = 1.0F;
                    break;
                }

                if (bTriggersPositiveInput && state.bIsNegativeTriggerPressed) {
                    axisState = -1.0F;
                    break;
                }
            }
        }

        // Save new axis state.
        axisEventState.state = axisState;

        // Notify game instance.
        pGameInstance->onInputAxisEvent(iAxisEventId, modifiers, axisState);

        // Notify nodes that receive input.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld != nullptr) {
            const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
            const auto pNodes = nodesGuard.getNodes();
            for (const auto& pNode : *pNodes) {
                pNode->onInputAxisEvent(iAxisEventId, modifiers, axisState);
            }
        }
    }
}

void GameManager::triggerAxisEvents(GamepadAxis gamepadAxis, float position) {
    std::scoped_lock<std::recursive_mutex> guard(inputManager.mtxAxisEvents);

    // Make sure this axis is registered in some axis event.
    const auto axisEventsToTriggerIt = inputManager.gamepadAxisToAxisEvents.find(gamepadAxis);
    if (axisEventsToTriggerIt == inputManager.gamepadAxisToAxisEvents.end()) {
        return;
    }

    // Copying array of axis events because it can be modified in `onInputAxisEvent` by user code
    // while we are iterating over it (the user should be able to modify it).
    // This should not be that bad due to the fact that an axis event is just a very small array of values.
    const auto axisEventsToTrigger = axisEventsToTriggerIt->second;
    for (const auto& iAxisEventId : axisEventsToTrigger) {
        // Get state of the event.
        auto axisStateIt = inputManager.axisEventStates.find(iAxisEventId);
        if (axisStateIt == inputManager.axisEventStates.end()) [[unlikely]] {
            // Unexpected.
            Logger::get().error(
                std::format("input manager returned 0 states for axis event with ID {}", iAxisEventId));
            continue;
        }

        // Stores various triggers that can activate this event.
        auto& axisEventState = axisStateIt->second;

        // Find an event trigger that matches the received one.
        bool bFound = false;
        for (auto& triggerState : axisEventState.vGamepadTriggers) {
            if (triggerState.trigger == gamepadAxis) {
                // Found it, update state of this trigger.
                triggerState.lastPosition = position;
                bFound = true;
                break;
            }
        }

        // Log an error if the key is not found.
        if (!bFound) [[unlikely]] {
            Logger::get().error(
                std::format(
                    "could not find gamepad axis `{}` in axis states for axis event with ID {}",
                    getGamepadAxisName(gamepadAxis),
                    iAxisEventId));
            continue;
        }

        // Prepare new state for this event.
        float newEventState = position;
        const float oldEventState = axisEventState.state;

        if (abs(newEventState) < inputManager.getGamepadDeadzone()) {
            newEventState = 0.0F;
        }

        // Save new state.
        axisEventState.state = newEventState;

        if (abs(oldEventState) < inputManager.getGamepadDeadzone() && newEventState == 0.0F) {
            // Don't broadcast a notification since we had 0 input before and still have 0 input.
            continue;
        }

        // Notify game instance.
        pGameInstance->onInputAxisEvent(iAxisEventId, KeyboardModifiers(0), axisEventState.state);

        // Notify nodes that receive input.
        std::scoped_lock guard(mtxWorldData.first);
        if (mtxWorldData.second.pWorld != nullptr) {
            const auto nodesGuard = mtxWorldData.second.pWorld->getReceivingInputNodes();
            const auto pNodes = nodesGuard.getNodes();
            for (const auto& pNode : *pNodes) {
                pNode->onInputAxisEvent(iAxisEventId, KeyboardModifiers(0), axisEventState.state);
            }
        }
    }
}

Window* GameManager::getWindow() const { return pWindow; }

InputManager* GameManager::getInputManager() { return &inputManager; }

CameraManager* GameManager::getCameraManager() const { return pCameraManager.get(); }

Renderer* GameManager::getRenderer() const { return pRenderer.get(); }

GameInstance* GameManager::getGameInstance() const { return pGameInstance.get(); }
