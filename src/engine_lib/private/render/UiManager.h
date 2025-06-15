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
#include "input/GamepadButton.hpp"

// External.
#include "math/GLMath.hpp"

class Node;
class Renderer;
class ShaderProgram;
class UiNode;
class TextUiNode;
class RectUiNode;
class LayoutUiNode;
class SliderUiNode;

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
     * Called by UI nodes after they are spawned.
     *
     * @param pNode UI node.
     */
    void onNodeSpawning(SliderUiNode* pNode);

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
     * Called by spawned UI nodes after they changed their visibility.
     *
     * @param pNode UI node.
     */
    void onSpawnedNodeChangedVisibility(SliderUiNode* pNode);

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
     * Called by UI nodes before they are despawned.
     *
     * @param pNode UI node.
     */
    void onNodeDespawning(SliderUiNode* pNode);

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
     * Called by game manager when window received gamepad input.
     *
     * @param button         Gamepad button.
     * @param bIsPressedDown Whether the button was pressed or released.
     */
    void onGamepadInput(GamepadButton button, bool bIsPressedDown);

    /**
     * Called by game manager when window received an event about text character being inputted.
     *
     * @param sTextCharacter Character that was typed.
     */
    void onKeyboardInputTextCharacter(const std::string& sTextCharacter);

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
        Data() = default;

        /** Groups various types of spawned and visible UI nodes to render per layer. */
        struct SpawnedVisibleLayerUiNodes {
            /**
             * Returns total number of nodes considered.
             *
             * @return Node count.
             */
            size_t getTotalNodeCount() const {
                return vTextNodes.size() + vRectNodes.size() + vSliderNodes.size() +
                       receivingInputUiNodes.size() + receivingInputUiNodesRenderedLastFrame.size() +
                       layoutNodesWithScrollBars.size();
            }

            /** Node depth - text nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<TextUiNode*>>> vTextNodes;

            /** Node depth - rect nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<RectUiNode*>>> vRectNodes;

            /** Node depth - slider nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<SliderUiNode*>>> vSliderNodes;

            /** Layout nodes from @ref receivingInputUiNodes that need their scroll bar to be rendered. */
            std::unordered_set<LayoutUiNode*> layoutNodesWithScrollBars;

            /** UI nodes that receive input. */
            std::unordered_set<UiNode*> receivingInputUiNodes;

            /**
             * Nodes from @ref receivingInputUiNodes that were rendered (not outside of the screen bounds)
             * last frame.
             */
            std::vector<UiNode*> receivingInputUiNodesRenderedLastFrame;
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
        std::array<SpawnedVisibleLayerUiNodes, static_cast<size_t>(UiLayer::COUNT)> vSpawnedVisibleNodes;

        /** Shader program used for rendering text. */
        std::shared_ptr<ShaderProgram> pTextShaderProgram;

        /** Shader program used for rendering rect UI nodes and text edit's cursor. */
        std::shared_ptr<ShaderProgram> pRectAndCursorShaderProgram;

        /** Quad used for rendering some nodes. */
        std::unique_ptr<ScreenQuadGeometry> pScreenQuadGeometry;
    };

    /** Groups data used to draw a scroll bar. */
    struct ScrollBarDrawInfo {
        /** Position in pixels. */
        glm::vec2 posInPixels = glm::vec2(0.0F, 0.0F);

        /** Width in pixels. */
        float widthInPixels = 0.0F;

        /** Height in pixels. */
        float heightInPixels = 0.0F;

        /** Start offset (from the top) of scroll bar in range [0.0; 1.0] relative to @ref heightInPixels. */
        float verticalPos = 0.0F;

        /** Size of the scroll bar in range [0.0; 1.0] relative to @ref heightInPixels. */
        float verticalSize = 0.0F;

        /** Color of the scroll bar. */
        glm::vec4 color = glm::vec4(0.5F, 0.5F, 0.5F, 0.5F); // NOLINT
    };

    /**
     * Creates a new manager.
     *
     * @param pRenderer Renderer.
     */
    UiManager(Renderer* pRenderer);

    /** Called after the window size changed. */
    void onWindowSizeChanged();

    /**
     * Renders the UI text nodes on the current framebuffer.
     *
     * @param iLayer UI layer to render to.
     */
    void drawTextNodes(size_t iLayer);

    /**
     * Renders the UI rect nodes on the current framebuffer.
     *
     * @param iLayer UI layer to render to.
     */
    void drawRectNodes(size_t iLayer);

    /**
     * Renders the UI slider nodes on the current framebuffer.
     *
     * @param iLayer UI layer to render to.
     */
    void drawSliderNodes(size_t iLayer);

    /**
     * Renders scroll bars for layout UI nodes.
     *
     * @param iLayer UI layer to render to.
     */
    void drawLayoutScrollBars(size_t iLayer);

    /**
     * Renders scroll bars.
     *
     * @warning Expects that @ref mtxData is locked.
     *
     * @param vScrollBarsToDraw Scroll bars to render.
     * @param iWindowHeight     Height of the window.
     */
    void drawScrollBarsDataLocked(
        const std::vector<ScrollBarDrawInfo>& vScrollBarsToDraw, unsigned int iWindowHeight);

    /**
     * Changes node that has focus.
     *
     * @remark Triggers UiNode focus related functions.
     *
     * @param pNode `nullptr` to remove focus.
     */
    void changeFocusedNode(UiNode* pNode);

    /**
     * Draws an quad in screen (window) coordinates.
     *
     * @remark Assumes that @ref mtxData is locked.
     *
     * @param screenPos     Position of the top-left corner of the quad.
     * @param screenSize    Size of the quad.
     * @param yClip         Start Y and end Y in range [0.0; 1.0] relative to size of the quad to cut
     * (to not render).
     * @param iScreenHeight Height of the screen.
     */
    void drawQuad(
        const glm::vec2& screenPos,
        const glm::vec2& screenSize,
        unsigned int iScreenHeight,
        const glm::vec2& yClip = glm::vec2(0.0F, 1.0F)) const;

    /** UI-related data. */
    std::pair<std::recursive_mutex, Data> mtxData;

    /** Orthographic projection matrix for rendering UI elements. */
    glm::mat4 uiProjMatrix;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;

    /** Width of the scroll bar relative to the width of the UI node. */
    static constexpr float scrollBarWidthRelativeNode = 0.025F;
};
