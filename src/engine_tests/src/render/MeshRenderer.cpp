// Custom.
#include "game/node/MeshNode.h"
#include "game/GameInstance.h"
#include "game/Window.h"
#include "TestFilePaths.hpp"

// External.
#include "catch2/catch_test_macros.hpp"
#include "misc/ProjectPaths.h"
#include <filesystem>

TEST_CASE("attempt to crash/fail validation on mesh renderer") {
    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&](Node* pRootNode) {
                // Prepare a custom fragment shader (copy of the default shader).
                std::filesystem::create_directory(
                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / "shaders");

                const auto sPathToCustomFragShader =
                    std::string(sTestDirName) + "/shaders/" + std::string(vUsedTestFileNames[12]);
                const auto pathToCustomFragShader =
                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToCustomFragShader;

                const auto pathToMeshFragShader =
                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "shaders" / "node" /
                    "MeshNode.frag.glsl";
                REQUIRE(std::filesystem::exists(pathToMeshFragShader));
                std::filesystem::copy_file(pathToMeshFragShader, pathToCustomFragShader);
                REQUIRE(std::filesystem::exists(pathToCustomFragShader));

                std::filesystem::copy_file(
                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "shaders" / "Light.glsl",
                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName /
                        "Light.glsl");

                const auto spawnOpaqueMesh = [&](bool bOtherShader = false) -> MeshNode* {
                    if (!bOtherShader) {
                        return pRootNode->addChildNode(std::make_unique<MeshNode>());
                    } else {
                        auto pMeshU = std::make_unique<MeshNode>();
                        pMeshU->getMaterial().setPathToCustomFragmentShader(sPathToCustomFragShader);
                        return pRootNode->addChildNode(std::move(pMeshU));
                    }
                };
                const auto spawnTransparentMesh = [&](bool bOtherShader = false) -> MeshNode* {
                    auto pMeshU = std::make_unique<MeshNode>();
                    pMeshU->getMaterial().setEnableTransparency(true);
                    if (bOtherShader) {
                        pMeshU->getMaterial().setPathToCustomFragmentShader(sPathToCustomFragShader);
                    }
                    return pRootNode->addChildNode(std::move(pMeshU));
                };

                {
                    auto pMesh = spawnOpaqueMesh();
                    pMesh->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh = spawnTransparentMesh();
                    pMesh->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh2 = spawnOpaqueMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnTransparentMesh();
                    auto pMesh2 = spawnTransparentMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh2 = spawnTransparentMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnTransparentMesh();
                    auto pMesh2 = spawnOpaqueMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh2 = spawnOpaqueMesh(true);
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnTransparentMesh();
                    auto pMesh2 = spawnTransparentMesh(true);
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnTransparentMesh();
                    auto pMesh2 = spawnOpaqueMesh();
                    auto pMesh3 = spawnOpaqueMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                    pMesh3->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh3 = spawnTransparentMesh();
                    auto pMesh2 = spawnTransparentMesh(true);
                    auto pMesh4 = spawnTransparentMesh();
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                    pMesh3->unsafeDetachFromParentAndDespawn();
                    pMesh4->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh2 = spawnOpaqueMesh(true);
                    auto pMesh3 = spawnTransparentMesh();
                    auto pMesh4 = spawnTransparentMesh(true);
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                    pMesh3->unsafeDetachFromParentAndDespawn();
                    pMesh4->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnOpaqueMesh();
                    auto pMesh2 = spawnTransparentMesh();
                    auto pMesh3 = spawnOpaqueMesh(true);
                    auto pMesh4 = spawnTransparentMesh(true);
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                    pMesh3->unsafeDetachFromParentAndDespawn();
                    pMesh4->unsafeDetachFromParentAndDespawn();
                }

                {
                    auto pMesh1 = spawnTransparentMesh();
                    auto pMesh2 = spawnOpaqueMesh();
                    auto pMesh3 = spawnTransparentMesh(true);
                    auto pMesh4 = spawnOpaqueMesh(true);
                    pMesh1->unsafeDetachFromParentAndDespawn();
                    pMesh2->unsafeDetachFromParentAndDespawn();
                    pMesh3->unsafeDetachFromParentAndDespawn();
                    pMesh4->unsafeDetachFromParentAndDespawn();
                }

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
