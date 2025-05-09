#pragma once

// Standard.
#include <unordered_set>
#include <mutex>
#include <memory>

// Custom.
#include "game/geometry/ScreenQuadGeometry.h"
#include "render/UiLayer.hpp"
#include "input/KeyboardButton.hpp"
#include "input/MouseButton.hpp"

// External.
#include "math/GLMath.hpp"

class Node;
class Renderer;
class ShaderProgram;
class UiNode;
class TextUiNode;
class RectUiNode;

/** Keeps track of spawned UI nodes and handles UI rendering. */
class UiManager {
    // Only renderer is expected to create objects of this type.
    friend class Renderer;

public:
    UiManager() = delete;
    ~UiManager();

    /**
     * Renders the UI on the specified framebuffer.
     *
     * @param iDrawFramebufferId Framebuffer to draw to.
     */
    void drawUi(unsigned int iDrawFramebufferId);

    /**
     * Called by UI nodes after they are spawned.
     *
     * @param pNode UI node.
     */
    void onNodeSpawning(TextUiNode* pNode);

    /**
     * Called by UI nodes after they are spawned.
     *
     * @param pNode UI node.
     */
    void onNodeSpawning(RectUiNode* pNode);

    /**
     * Called by spawned UI nodes after they changed their visibility.
     *
     * @param pNode UI node.
     */
    void onSpawnedNodeChangedVisibility(TextUiNode* pNode);

    /**
     * Called by spawned UI nodes after they changed their visibility.
     *
     * @param pNode UI node.
     */
    void onSpawnedNodeChangedVisibility(RectUiNode* pNode);

    /**
     * Called by UI nodes before they are despawned.
     *
     * @param pNode UI node.
     */
    void onNodeDespawning(TextUiNode* pNode);

    /**
     * Called by UI nodes before they are despawned.
     *
     * @param pNode UI node.
     */
    void onNodeDespawning(RectUiNode* pNode);

    /**
     * Called by UI nodes after their depth (in the node tree) was changed.
     *
     * @param pTargetNode UI node.
     */
    void onNodeChangedDepth(UiNode* pTargetNode);

    /**
     * Makes the specified UI node (tree) a modal UI node (tree) that takes all input to itself.
     *
     * @remark Replaces old modal node (tree).
     * @remark Automatically becomes non-modal when a node gets despawned, becomes invisible or disables
     * input.
     *
     * @param pNewModalNode `nullptr` to remove modal node, otherwise node to make modal.
     */
    void setModalNode(UiNode* pNewModalNode);

    /**
     * Sets node that will have focus to receive keyboard/gamepad input.
     *
     * @param pFocusedNode Node.
     */
    void setFocusedNode(UiNode* pFocusedNode);

    /**
     * Called by UI nodes to notify about a UI node that receives input being spawned/despawned
     * or if a UI node enabled/disable input while spawned.
     *
     * @param pNode         UI node.
     * @param bEnabledInput `true` if the node enabled input receiving, `false` otherwise.
     */
    void onSpawnedUiNodeInputStateChange(UiNode* pNode, bool bEnabledInput);

    /**
     * Called by game manager when window received keyboard input.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called by game manager when window received mouse input.
     *
     * @param button         Mouse button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the button down event occurred or button up.
     */
    void onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown);

    /**
     * Called by game manager when window received mouse movement.
     *
     * @param iXOffset Mouse X movement delta in pixels.
     * @param iYOffset Mouse Y movement delta in pixels.
     */
    void onMouseMove(int iXOffset, int iYOffset);

    /**
     * Called when the window received mouse scroll movement.
     *
     * @param iOffset Movement offset. Positive is away from the user, negative otherwise.
     */
    void onMouseScrollMove(int iOffset);

    /**
     * Tells if there is a modal UI node (tree) that should take all input instead of others.
     *
     * @return `true` if modal node (tree) exists.
     */
    bool hasModalUiNodeTree();

private:
    /** Groups mutex-guarded data. */
    struct Data {
        /** Groups various types of spawned and visible UI nodes to render. */
        struct SpawnedVisibleUiNodes {
            /** Node depth - text nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<TextUiNode*>>> vTextNodes;

            /** Node depth - rect nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<RectUiNode*>>> vRectNodes;

            /** UI nodes that receive input. */
            std::unordered_set<UiNode*> receivingInputUiNodes;

            /**
             * Nodes from @ref receivingInputUiNodes that were rendered (not outside of the screen bounds)
             * last frame.
             */
            std::vector<UiNode*> receivingInputUiNodesRenderedLastFrame;

            /**
             * Returns total number of nodes considered.
             *
             * @return Node count.
             */
            size_t getTotalNodeCount() const {
                return vTextNodes.size() + vRectNodes.size() + receivingInputUiNodes.size() +
                       receivingInputUiNodesRenderedLastFrame.size();
            }
        };

        /** UI node that currently has the focus. */
        UiNode* pFocusedNode = nullptr;

        /** UI node that had mouse cursor floating over it last frame. */
        UiNode* pHoveredNodeLastFrame = nullptr;

        /** Empty if no modal node (tree). Nodes that receive input from node (tree) that was made modal. */
        std::unordered_set<UiNode*> modalInputReceivingNodes;

        /** Tells if check for @ref pHoveredNodeLastFrame was done this frame or not. */
        bool bWasHoveredNodeCheckedThisFrame = false;

        /**
         * All spawned and visible UI nodes.
         *
         * @remark It's safe to store raw pointers here because node will notify this manager when
         * the node is becoming invisible or despawning.
         */
        std::array<SpawnedVisibleUiNodes, static_cast<size_t>(UiLayer::COUNT)> vSpawnedVisibleNodes;

        /** Shader program used for rendering text. */
        std::shared_ptr<ShaderProgram> pTextShaderProgram;

        /** Shader program used for rendering rect UI nodes. */
        std::shared_ptr<ShaderProgram> pRectShaderProgram;

        /** Quad used for rendering some nodes. */
        std::unique_ptr<ScreenQuadGeometry> pScreenQuadGeometry;
    };

    /**
     * Creates a new manager.
     *
     * @param pRenderer Renderer.
     */
    UiManager(Renderer* pRenderer);

    /**
     * Renders the UI rect nodes on the current framebuffer.
     *
     * @param iLayer UI layer to render to.
     */
    void drawRectNodes(size_t iLayer);

    /**
     * Renders the UI text nodes on the current framebuffer.
     *
     * @param iLayer UI layer to render to.
     */
    void drawTextNodes(size_t iLayer);

    /** UI-related data. */
    std::pair<std::recursive_mutex, Data> mtxData;

    /** Orthographic projection matrix for rendering UI elements. */
    glm::mat4 uiProjMatrix;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;
};
