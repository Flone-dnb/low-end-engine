#pragma once

// Custom.
#include "game/GameInstance.h"

class Window;
class EditorCameraNode;
class TextUiNode;
class UiNode;
class NodeTreeInspector;

/** Editor's game instance. */
class EditorGameInstance : public GameInstance {
public:
    /**
     * Creates a new game instance.
     *
     * @param pWindow Do not delete (free). Valid pointer to the game's window.
     */
    EditorGameInstance(Window* pWindow);

    virtual ~EditorGameInstance() override = default;

protected:
    /**
     * Called after GameInstance's constructor is finished and created
     * GameInstance object was saved in GameManager (that owns GameInstance).
     *
     * At this point you can create and interact with the game world and etc.
     */
    virtual void onGameStarted() override;

    /**
     * Called after a gamepad controller was connected.
     *
     * @param sGamepadName Name of the connected gamepad.
     */
    virtual void onGamepadConnected(std::string_view sGamepadName) override;

    /** Called after a gamepad controller was disconnected. */
    virtual void onGamepadDisconnected() override;

    /**
     * Called before a new frame is rendered.
     *
     * @remark Called before nodes that should be called every frame.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    virtual void onBeforeNewFrame(float timeSincePrevCallInSec) override;

    /**
     * Called before a world is destroyed.
     *
     * @param pRootNode Root node of a world about to be destroyed.
     */
    virtual void onBeforeWorldDestroyed(Node* pRootNode) override;

private:
    /** Groups pointers to nodes from game's level. */
    struct GameWorldNodes {
        /** Root node of game's level. */
        Node* pRoot = nullptr;

        /** Camera that allows displaying game's level in the viewport. */
        EditorCameraNode* pViewportCamera = nullptr;

        /** FPS, RAM and other stats. */
        TextUiNode* pStatsText = nullptr;
    };

    /** Groups pointers to nodes from editor's world. */
    struct EditorWorldNodes {
        /** Root node of editor's world. */
        Node* pRoot = nullptr;

        /** Node in the editor's world that occupies space for game's world to be rendered to. */
        UiNode* pViewportUiPlaceholder = nullptr;

        /** Allows viewing and editing game's node tree. */
        NodeTreeInspector* pNodeTreeInspector = nullptr;
    };

    /** Registers action and axis input events in the input manager. */
    void registerEditorInputEvents();

    /**
     * Should be called after created/loaded a new world to add editor-specific nodes.
     *
     * @param pRootNode Root node of the editor's world.
     */
    void attachEditorNodes(Node* pRootNode);

    /**
     * Should be called after game level was loaded.
     *
     * @param pRootNode Root node of game's node tree.
     */
    void onAfterGameWorldCreated(Node* pRootNode);

    /** Not `nullptr` if game world exists. */
    GameWorldNodes gameWorldNodes;

    /** Not `nullptr` if editor world exists. */
    EditorWorldNodes editorWorldNodes;
};
