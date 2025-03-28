// Custom.
#include "game/node/Node.h"
#include "game/GameInstance.h"
#include "game/Window.h"

// External.
#include "catch2/catch_test_macros.hpp"

TEST_CASE("build and check node hierarchy") {
    {
        // Build hierarchy.
        auto pParentNodeUnique = std::make_unique<Node>();
        auto pChildNodeUnique = std::make_unique<Node>();

        const auto pChildChildNode1 = pChildNodeUnique->addChildNode(std::make_unique<Node>());
        const auto pChildChildNode2 = pChildNodeUnique->addChildNode(std::make_unique<Node>());
        const auto pChildNode = pParentNodeUnique->addChildNode(std::move(pChildNodeUnique));

        // Get child nodes.
        const auto mtxParentChildNodes = pParentNodeUnique->getChildNodes();
        std::scoped_lock parentChildNodesGuard(*mtxParentChildNodes.first);

        const auto mtxChildChildNodes = pChildNode->getChildNodes();
        std::scoped_lock childChildNodesGuard(*mtxChildChildNodes.first);

        // Check that everything is correct.
        REQUIRE(mtxParentChildNodes.second.size() == 1);
        REQUIRE(&*mtxParentChildNodes.second.operator[](0) == &*pChildNode);

        REQUIRE(mtxChildChildNodes.second.size() == 2);
        REQUIRE(&*mtxChildChildNodes.second.operator[](0) == &*pChildChildNode1);
        REQUIRE(&*mtxChildChildNodes.second.operator[](1) == &*pChildChildNode2);

        REQUIRE(pChildNode->getParentNode().second == pParentNodeUnique.get());
        REQUIRE(pChildChildNode1->getParentNode().second == pChildNode);
        REQUIRE(pChildChildNode2->getParentNode().second == pChildNode);

        REQUIRE(pParentNodeUnique->isParentOf(&*pChildNode));
        REQUIRE(pParentNodeUnique->isParentOf(&*pChildChildNode1));
        REQUIRE(pParentNodeUnique->isParentOf(&*pChildChildNode2));

        REQUIRE(pChildNode->isChildOf(pParentNodeUnique.get()));
        REQUIRE(pChildChildNode1->isChildOf(pParentNodeUnique.get()));
        REQUIRE(pChildChildNode1->isChildOf(&*pChildNode));
        REQUIRE(pChildChildNode2->isChildOf(pParentNodeUnique.get()));
        REQUIRE(pChildChildNode2->isChildOf(&*pChildNode));

        REQUIRE(!pChildChildNode1->isChildOf(&*pChildChildNode2));
        REQUIRE(!pChildChildNode1->isParentOf(&*pChildChildNode2));
    }

    REQUIRE(Node::getAliveNodeCount() == 0);
}

