#pragma once

// Standard.
#include <optional>
#include <filesystem>
#include <memory>

// Custom.
#include "game/GameInstance.h"
#include "node/GizmoMode.hpp"
#include "math/GLMath.hpp"

class Window;
class EditorCameraNode;
class TextUiNode;
class UiNode;
class NodeTreeInspector;
class ContextMenuNode;
class ContentBrowser;
class PropertyInspector;
class ShaderProgram;
class Buffer;
class Texture;
class GizmoNode;
class CameraManager;
class SpatialNode;

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
     * Creates gizmo to control the specified node.
     *
     * @param pNode Node to control.
     * @param mode  Gizmo mode.
     */
    void showGizmoToControlNode(SpatialNode* pNode, GizmoMode mode = GizmoMode::MOVE);

    /**
     * Tells if @ref openContextMenu was called and the menu is still opened.
     *
     * @return Context menu state.
     */
    bool isContextMenuOpened() const;

    /**
     * Returns gizmo if it's visible.
     *
     * @return `nullptr` if the gizmo is not active.
     */
    GizmoNode* getGizmoNode();

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
     * Called when the window (that owns this object) receives mouse input.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void onMouseButtonPressed(MouseButton button, KeyboardModifiers modifiers) override;

    /**
     * Called when the window (that owns this object) receives mouse input.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     */
    virtual void onMouseButtonReleased(MouseButton button, KeyboardModifiers modifiers) override;

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

    /** Called by window after its size changed. */
    virtual void onWindowSizeChanged() override;

    /**
     * Called when the window that owns this game instance
     * was requested to close (no new frames will be rendered).
     *
     * Prefer to have your destructor logic here, because after this function is finished
     * the world will be destroyed and will be inaccessible (`nullptr`).
     */
    virtual void onWindowClose() override;

    /** Called after the renderer finished submitting draw commands to render meshes. */
    virtual void onFinishedSubmittingMeshDrawCommands() override;

private:
    /** Groups pointers to nodes from game's level. */
    struct GameWorldNodes {
        /** Root node of game's level. */
        Node* pRoot = nullptr;

        /** Camera that allows displaying game's level in the viewport. */
        EditorCameraNode* pViewportCamera = nullptr;

        /** FPS, RAM and other stats. */
        TextUiNode* pStatsText = nullptr;

        /** Not `nullptr` if a gizmo is shown. */
        GizmoNode* pGizmoNode = nullptr;
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

    /** Groups data used for GPU picking. */
    struct GpuPickingData {
        GpuPickingData() = default;
        ~GpuPickingData();

        /**
         * Recreates @ref pNodeIdTexture with the size of the main framebuffer.
         *
         * @param pViewportCamera Viewport camera.
         * @param bIsCameraRecreated Specify `true` if the camera node was (re)created just now.
         */
        void recreateNodeIdTexture(CameraNode* pViewportCamera, bool bIsCameraRecreated);

        /** Compute shader used to check node ID value under the mouse cursor. */
        std::shared_ptr<ShaderProgram> pPickingProgram;

        /** Compute shader used to clear node ID texture. */
        std::shared_ptr<ShaderProgram> pClearTextureProgram;

        /** Buffer that stores a single unsigned int - node ID value under the mouse cursor. */
        std::unique_ptr<Buffer> pClickedNodeIdValueBuffer;

        /** Texture that stores node IDs of all rendered objects. */
        std::unique_ptr<Texture> pNodeIdTexture;

        /** `true` if left mouse button was clicked in the game's viewport on this tick. */
        bool bMouseClickedThisTick = false;

        /** `false` if left mouse button was pressed but button was not released yet. */
        bool bLeftMouseButtonReleased = true;

        /** `true` if @ref pPickingProgram was started and we expect a result soon. */
        bool bIsWaitingForGpuResult = false;
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

    /**
     * Updates FPS and other stats.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    void updateFrameStatsText(float timeSincePrevCallInSec);

    /** Processes results of the clicked object. */
    void processGpuPickingResult();

    /** Not `nullptr` if game world exists. */
    GameWorldNodes gameWorldNodes;

    /** Not `nullptr` if editor world exists. */
    EditorWorldNodes editorWorldNodes;

    /** Path to the last opened node tree file. */
    std::optional<std::filesystem::path> lastOpenedNodeTree;

    /** Data for GPU-picking. */
    GpuPickingData gpuPickingData;

    /** Ambient light to use for worlds in the editor. */
    glm::vec3 editorAmbientLight = glm::vec3(0.25F, 0.25F, 0.25F);
};
