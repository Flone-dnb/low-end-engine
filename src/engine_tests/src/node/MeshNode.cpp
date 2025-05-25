// Custom.
#include "game/node/MeshNode.h"
#include "game/GameInstance.h"
#include "game/Window.h"
#include "TestFilePaths.hpp"

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

    auto result = Window::create("", true, true);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        Error error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }

    const std::unique_ptr<Window> pMainWindow = std::get<std::unique_ptr<Window>>(std::move(result));
    pMainWindow->processEvents<TestGameInstance>();
}

TEST_CASE("serialize node tree with 2 mesh nodes") {
    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                auto pRoot = std::make_unique<MeshNode>();
                pRoot->addChildNode(std::make_unique<MeshNode>());

                const auto pathToDirectory = ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) /
                                             sTestDirName / vUsedTestFileNames[8];
                auto optionalError = pRoot->serializeNodeTree(pathToDirectory / "test", false);
                if (optionalError.has_value()) [[unlikely]] {
                    optionalError->addCurrentLocationToErrorStack();
                    INFO(optionalError->getFullErrorMessage());
                    REQUIRE(false);
                }

                REQUIRE(std::filesystem::exists(pathToDirectory / "test.toml"));

                // 2 geometry files must exist.
                REQUIRE(std::filesystem::exists(pathToDirectory / "test.0.geometry.bin"));
                REQUIRE(std::filesystem::exists(pathToDirectory / "test.1.geometry.bin"));

                getWindow()->close();
            });
        }
        virtual ~TestGameInstance() override {}
    };

    auto result = Window::create("", true, true);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        Error error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }

    const std::unique_ptr<Window> pMainWindow = std::get<std::unique_ptr<Window>>(std::move(result));
    pMainWindow->processEvents<TestGameInstance>();
}
