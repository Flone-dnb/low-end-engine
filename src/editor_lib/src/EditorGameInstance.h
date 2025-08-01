﻿#pragma once

// Standard.
#include <optional>
#include <filesystem>

// Custom.
#include "game/GameInstance.h"

class Window;
class EditorCameraNode;
class TextUiNode;
class UiNode;
class NodeTreeInspector;
class ContextMenuNode;
class ContentBrowser;
class PropertyInspector;

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

    /**
     * Shows context menu at the current position of the mouse cursor.
     *
     * @remark Menu will be automatically closed when a menu item is clicked or if mouse is no longer hovering
     * over the context menu.
     *
     * @param vMenuItems Names and callbacks for menu items.
     * @param sTitle     Optional title of the menu to show.
     */
    void openContextMenu(
        const std::vector<std::pair<std::u16string, std::function<void()>>>& vMenuItems,
        const std::u16string& sTitle = u"");

    /**
     * Loads the specified file as a new game world.
     *
     * @param pathToNodeTree Path to node tree file.
     */
    void openNodeTreeAsGameWorld(const std::filesystem::path& pathToNodeTree);

    /**
     * Recreates game world by changing the root node. Additionally spawns editor-specific node's in game's
     * world.
     *
     * @param pNewGameRootNode New root node of the game world.
     */
    void changeGameWorldRootNode(std::unique_ptr<Node> pNewGameRootNode);

    /**
     * Can be used to disable game world from rendering and overwriting UI that is located in the editor's
     * viewport area.
     *
     * @param bEnable Specify `false` so that game world will not overwrite pixels located in the editor's
     * viewport.
     */
    void setEnableViewportCamera(bool bEnable);

    /**
     * Tells if @ref openContextMenu was called and the menu is still opened.
     *
     * @return Context menu state.
     */
    bool isContextMenuOpened() const;

    /**
     * Returns property inspector that displays reflected fields.
     *
     * @return Inspector.
     */
    PropertyInspector* getPropertyInspector() const;

    /**
     * Returns node tree inspector that displays game world's node tree.
     *
     * @return Inspector.
     */
    NodeTreeInspector* getNodeTreeInspector() const;

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
     * Called when the window (that owns this object) receives keyboard input.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void onKeyboardButtonReleased(KeyboardButton key, KeyboardModifiers modifiers) override;

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

    /**
     * Called when the window focus was changed.
     *
     * @param bIsFocused  Whether the window has gained or lost the focus.
     */
    virtual void onWindowFocusChanged(bool bIsFocused) override;

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

        /** Displays reflected fields of a type. */
        PropertyInspector* pPropertyInspector = nullptr;

        /** Node used as context menu. */
        ContextMenuNode* pContextMenu = nullptr;

        /** Displays filesystem. */
        ContentBrowser* pContentBrowser = nullptr;
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

    /** Creates an empty game world. */
    void createGameWorld();

    /** Not `nullptr` if game world exists. */
    GameWorldNodes gameWorldNodes;

    /** Not `nullptr` if editor world exists. */
    EditorWorldNodes editorWorldNodes;

    /** Path to the last opened node tree file. */
    std::optional<std::filesystem::path> lastOpenedNodeTree;
};