TEST_CASE("move nodes in the hierarchy") {
    {
        auto pParentNodeU = std::make_unique<Node>();
        auto pCharacterNodeU = std::make_unique<Node>();
        auto pSomeNodeU = std::make_unique<Node>();
        auto pCarNodeU = std::make_unique<Node>();
        auto pCharacterChildNode1U = std::make_unique<Node>();
        auto pCharacterChildNode2U = std::make_unique<Node>();

        auto pParentNode = pParentNodeU.get();
        auto pCharacterNode = pCharacterNodeU.get();
        auto pCarNode = pCarNodeU.get();
        auto pSomeNode = pSomeNodeU.get();
        auto pCharacterChildNode1 = pCharacterChildNode1U.get();
        auto pCharacterChildNode2 = pCharacterChildNode2U.get();

        // Build hierarchy.
        pCharacterNode->addChildNode(std::move(pCharacterChildNode1U));
        pCharacterNode->addChildNode(std::move(pCharacterChildNode2U));
        pParentNode->addChildNode(std::move(pCharacterNodeU));
        pParentNode->addChildNode(std::move(pCarNodeU));

        // Attach the character to the car.
        pCarNode->addChildNode(pCharacterNode);
        pCarNode->addChildNode(std::move(pSomeNodeU));

        // Check that everything is correct.
        REQUIRE(pCharacterNode->getParentNode().second == pCarNode);
        REQUIRE(pSomeNode->getParentNode().second == pCarNode);
        REQUIRE(pCharacterNode->getChildNodes().second.size() == 2);
        REQUIRE(pCarNode->getChildNodes().second.size() == 2);
        REQUIRE(pCharacterChildNode1->isChildOf(pCharacterNode));
        REQUIRE(pCharacterChildNode2->isChildOf(pCharacterNode));

        // Detach node.
        pSomeNode->unsafeDetachFromParentAndDespawn();
        pSomeNode = nullptr;

        REQUIRE(pCarNode->getChildNodes().second.size() == 1);

        // Detach the character from the car.
        pParentNode->addChildNode(
            pCharacterNode, Node::AttachmentRule::KEEP_RELATIVE, Node::AttachmentRule::KEEP_RELATIVE);

        // Check that everything is correct.
        REQUIRE(pCharacterNode->getParentNode().second == pParentNode);
        REQUIRE(pCharacterNode->getChildNodes().second.size() == 2);
        REQUIRE(pCharacterChildNode1->isChildOf(pCharacterNode));
        REQUIRE(pCharacterChildNode2->isChildOf(pCharacterNode));
    }

    REQUIRE(Node::getAliveNodeCount() == 0);
}

