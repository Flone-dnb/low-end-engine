// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/GameInstance.h"
#include "game/Window.h"
#include "TestFilePaths.hpp"

// External.
#include "catch2/catch_test_macros.hpp"

TEST_CASE("serialize and deserialize layout UI node with child nodes (child node order should be saved)") {
    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual ~TestGameInstance() override = default;

        virtual void onGameStarted() override {
            createWorld([&](Node* pRootNode) {
                const auto pathToFile = ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) /
                                        sTestDirName / vUsedTestFileNames[10];

                {
                    auto pLayout = std::make_unique<LayoutUiNode>();
                    const auto pText1 = pLayout->addChildNode(std::make_unique<TextUiNode>());
                    const auto pText2 = pLayout->addChildNode(std::make_unique<TextUiNode>());

                    const auto pChildLayout = pLayout->addChildNode(std::make_unique<LayoutUiNode>());
                    const auto pChildText = pChildLayout->addChildNode(std::make_unique<TextUiNode>());

                    const auto pText3 = pLayout->addChildNode(std::make_unique<TextUiNode>());

                    pText1->setText(u"text1");
                    pText2->setText(u"text2");
                    pText3->setText(u"text3");
                    pChildText->setText(u"child text");

                    const auto mtxChildNodes = pLayout->getChildNodes();
                    REQUIRE(mtxChildNodes.second.size() == 4);
                    REQUIRE(dynamic_cast<TextUiNode*>(mtxChildNodes.second[0])->getText() == u"text1");
                    REQUIRE(dynamic_cast<TextUiNode*>(mtxChildNodes.second[1])->getText() == u"text2");
                    REQUIRE(dynamic_cast<LayoutUiNode*>(mtxChildNodes.second[2]) != nullptr);
                    REQUIRE(dynamic_cast<TextUiNode*>(mtxChildNodes.second[3])->getText() == u"text3");

                    // Serialize.
                    auto optionalError = pLayout->serializeNodeTree(pathToFile, false);
                    if (optionalError.has_value()) [[unlikely]] {
                        optionalError->addCurrentLocationToErrorStack();
                        INFO(optionalError->getFullErrorMessage());
                        REQUIRE(false);
                    }
                }

                // Deserialize.
                auto result = Node::deserializeNodeTree(pathToFile);
                if (std::holds_alternative<Error>(result)) [[unlikely]] {
                    auto error = std::get<Error>(std::move(result));
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
                auto pDeserializedNode = std::get<std::unique_ptr<Node>>(std::move(result));

                const auto pDeserializedLayout = dynamic_cast<LayoutUiNode*>(pDeserializedNode.get());
                REQUIRE(pDeserializedLayout != nullptr);

                const auto mtxChildNodes = pDeserializedLayout->getChildNodes();
                REQUIRE(mtxChildNodes.second.size() == 4);

                // We must guarantee that after deserialization child order is the same.
                const auto pText1 = dynamic_cast<TextUiNode*>(mtxChildNodes.second[0]);
                const auto pText2 = dynamic_cast<TextUiNode*>(mtxChildNodes.second[1]);
                const auto pChildLayout = dynamic_cast<LayoutUiNode*>(mtxChildNodes.second[2]);
                const auto pText3 = dynamic_cast<TextUiNode*>(mtxChildNodes.second[3]);

                REQUIRE(pText1 != nullptr);
                REQUIRE(pText2 != nullptr);
                REQUIRE(pText3 != nullptr);
                REQUIRE(pChildLayout != nullptr);

                REQUIRE(pText1->getText() == u"text1");
                REQUIRE(pText2->getText() == u"text2");
                REQUIRE(pText3->getText() == u"text3");

                const auto mtxChildLayoutNodes = pChildLayout->getChildNodes();
                REQUIRE(mtxChildLayoutNodes.second.size() == 1);

                const auto pChildText = dynamic_cast<TextUiNode*>(mtxChildLayoutNodes.second[0]);
                REQUIRE(pChildText != nullptr);
                REQUIRE(pChildText->getText() == u"child text");

                getWindow()->close();
            });
        }
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
