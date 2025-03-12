// Custom.
#include "game/node/MeshNode.h"
#include "game/GameInstance.h"
#include "game/Window.h"

// External.
#include "catch2/catch_test_macros.hpp"

TEST_CASE("despawn invisible mesh node") {
    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                const auto pSpawnedMeshNode = getWorldRootNode()->addChildNode(std::make_unique<MeshNode>());

                pSpawnedMeshNode->setIsVisible(false);
                pSpawnedMeshNode->unsafeDetachFromParentAndDespawn();

                getWindow()->close();
            });
        }
        virtual ~TestGameInstance() override {}
    };

    auto result = Window::create("", true);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        Error error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }

    const std::unique_ptr<Window> pMainWindow = std::get<std::unique_ptr<Window>>(std::move(result));
    pMainWindow->processEvents<TestGameInstance>();
}