TEST_CASE("get parent node of type") {
    class MyDerivedNode : public Node {
    public:
        MyDerivedNode() = default;
        MyDerivedNode(const std::string& sName) : Node(sName) {}
        virtual ~MyDerivedNode() override = default;
        int iAnswer = 0;
    };

    class MyDerivedDerivedNode : public MyDerivedNode {
    public:
        MyDerivedDerivedNode() = default;
        MyDerivedDerivedNode(const std::string& sName) : MyDerivedNode(sName) {}
        virtual ~MyDerivedDerivedNode() override = default;
        virtual void onSpawning() override {
            MyDerivedNode::onSpawning();

            bSpawnCalled = true;

            // Get parent without name.
            auto pNode = getParentNodeOfType<MyDerivedNode>();
            REQUIRE(pNode == getParentNode().second);
            REQUIRE(pNode->iAnswer == 0);

            // Get parent with name.
            pNode = getParentNodeOfType<MyDerivedNode>("MyDerivedNode");
            REQUIRE(pNode->iAnswer == 42);
        }
        bool bSpawnCalled = false;
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                auto pDerivedNodeChild = std::make_unique<MyDerivedNode>();

                const auto pDerivedDerivedNode =
                    pDerivedNodeChild->addChildNode(std::make_unique<MyDerivedDerivedNode>());

                auto pDerivedNodeParent = std::make_unique<MyDerivedNode>("MyDerivedNode");
                pDerivedNodeParent->iAnswer = 42;

                pDerivedNodeParent->addChildNode(std::move(pDerivedNodeChild));
                getWorldRootNode()->addChildNode(std::move(pDerivedNodeParent));

                REQUIRE(pDerivedDerivedNode->bSpawnCalled);
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

TEST_CASE("get child node of type") {
    class MyDerivedNode : public Node {
    public:
        MyDerivedNode() = default;
        MyDerivedNode(const std::string& sName) : Node(sName) {}
        virtual ~MyDerivedNode() override = default;
        int iAnswer = 0;
    };

    class MyDerivedDerivedNode : public MyDerivedNode {
    public:
        MyDerivedDerivedNode() = default;
        MyDerivedDerivedNode(const std::string& sName) : MyDerivedNode(sName) {}
        virtual ~MyDerivedDerivedNode() override = default;
        virtual void onSpawning() override {
            MyDerivedNode::onSpawning();

            bSpawnCalled = true;

            // Get child without name.
            auto pNode = getChildNodeOfType<MyDerivedNode>();
            REQUIRE(pNode == getChildNodes().second.operator[](0));
            REQUIRE(pNode->iAnswer == 0);

            // Get child with name.
            pNode = getChildNodeOfType<MyDerivedNode>("MyDerivedNode");
            REQUIRE(pNode->iAnswer == 42);
        }
        bool bSpawnCalled = false;
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                auto pDerivedNodeParent = std::make_unique<MyDerivedNode>();

                auto pDerivedNodeChild = std::make_unique<MyDerivedNode>("MyDerivedNode");
                pDerivedNodeChild->iAnswer = 42;

                pDerivedNodeParent->addChildNode(std::move(pDerivedNodeChild));

                auto pDerivedDerivedNodeU = std::make_unique<MyDerivedDerivedNode>();
                auto pDerivedDerivedNode = pDerivedDerivedNodeU.get();

                pDerivedDerivedNodeU->addChildNode(std::move(pDerivedNodeParent));
                getWorldRootNode()->addChildNode(std::move(pDerivedDerivedNodeU));

                REQUIRE(pDerivedDerivedNode->bSpawnCalled);
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

TEST_CASE("onBeforeNewFrame is called only on marked nodes") {
    class MyNode : public Node {
    public:
        MyNode(bool bEnableTick) { setIsCalledEveryFrame(bEnableTick); }

        bool bTickCalled = false;

    protected:
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            Node::onBeforeNewFrame(fTimeSincePrevCallInSec);

            bTickCalled = true;
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                REQUIRE(getWorldRootNode() != nullptr);

                REQUIRE(getCalledEveryFrameNodeCount() == 0);

                auto pNotCalledtNodeU = std::make_unique<MyNode>(false);
                pNotCalledtNode = pNotCalledtNodeU.get();
                getWorldRootNode()->addChildNode(
                    std::move(pNotCalledtNodeU),
                    Node::AttachmentRule::KEEP_RELATIVE,
                    Node::AttachmentRule::KEEP_RELATIVE); // queues deferred task to add to world

                auto pCalledNodeU = std::make_unique<MyNode>(true);
                pCalledNode = pCalledNodeU.get();
                getWorldRootNode()->addChildNode(
                    std::move(pCalledNodeU),
                    Node::AttachmentRule::KEEP_RELATIVE,
                    Node::AttachmentRule::KEEP_RELATIVE); // queues deferred task to add to world
            });
        }
        virtual ~TestGameInstance() override {}

        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            iTicks += 1;

            if (iTicks == 2) {
                REQUIRE(getTotalSpawnedNodeCount() == 3);
                REQUIRE(getCalledEveryFrameNodeCount() == 1);

                REQUIRE(pCalledNode->bTickCalled);
                REQUIRE(!pNotCalledtNode->bTickCalled);

                getWindow()->close();
            }
        }

    private:
        size_t iTicks = 0;
        MyNode* pCalledNode = nullptr;
        MyNode* pNotCalledtNode = nullptr;
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

TEST_CASE("tick groups order is correct") {
    static bool bFirstNodeTicked = false;
    static bool bSecondNodeTicked = false;

    class MyFirstNode : public Node {
    public:
        MyFirstNode() { setIsCalledEveryFrame(true); }

    protected:
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            Node::onBeforeNewFrame(fTimeSincePrevCallInSec);

            REQUIRE(!bFirstNodeTicked);
            REQUIRE(!bSecondNodeTicked);
            bFirstNodeTicked = true;
        }
    };

    class MySecondNode : public Node {
    public:
        MySecondNode() {
            setIsCalledEveryFrame(true);
            setTickGroup(TickGroup::SECOND);
        }

    protected:
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            Node::onBeforeNewFrame(fTimeSincePrevCallInSec);

            REQUIRE(bFirstNodeTicked);
            REQUIRE(!bSecondNodeTicked);
            bSecondNodeTicked = true;

            getGameInstanceWhileSpawned()->getWindow()->close();
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                REQUIRE(getWorldRootNode() != nullptr);

                auto pFirstNodeU = std::make_unique<MyFirstNode>();
                pFirstNode = pFirstNodeU.get();

                auto pSecondNodeU = std::make_unique<MySecondNode>();
                pSecondNode = pSecondNodeU.get();

                getWorldRootNode()->addChildNode(std::move(pFirstNodeU));
                getWorldRootNode()->addChildNode(std::move(pSecondNodeU));
            });
        }
        virtual ~TestGameInstance() override {}

    protected:
        virtual void onWindowClose() override {
            REQUIRE(bFirstNodeTicked);
            REQUIRE(bSecondNodeTicked);
        }

    private:
        MyFirstNode* pFirstNode = nullptr;
        MySecondNode* pSecondNode = nullptr;
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

    bFirstNodeTicked = false;
    bSecondNodeTicked = false;
}

