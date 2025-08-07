#include "GameManager.h"

// Custom.
#include "io/Logger.h"
#include "misc/ReflectedTypeDatabase.h"
#include "game/camera/CameraManager.h"
#include "game/node/Node.h"
#include "misc/Profiler.hpp"
#include "render/UiNodeManager.h"
#include "game/Window.h"
#include "sound/SoundManager.h"

// External.
#if defined(ENGINE_PROFILER_ENABLED)
#include "tracy/public/common/TracySystem.hpp"
#endif

GameManager::WorldCreationTask::~WorldCreationTask() {}
GameManager::WorldCreationTask::LoadNodeTreeTask::~LoadNodeTreeTask() {}
GameManager::WorldData::~WorldData() {}

GameManager::GameManager(
    Window* pWindow, std::unique_ptr<Renderer> pRenderer, std::unique_ptr<GameInstance> pGameInstance)
    : pWindow(pWindow) {
#if defined(ENGINE_PROFILER_ENABLED)
    tracy::SetThreadName("main thread");
    Logger::get().info("profiler enabled");
#endif

#if defined(ENGINE_ASAN_ENABLED)
    Logger::get().info("AddressSanitizer (ASan) is enabled, expect increased RAM usage!");
#endif

    this->pRenderer = std::move(pRenderer);
    this->pGameInstance = std::move(pGameInstance);
    pSoundManager = std::unique_ptr<SoundManager>(new SoundManager());

    ReflectedTypeDatabase::registerEngineTypes();
}

void GameManager::onBeforeWorldDestroyed(Node* pRootNode) {
    pGameInstance->onBeforeWorldDestroyed(pRootNode);
}

void GameManager::destroyWorldsImmediately() {
    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        // Not clearing the pointer because some nodes can still reference world.
        pWorld->destroyWorld();

        // Can safely destroy the world object now.
        pWorld = nullptr;
    }
    mtxWorldData.second.vWorlds.clear();
}

void GameManager::destroy() {
    // Log destruction so that it will be slightly easier to read logs.
    Logger::get().info("\n\n\n-------------------- starting game manager destruction... "
                       "--------------------\n\n");
    Logger::get().flushToDisk();

    // Destroy world before game instance, so that no node will reference game instance.
    destroyWorldsImmediately();

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

void GameManager::createWorld(const std::function<void(Node*)>& onCreated, bool bDestroyOldWorlds) {
    std::scoped_lock guard(mtxWorldData.first);
    auto& pOptionalTask = mtxWorldData.second.pPendingWorldCreationTask;

    if (pOptionalTask != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "world is already being created/loaded, wait until the world is loaded");
    }

    // Create new world on the next tick because we might be currently iterating over "tickable" nodes
    // or nodes that receiving input so avoid modifying those arrays in this case.
    pOptionalTask = std::unique_ptr<WorldCreationTask>(new WorldCreationTask());
    pOptionalTask->onCreated = onCreated;
    pOptionalTask->bDestroyOldWorlds = bDestroyOldWorlds;
}

void GameManager::loadNodeTreeAsWorld(
    const std::filesystem::path& pathToNodeTreeFile,
    const std::function<void(Node*)>& onLoaded,
    bool bDestroyOldWorlds) {
    std::scoped_lock guard(mtxWorldData.first);
    auto& pOptionalTask = mtxWorldData.second.pPendingWorldCreationTask;

    if (pOptionalTask != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "world is already being created/loaded, wait until the world is loaded");
    }

    // Create new world on the next tick because we might be currently iterating over "tickable" nodes
    // or nodes that receiving input so avoid modifying those arrays in this case.
    pOptionalTask = std::unique_ptr<WorldCreationTask>(new WorldCreationTask());
    pOptionalTask->onCreated = onLoaded;
    pOptionalTask->pOptionalNodeTreeLoadTask =
        std::unique_ptr<WorldCreationTask::LoadNodeTreeTask>(new WorldCreationTask::LoadNodeTreeTask());
    pOptionalTask->pOptionalNodeTreeLoadTask->pathToNodeTreeToLoad = pathToNodeTreeFile;
    pOptionalTask->bDestroyOldWorlds = bDestroyOldWorlds;
}

