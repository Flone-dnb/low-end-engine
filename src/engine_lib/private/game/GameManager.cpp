#include "GameManager.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "io/Logger.h"
#include "game/World.h"

std::variant<std::unique_ptr<GameManager>, Error> GameManager::create(Window* pWindow) {
    // Create renderer.
    auto result = Renderer::create(pWindow);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        return error;
    }
    auto pRenderer = std::get<std::unique_ptr<Renderer>>(std::move(result));

    // Create game instance.
    auto pGameInstance = std::unique_ptr<GameInstance>(new GameInstance(pWindow));

    return std::unique_ptr<GameManager>(
        new GameManager(pWindow, std::move(pRenderer), std::move(pGameInstance)));
}

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
    pRenderer->waitForGpuToFinishWorkUpToThisPoint();

    // Destroy world if it exists.
    {
        std::scoped_lock guard(mtxWorld.first);

        if (mtxWorld.second != nullptr) {
            // Destroy world before game instance, so that no node will reference game instance.
            // Not clearing the pointer because some nodes can still reference world.
            mtxWorld.second->destroyWorld();

            // Can safely destroy the world object now.
            mtxWorld.second = nullptr;
        }
    }

    // Destroy game instance.
    pGameInstance = nullptr;

    // After game instance, destroy the renderer.
    pRenderer = nullptr;
}

void GameManager::onGameStarted() {
    // Log game start so that it will be slightly easier to read logs.
    Logger::get().info(
        "\n\n\n------------------------------ game started ------------------------------\n\n");
    Logger::get().flushToDisk();

    pGameInstance->onGameStarted();
}

void GameManager::onBeforeNewFrame(float timeSincePrevCallInSec) {
    // Tick game instance.
    pGameInstance->onBeforeNewFrame(timeSincePrevCallInSec);

    {
        // Tick nodes.
        std::scoped_lock guard(mtxWorld.first);
        if (mtxWorld.second == nullptr) {
            return;
        }
        mtxWorld.second->tickTickableNodes(timeSincePrevCallInSec);
    }
}

void GameManager::onWindowClose() const { pGameInstance->onWindowClose(); }

Window* GameManager::getWindow() const { return pWindow; }

Renderer* GameManager::getRenderer() const { return pRenderer.get(); }

GameInstance* GameManager::getGameInstance() const { return pGameInstance.get(); }
