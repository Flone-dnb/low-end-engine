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
            createWorld([&](Node* pRootNode) {
                const auto pSpawnedMeshNode = pRootNode->addChildNode(std::make_unique<MeshNode>());

                pSpawnedMeshNode->setIsVisible(false);
                pSpawnedMeshNode->unsafeDetachFromParentAndDespawn();

                getWindow()->close();
            });
        }
        virtual ~TestGameInstance() override {}
    };

    auto result = WindowBuilder().hidden().build();
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
            createWorld([&](Node* pRootNode) {
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
                const auto pathToGeoDir =
                    pathToDirectory /
                    (std::string("test") + std::string(Serializable::getNodeTreeGeometryDirSuffix()));
                REQUIRE(std::filesystem::exists(
                    pathToGeoDir /
                    ("0.meshGeometry." + std::string(Serializable::getBinaryFileExtension()))));
                REQUIRE(std::filesystem::exists(
                    pathToGeoDir /
                    ("1.meshGeometry." + std::string(Serializable::getBinaryFileExtension()))));

                getWindow()->close();
            });
        }
        virtual ~TestGameInstance() override {}
    };

    auto result = WindowBuilder().hidden().build();
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        Error error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }

    const std::unique_ptr<Window> pMainWindow = std::get<std::unique_ptr<Window>>(std::move(result));
    pMainWindow->processEvents<TestGameInstance>();
}