void GameManager::addTaskToThreadPool(const std::function<void()>& task) {
    if (threadPool.isStopped()) {
        // Being destroyed, don't queue any new tasks.
        return;
    }

    threadPool.addTask(task);
}

void GameManager::destroyWorld(World* pWorldToDestroy, const std::function<void()>& onAfterDestroyed) {
    std::scoped_lock guard(mtxWorldData.first);

    // Don't destroy world right now because this function might be called from one of the world's entities
    // (such as UI button click) which means that some world logic is still running and should not be
    // destroyed.

    mtxWorldData.second.pWorldDestroyTask = std::make_unique<WorldDestroyTask>();
    mtxWorldData.second.pWorldDestroyTask->pWorld = pWorldToDestroy;
    mtxWorldData.second.pWorldDestroyTask->onAfterDestroyed = onAfterDestroyed;
}

size_t GameManager::getReceivingInputNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    size_t iNodeCount = 0;
    for (const auto& pWorld : mtxWorldData.second.vWorlds) {
        const auto nodeGuard = pWorld->getReceivingInputNodes();
        iNodeCount += nodeGuard.getNodes()->size();
    }
    return iNodeCount;
}

size_t GameManager::getTotalSpawnedNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    size_t iNodeCount = 0;
    for (const auto& pWorld : mtxWorldData.second.vWorlds) {
        iNodeCount += pWorld->getTotalSpawnedNodeCount();
    }

    return iNodeCount;
}

size_t GameManager::getCalledEveryFrameNodeCount() {
    std::scoped_lock guard(mtxWorldData.first);

    size_t iNodeCount = 0;
    for (const auto& pWorld : mtxWorldData.second.vWorlds) {
        iNodeCount += pWorld->getCalledEveryFrameNodeCount();
    }

    return iNodeCount;
}

void GameManager::onGameStarted() {
    // Log game start so that it will be slightly easier to read logs.
    Logger::get().info(
        "\n\n\n------------------------------ game started ------------------------------\n\n");
    Logger::get().flushToDisk();

    pGameInstance->onGameStarted();
}

void GameManager::onWindowSizeChanged() {
    if (pRenderer != nullptr) {
        pRenderer->onWindowSizeChanged();
    }
    if (pGameInstance != nullptr) {
        pGameInstance->onWindowSizeChanged();
    }
}

