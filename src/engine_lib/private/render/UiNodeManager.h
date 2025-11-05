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
class TextEditUiNode;
class RectUiNode;
class ProgressBarUiNode;
class LayoutUiNode;
class SliderUiNode;
class CheckboxUiNode;
class World;

/** Keeps track of spawned UI nodes and handles UI rendering. */
class UiNodeManager {
    // Only world is expected to create objects of this type.
    friend class World;

public:
    UiNodeManager() = delete;
    ~UiNodeManager();

    /** Called after the window size changed. */
    void onWindowSizeChanged();

    /**
     * Renders the UI on the specified framebuffer.
     *
     * @param iDrawFramebufferId Framebuffer to draw to.
     */
    void drawUiOnFramebuffer(unsigned int iDrawFramebufferId);

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
    void onNodeSpawning(TextEditUiNode* pNode);

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
     * Called by UI nodes after they are spawned.
     *
     * @param pNode UI node.
     */
    void onNodeSpawning(CheckboxUiNode* pNode);

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
    void onSpawnedNodeChangedVisibility(TextEditUiNode* pNode);

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
     * Called by spawned UI nodes after they changed their visibility.
     *
     * @param pNode UI node.
     */
    void onSpawnedNodeChangedVisibility(CheckboxUiNode* pNode);

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
    void onNodeDespawning(TextEditUiNode* pNode);

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
     * Called by UI nodes before they are despawned.
     *
     * @param pNode UI node.
     */
    void onNodeDespawning(CheckboxUiNode* pNode);

    /**
     * Called by UI nodes after their depth (in the node tree) was changed.
     *
     * @param pTargetNode UI node.
     */
    void onNodeChangedDepth(UiNode* pTargetNode);

    /**
     * Writes the specified text to the clipboard to later paste the text.
     *
     * @param sText Text to copy.
     */
    void writeToClipboard(const std::string& sText);

    /**
     * Returns empty string if nothing in the clipboard or some text that was previously added using @ref
     * writeToClipboard.
     *
     * @return Clipboard text.
     */
    std::string getTextFromClipboard() const;

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
     * @param button         Keyboard button.
     * @param modifiers      Keyboard modifier keys.
     * @param bIsPressedDown Whether the key down event occurred or key up.
     */
    void onKeyboardInput(KeyboardButton button, KeyboardModifiers modifiers, bool bIsPressedDown);

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
     * Called by game manager after mouse cursor changes its visibility.
     *
     * @param bVisibleNow `true` if the cursor is visible now.
     */
    void onCursorVisibilityChanged(bool bVisibleNow);

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

    /**
     * Tells if a focused node exists or not.
     *
     * @return `true` if focused node exists.
     */
    bool hasFocusedNode();

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
                return vTextNodes.size() + vTextEditNodes.size() + vRectNodes.size() +
                       vProgressBarNodes.size() + vSliderNodes.size() + vCheckboxNodes.size() +
                       receivingInputUiNodes.size() + receivingInputUiNodesRenderedLastFrame.size() +
                       layoutNodesWithScrollBars.size();
            }

            /** Node depth - text nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<TextUiNode*>>> vTextNodes;

            /** Node depth - text edit nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<TextEditUiNode*>>> vTextEditNodes;

            /** Node depth - rect nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<RectUiNode*>>> vRectNodes;

            /** Node depth - progress bar nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<ProgressBarUiNode*>>> vProgressBarNodes;

            /** Node depth - slider nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<SliderUiNode*>>> vSliderNodes;

            /** Node depth - checkbox nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<CheckboxUiNode*>>> vCheckboxNodes;

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

        /** Empty if no modal node (tree). Nodes that receive input from node (tree) that was made modal. */
        std::unordered_set<UiNode*> modalInputReceivingNodes;

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
     * @param pWorld    World that created this manager.
     */
    UiNodeManager(Renderer* pRenderer, World* pWorld);

    /** Triggers `onMouseEntered` and `onMouseLeft` events for UI nodes. */
    void processMouseHoverOnNodes();

    /**
     * Renders the UI text nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawTextNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders the UI text edit nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawTextEditNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders the UI rect nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawRectNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders the UI progress bar nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawProgressBarNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders the UI slider nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawSliderNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders the UI checkbox nodes on the current framebuffer.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawCheckboxNodesDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders scroll bars for layout UI nodes.
     *
     * @remark Expects that @ref mtxData is locked.
     *
     * @param iLayer UI layer to render to.
     * @param iWindowWidth Width of the window (in pixels).
     * @param iWindowHeight Height of the window (in pixels).
     */
    void drawLayoutScrollBarsDataLocked(size_t iLayer, unsigned int iWindowWidth, unsigned int iWindowHeight);

    /**
     * Renders scroll bars.
     *
     * @warning Expects that @ref mtxData is locked.
     *
     * @param vScrollBarsToDraw Scroll bars to render.
     * @param iWindowWidth      Width of the window in pixels.
     * @param iWindowHeight     Height of the window in pixels.
     */
    void drawScrollBarsDataLocked(
        const std::vector<ScrollBarDrawInfo>& vScrollBarsToDraw,
        unsigned int iWindowWidth,
        unsigned int iWindowHeight);

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
     * @param iScreenHeight Height of the screen.
     * @param clipRect      Clipping rectangle in range [0.0; 1.0] where XY mark clip start
     * and ZW mark clip size.
     */
    void drawQuad(
        const glm::vec2& screenPos,
        const glm::vec2& screenSize,
        unsigned int iScreenHeight,
        const glm::vec4& clipRect = glm::vec4(0.0F, 0.0F, 1.0F, 1.0F)) const;

    /**
     * Collects all visible child nodes (recursively) that receive input and returns them.
     *
     * @param pParent Node to start searching from.
     * @param inputReceivingNodes Output array of nodes.
     */
    void collectVisibleInputReceivingChildNodes(
        UiNode* pParent, std::unordered_set<UiNode*>& inputReceivingNodes) const;

    /**
     * Tells if the node has a modal node in its parent chain.
     *
     * @param pNode Node to test.
     *
     * @return `true` if there's a modal parent node.
     */
    bool hasModalParent(UiNode* pNode) const;

    /** UI-related data. */
    std::pair<std::recursive_mutex, Data> mtxData;

    /** Orthographic projection matrix for rendering UI elements. */
    glm::mat4 uiProjMatrix;

    /** A single text entry is our clipboard. */
    std::string sClipboard;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;

    /** World that owns this manager. */
    World* const pWorld = nullptr;

    /** Width of the scroll bar relative to the width of the screen. */
    static constexpr float scrollBarWidthRelativeScreen = 0.003F;
};