TEST_CASE("input event callbacks in Node are triggered") {
    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false); // disabled by default
            setIsReceivingInput(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }

            {
                auto& mtxAxisEvents = getAxisEventBindings();
                std::scoped_lock guard(mtxAxisEvents.first);

                mtxAxisEvents.second[0] = [&](KeyboardModifiers modifiers, float input) {
                    axis1(modifiers, input);
                };
            }
        }

        bool bAction1Triggered = false;
        bool bAxis1Triggered = false;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { bAction1Triggered = true; }
        void axis1(KeyboardModifiers modifiers, float input) { bAxis1Triggered = true; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register events.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
                optionalError =
                    getInputManager()->addAxisEvent(0, {{KeyboardButton::A, KeyboardButton::B}}, {});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::A, KeyboardModifiers(0), true);
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);

            REQUIRE(pMyNode->bAction1Triggered);
            REQUIRE(pMyNode->bAxis1Triggered);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("detach and despawn spawned node") {
    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([this]() {
                REQUIRE(getTotalSpawnedNodeCount() == 1);

                auto pMyNodeU = std::make_unique<Node>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            iTickCount += 1;

            if (iTickCount == 1) {
                REQUIRE(getTotalSpawnedNodeCount() == 2);

                pMyNode->unsafeDetachFromParentAndDespawn();
                pMyNode = nullptr;

                REQUIRE(getTotalSpawnedNodeCount() == 1);
                REQUIRE(Node::getAliveNodeCount() == 1);

                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        size_t iTickCount = 0;
        Node* pMyNode = nullptr;
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

TEST_CASE("input event callbacks and tick in Node is not triggered after despawning") {
    class MyNode : public Node {
    public:
        MyNode() {
            setIsReceivingInput(true);
            setIsCalledEveryFrame(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }

            {
                auto& mtxAxisEvents = getAxisEventBindings();
                std::scoped_lock guard(mtxAxisEvents.first);

                mtxAxisEvents.second[0] = [&](KeyboardModifiers modifiers, float input) {
                    axis1(modifiers, input);
                };
            }
        }

        bool bAction1Triggered = false;
        bool bAxis1Triggered = false;
        size_t iTickCalledCount = 0;

    protected:
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            Node::onBeforeNewFrame(fTimeSincePrevCallInSec);

            iTickCalledCount += 1;
        }

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { bAction1Triggered = true; }
        void axis1(KeyboardModifiers modifiers, float input) { bAxis1Triggered = true; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register events.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
                optionalError =
                    getInputManager()->addAxisEvent(0, {{KeyboardButton::A, KeyboardButton::B}}, {});
                if (optionalError.has_value()) {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            iTickCount += 1;

            if (iTickCount == 1) {
                // Simulate input.
                getWindow()->onKeyboardInput(KeyboardButton::A, KeyboardModifiers(0), true);
                getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);

                REQUIRE(pMyNode->bAction1Triggered);
                REQUIRE(pMyNode->bAxis1Triggered);

                REQUIRE(getTotalSpawnedNodeCount() == 2);
                REQUIRE(getReceivingInputNodeCount() == 1);

                // GameInstance is ticking before nodes.
                REQUIRE(pMyNode->iTickCalledCount == 0);
            } else if (iTickCount == 2) {
                REQUIRE(pMyNode->iTickCalledCount == 1);

                pMyNode->unsafeDetachFromParentAndDespawn();
                pMyNode = nullptr;

                REQUIRE(getTotalSpawnedNodeCount() == 1);
                REQUIRE(Node::getAliveNodeCount() == 1);
                REQUIRE(getReceivingInputNodeCount() == 0);

                getWindow()->onKeyboardInput(KeyboardButton::A, KeyboardModifiers(0), true);
                getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
                REQUIRE(getReceivingInputNodeCount() == 0);
            } else if (iTickCount == 3) {
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        size_t iTickCount = 0;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("disable \"is called every frame\" in onBeforeNewFrame") {
    class MyNode : public Node {
    public:
        MyNode() { setIsCalledEveryFrame(true); }

        size_t iTickCallCount = 0;

    protected:
        virtual void onBeforeNewFrame(float delta) override {
            iTickCallCount += 1;

            setIsCalledEveryFrame(false);
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (pMyNode->iTickCallCount == 1) {
                // Node ticked once and disabled it's ticking, wait a few frames to see that node's tick
                // will not be called.
                bWait = true;
            }

            if (!bWait) {
                return;
            }

            iFramesPassed += 1;
            if (iFramesPassed >= iFramesToWait) {
                REQUIRE(pMyNode->iTickCallCount == 1);
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        bool bWait = false;
        size_t iFramesPassed = 0;
        const size_t iFramesToWait = 10;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("disable \"is called every frame\" in onBeforeNewFrame and despawn") {
    static size_t iTickCallCount = 0;

    class MyNode : public Node {
    public:
        MyNode() { setIsCalledEveryFrame(true); }

    protected:
        virtual void onBeforeNewFrame(float delta) override {
            iTickCallCount += 1;

            setIsCalledEveryFrame(false);

            unsafeDetachFromParentAndDespawn();
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();

                REQUIRE(getCalledEveryFrameNodeCount() == 0);

                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                REQUIRE(getCalledEveryFrameNodeCount() == 1);
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (iTickCallCount == 0) {
                // Game instance is ticking before nodes.
                REQUIRE(getCalledEveryFrameNodeCount() == 1);
            } else if (iTickCallCount == 1) {
                REQUIRE(getCalledEveryFrameNodeCount() == 0);
                bWait = true;
            }

            if (!bWait) {
                return;
            }

            iFramesPassed += 1;
            if (iFramesPassed >= iFramesToWait) {
                REQUIRE(iTickCallCount == 1);
                REQUIRE(getCalledEveryFrameNodeCount() == 0);
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        bool bWait = false;
        size_t iFramesPassed = 0;
        const size_t iFramesToWait = 10;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("quickly enable and disable \"is called every frame\" while spawned") {
    class MyNode : public Node {
    public:
        size_t iTickCallCount = 0;

        void test() {
            REQUIRE(isCalledEveryFrame() == false);
            setIsCalledEveryFrame(true);
            setIsCalledEveryFrame(false);
        }

    protected:
        virtual void onBeforeNewFrame(float delta) override {
            REQUIRE(false);
            iTickCallCount += 1;
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (!bWait) {
                pMyNode->test();
                bWait = true;
                REQUIRE(pMyNode->iTickCallCount == 0);
                return;
            }

            iFramesPassed += 1;
            if (iFramesPassed >= iFramesToWait) {
                REQUIRE(pMyNode->iTickCallCount == 0);
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        bool bWait = false;
        size_t iFramesPassed = 0;
        const size_t iFramesToWait = 10;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("quickly enable, disable and enable \"is called every frame\" while spawned") {
    class MyNode : public Node {
    public:
        size_t iTickCallCount = 0;

        void test() {
            REQUIRE(isCalledEveryFrame() == false);
            setIsCalledEveryFrame(true);
            setIsCalledEveryFrame(false);
            setIsCalledEveryFrame(true);
        }

    protected:
        virtual void onBeforeNewFrame(float delta) override { iTickCallCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (!bWait) {
                pMyNode->test();
                bWait = true;
                REQUIRE(pMyNode->iTickCallCount == 0);
                return;
            }

            iFramesPassed += 1;
            if (iFramesPassed >= iFramesToWait) {
                REQUIRE(pMyNode->iTickCallCount > 0);
                REQUIRE(pMyNode->isCalledEveryFrame());
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        bool bWait = false;
        size_t iFramesPassed = 0;
        const size_t iFramesToWait = 10;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("enable \"is called every frame\" while spawned and despawn") {
    static size_t iTickCallCount = 0;

    class MyNode : public Node {
    public:
        void test() {
            REQUIRE(isCalledEveryFrame() == false);
            setIsCalledEveryFrame(true);
            unsafeDetachFromParentAndDespawn();
        }

    protected:
        virtual void onBeforeNewFrame(float delta) override {
            REQUIRE(false);
            iTickCallCount += 1;
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (!bWait) {
                pMyNode->test();
                bWait = true;
                REQUIRE(iTickCallCount == 0);
                return;
            }

            iFramesPassed += 1;
            if (iFramesPassed >= iFramesToWait) {
                REQUIRE(iTickCallCount == 0);
                getWindow()->close();
            }
        }
        virtual ~TestGameInstance() override {}

    private:
        bool bWait = false;
        size_t iFramesPassed = 0;
        const size_t iFramesToWait = 10;
        MyNode* pMyNode = nullptr;
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

TEST_CASE("disable receiving input while processing input") {
    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false); // disabled by default
            setIsReceivingInput(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        size_t iAction1TriggerCount = 0;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) {
            iAction1TriggerCount += 1;

            setIsReceivingInput(false);
        }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            if (!bInitialTriggerFinished) {
                // Simulate input.
                getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
                REQUIRE(pMyNode->iAction1TriggerCount == 1);

                // node should disable its input processing now using a deferred task
                // wait 1 frame
                bInitialTriggerFinished = true;
                return;
            }

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(pMyNode->iAction1TriggerCount == 1);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
        bool bInitialTriggerFinished = false;
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

TEST_CASE("disable receiving input and despawn") {
    static size_t iAction1TriggerCount = 0;

    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false); // disabled by default
            setIsReceivingInput(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        void test() {
            setIsReceivingInput(false);
            unsafeDetachFromParentAndDespawn();
        }

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
            REQUIRE(iAction1TriggerCount == 1);

            pMyNode->test();

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(iAction1TriggerCount == 1);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("enable receiving input and despawn") {
    static size_t iAction1TriggerCount = 0;

    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false); // disabled by default

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        void test() {
            setIsReceivingInput(true);
            unsafeDetachFromParentAndDespawn();
        }

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
            REQUIRE(iAction1TriggerCount == 0);

            pMyNode->test();

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(iAction1TriggerCount == 0);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("enable receiving input while spawned") {
    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        void test() {
            REQUIRE(isReceivingInput() == false);
            setIsReceivingInput(true);
        }

        size_t iAction1TriggerCount = 0;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
            REQUIRE(pMyNode->iAction1TriggerCount == 0);

            pMyNode->test();

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(pMyNode->iAction1TriggerCount == 1);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("quickly enable receiving input and disable while spawned") {
    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        void test() {
            REQUIRE(isReceivingInput() == false);
            setIsReceivingInput(true);
            setIsReceivingInput(false);
        }

        size_t iAction1TriggerCount = 0;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
            REQUIRE(pMyNode->iAction1TriggerCount == 0);

            pMyNode->test();

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(pMyNode->iAction1TriggerCount == 0);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("quickly disable receiving input and enable while spawned") {
    class MyNode : public Node {
    public:
        MyNode() {
            REQUIRE(isReceivingInput() == false); // disabled by default
            setIsReceivingInput(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }
        }

        void test() {
            REQUIRE(isReceivingInput() == true);
            setIsReceivingInput(false);
            setIsReceivingInput(true);
        }

        size_t iAction1TriggerCount = 0;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register event.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }
        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate input.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);
            REQUIRE(pMyNode->iAction1TriggerCount == 1);

            pMyNode->test();

            // Simulate input again.
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);
            REQUIRE(pMyNode->iAction1TriggerCount == 2);

            getWindow()->close();
        }
        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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

TEST_CASE("input event callbacks are only triggered when input changed") {
    class MyNode : public Node {
    public:
        MyNode() {
            setIsReceivingInput(true);

            {
                auto& mtxActionEvents = getActionEventBindings();
                std::scoped_lock guard(mtxActionEvents.first);

                mtxActionEvents.second[0] = [&](KeyboardModifiers modifiers, bool bIsPressedDown) {
                    action1(modifiers, bIsPressedDown);
                };
            }

            {
                auto& mtxAxisEvents = getAxisEventBindings();
                std::scoped_lock guard(mtxAxisEvents.first);

                mtxAxisEvents.second[0] = [&](KeyboardModifiers modifiers, float input) {
                    axis1(modifiers, input);
                };
            }
        }

        size_t iAction1TriggerCount = 0;
        size_t iAxis1TriggerCount = 0;

    private:
        void action1(KeyboardModifiers modifiers, bool bIsPressedDown) { iAction1TriggerCount += 1; }
        void axis1(KeyboardModifiers modifiers, float input) { iAxis1TriggerCount += 1; }
    };

    class TestGameInstance : public GameInstance {
    public:
        TestGameInstance(Window* pWindow) : GameInstance(pWindow) {}
        virtual void onGameStarted() override {
            createWorld([&]() {
                // Spawn node.
                auto pMyNodeU = std::make_unique<MyNode>();
                pMyNode = pMyNodeU.get();
                getWorldRootNode()->addChildNode(std::move(pMyNodeU));

                // Register events.
                auto optionalError = getInputManager()->addActionEvent(0, {KeyboardButton::W});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
                optionalError =
                    getInputManager()->addAxisEvent(0, {{KeyboardButton::A, KeyboardButton::D}}, {});
                if (optionalError.has_value()) [[unlikely]] {
                    auto error = optionalError.value();
                    error.addCurrentLocationToErrorStack();
                    INFO(error.getFullErrorMessage());
                    REQUIRE(false);
                }
            });
        }

        virtual void onBeforeNewFrame(float fTimeSincePrevCallInSec) override {
            // Simulate "pressed" input.
            getWindow()->onKeyboardInput(KeyboardButton::A, KeyboardModifiers(0), true);
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);

            REQUIRE(pMyNode->iAction1TriggerCount == 1);
            REQUIRE(pMyNode->iAxis1TriggerCount == 1);

            // Simulate the same "pressed" input again.
            getWindow()->onKeyboardInput(KeyboardButton::A, KeyboardModifiers(0), true);
            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), true);

            // Input callbacks should not be triggered since the input is the same as the last one.
            REQUIRE(pMyNode->iAction1TriggerCount == 1);
            REQUIRE(pMyNode->iAxis1TriggerCount == 2); // axis events are "floating" and can't compare states

            getWindow()->onKeyboardInput(KeyboardButton::W, KeyboardModifiers(0), false);

            // Input differs from the last one.
            REQUIRE(pMyNode->iAction1TriggerCount == 2);
            REQUIRE(pMyNode->iAxis1TriggerCount == 2);

            getWindow()->close();
        }

        virtual ~TestGameInstance() override {}

    private:
        MyNode* pMyNode = nullptr;
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