void GameManager::onBeforeNewFrame(float timeSincePrevCallInSec) {
    PROFILE_FUNC

    {
        PROFILE_SCOPE("check world creation task")

        std::scoped_lock guard(mtxWorldData.first);

        // Destroy world if needed.
        if (mtxWorldData.second.pWorldDestroyTask != nullptr) {
            auto& vWorlds = mtxWorldData.second.vWorlds;
            auto& pTask = mtxWorldData.second.pWorldDestroyTask;

            bool bFoundWorld = false;
            for (auto it = vWorlds.begin(); it != vWorlds.end(); it++) {
                auto& pWorld = *it;

                if (pWorld.get() != pTask->pWorld) {
                    continue;
                }

                // Not clearing the pointer because some nodes can still reference world.
                pWorld->destroyWorld();

                // Can safely destroy the world object now.
                pWorld = nullptr;

                vWorlds.erase(it);
                bFoundWorld = true;
                break;
            }

            if (!bFoundWorld) {
                Error::showErrorAndThrowException(
                    "can't destroy world because the specified pointer is invalid");
            }

            pTask->onAfterDestroyed();
            pTask = nullptr;
        }

        auto& pWorldCreationTask = mtxWorldData.second.pPendingWorldCreationTask;

        // Create a new world if needed.
        if (pWorldCreationTask != nullptr) {
            if (pWorldCreationTask->pOptionalNodeTreeLoadTask == nullptr) {
                if (pWorldCreationTask->bDestroyOldWorlds) {
                    destroyWorldsImmediately();
                }
                Node* pRootNode = nullptr;
                {
                    auto pNewWorld = std::unique_ptr<World>(new World(this));
                    pRootNode = pNewWorld->getRootNode();
                    mtxWorldData.second.vWorlds.push_back(std::move(pNewWorld));
                }

                // Clear task before calling the callback because inside of the callback the user might
                // queue new world creation.
                auto callback = std::move(pWorldCreationTask->onCreated);
                pWorldCreationTask = nullptr;
                callback(pRootNode);
            } else {
                if (!pWorldCreationTask->pOptionalNodeTreeLoadTask->bIsAsyncTaskStarted) {
                    pWorldCreationTask->pOptionalNodeTreeLoadTask->bIsAsyncTaskStarted = true;

                    addTaskToThreadPool(
                        [this,
                         pathToNodeTree = mtxWorldData.second.pPendingWorldCreationTask
                                              ->pOptionalNodeTreeLoadTask->pathToNodeTreeToLoad]() {
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
                            mtxWorldData.second.pPendingWorldCreationTask->pOptionalNodeTreeLoadTask
                                ->pLoadedNodeTreeRoot = std::move(pRootNode);
                        });
                } else if (pWorldCreationTask->pOptionalNodeTreeLoadTask->pLoadedNodeTreeRoot != nullptr) {
                    // Loaded.
                    if (pWorldCreationTask->bDestroyOldWorlds) {
                        destroyWorldsImmediately();
                    }
                    Node* pRootNode = nullptr;
                    {
                        auto pNewWorld = std::unique_ptr<World>(new World(
                            this,
                            std::move(pWorldCreationTask->pOptionalNodeTreeLoadTask->pLoadedNodeTreeRoot)));
                        pRootNode = pNewWorld->getRootNode();
                        mtxWorldData.second.vWorlds.push_back(std::move(pNewWorld));
                    }

                    // Same thing, clear task before callback.
                    auto callback = std::move(pWorldCreationTask->onCreated);
                    pWorldCreationTask = nullptr;
                    callback(pRootNode);
                }
            }
        }
    }

    {
        PROFILE_SCOPE("tick game instance")
        pGameInstance->onBeforeNewFrame(timeSincePrevCallInSec);
    }

    std::scoped_lock guard(mtxWorldData.first);
    if (mtxWorldData.second.vWorlds.empty()) {
        return;
    }
    auto& vWorlds = mtxWorldData.second.vWorlds;

    {
        PROFILE_SCOPE("tick nodes")
        for (const auto& pWorld : vWorlds) {
            pWorld->tickTickableNodes(timeSincePrevCallInSec);
        }
    }

    // Update sound info.
    for (const auto& pWorld : vWorlds) {
        auto& mtxActiveCamera = pWorld->getCameraManager().getActiveCamera();
        std::scoped_lock guard(mtxActiveCamera.first);
        if (mtxActiveCamera.second.pNode == nullptr) {
            continue;
        }
        if (!mtxActiveCamera.second.bIsSoundListener) {
            continue;
        }
        pSoundManager->onBeforeNewFrame(pWorld->getCameraManager());
        break;
    }
}

void GameManager::onKeyboardInput(
    KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown, bool bIsRepeat) {
    if (!bIsRepeat) {
        if (bIsPressedDown) {
            pGameInstance->onKeyboardButtonPressed(key, modifiers);
        } else {
            pGameInstance->onKeyboardButtonReleased(key, modifiers);
        }
    }

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!bIsRepeat && !pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Trigger input events.
            triggerActionEvents(key, modifiers, bIsPressedDown);
            triggerAxisEvents(key, modifiers, bIsPressedDown);
        }

        // Notify UI.
        pWorld->getUiNodeManager().onKeyboardInput(key, modifiers, bIsPressedDown);
    }
}

void GameManager::onKeyboardInputTextCharacter(const std::string& sTextCharacter) {
    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        pWorld->getUiNodeManager().onKeyboardInputTextCharacter(sTextCharacter);
    }
}

void GameManager::onGamepadInput(GamepadButton button, bool bIsPressedDown) {
    if (bIsPressedDown) {
        pGameInstance->onGamepadButtonPressed(button);
    } else {
        pGameInstance->onGamepadButtonReleased(button);
    }

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Trigger action events.
            triggerActionEvents(button, KeyboardModifiers(0), bIsPressedDown);
        }

        // Notify UI.
        pWorld->getUiNodeManager().onGamepadInput(button, bIsPressedDown);
    }
}

void GameManager::onGamepadAxisMoved(GamepadAxis axis, float position) {
    pGameInstance->onGamepadAxisMoved(axis, position);

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Trigger axis events.
            triggerAxisEvents(axis, position);
        }
    }
}

void GameManager::onCursorVisibilityChanged(bool bVisibleNow) {
    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        pWorld->getUiNodeManager().onCursorVisibilityChanged(bVisibleNow);
    }
}

void GameManager::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    if (bIsPressedDown) {
        pGameInstance->onMouseButtonPressed(button, modifiers);
    } else {
        pGameInstance->onMouseButtonReleased(button, modifiers);
    }

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Trigger input events.
            triggerActionEvents(button, modifiers, bIsPressedDown);
        }

        if (getWindow()->isCursorVisible()) {
            // Notify UI.
            pWorld->getUiNodeManager().onMouseInput(button, modifiers, bIsPressedDown);
        }
    }
}

void GameManager::onMouseMove(int iXOffset, int iYOffset) {
    pGameInstance->onMouseMove(iXOffset, iYOffset);

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Trigger game instance logic.
            pGameInstance->onMouseMove(iXOffset, iYOffset);

            // Call on nodes that receive input.
            std::scoped_lock guard(mtxWorldData.first);
            for (const auto& pWorld : mtxWorldData.second.vWorlds) {
                const auto nodesGuard = pWorld->getReceivingInputNodes();
                const auto pNodes = nodesGuard.getNodes();
                for (const auto& pNode : *pNodes) {
                    pNode->onMouseMove(iXOffset, iYOffset);
                }
            }
        }

        if (getWindow()->isCursorVisible()) {
            // Notify UI.
            pWorld->getUiNodeManager().onMouseMove(iXOffset, iYOffset);
        }
    }
}

void GameManager::onMouseScrollMove(int iOffset) {
    pGameInstance->onMouseScrollMove(iOffset);

    std::scoped_lock guard(mtxWorldData.first);

    for (auto& pWorld : mtxWorldData.second.vWorlds) {
        if (!pWorld->getUiNodeManager().hasModalUiNodeTree()) {
            // Call on nodes that receive input.
            std::scoped_lock guard(mtxWorldData.first);
            for (const auto& pWorld : mtxWorldData.second.vWorlds) {
                const auto nodesGuard = pWorld->getReceivingInputNodes();
                const auto pNodes = nodesGuard.getNodes();
                for (const auto& pNode : *pNodes) {
                    pNode->onMouseScrollMove(iOffset);
                }
            }
        }

        if (getWindow()->isCursorVisible()) {
            // Notify UI.
            pWorld->getUiNodeManager().onMouseScrollMove(iOffset);
        }
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
                Logger::get().error(std::format(
                    "could not find the keyboard button `{}` in trigger buttons for action event with ID "
                    "{}",
                    getKeyboardButtonName(std::get<KeyboardButton>(button)),
                    iActionId));
            } else if (std::holds_alternative<KeyboardButton>(button)) {
                Logger::get().error(std::format(
                    "could not find the mouse button `{}` in trigger buttons for action event with ID {}",
                    static_cast<int>(std::get<MouseButton>(button)),
                    iActionId));
            } else if (std::holds_alternative<GamepadButton>(button)) {
                Logger::get().error(std::format(
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
        for (const auto& pWorld : mtxWorldData.second.vWorlds) {
            const auto nodesGuard = pWorld->getReceivingInputNodes();
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
            Logger::get().error(std::format(
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
        for (const auto& pWorld : mtxWorldData.second.vWorlds) {
            const auto nodesGuard = pWorld->getReceivingInputNodes();
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
            Logger::get().error(std::format(
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
        for (const auto& pWorld : mtxWorldData.second.vWorlds) {
            const auto nodesGuard = pWorld->getReceivingInputNodes();
            const auto pNodes = nodesGuard.getNodes();
            for (const auto& pNode : *pNodes) {
                pNode->onInputAxisEvent(iAxisEventId, KeyboardModifiers(0), axisEventState.state);
            }
        }
    }
}

Window* GameManager::getWindow() const { return pWindow; }

InputManager* GameManager::getInputManager() { return &inputManager; }

Renderer* GameManager::getRenderer() const { return pRenderer.get(); }

GameInstance* GameManager::getGameInstance() const { return pGameInstance.get(); }

SoundManager& GameManager::getSoundManager() const { return *pSoundManager; }
