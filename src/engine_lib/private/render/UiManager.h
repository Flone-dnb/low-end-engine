#pragma once

// Standard.
#include <unordered_set>
#include <mutex>
#include <memory>

// Custom.
#include "game/geometry/ScreenQuadGeometry.h"
#include "render/UiLayer.hpp"

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

private:
    /** Groups mutex-guarded data. */
    struct Data {
        /** Groups various types of spawned and visible UI nodes to render. */
        struct SpawnedVisibleUiNodes {
            /** Node depth - text nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<TextUiNode*>>> vTextNodes;

            /** Node depth - rect nodes on this depth. */
            std::vector<std::pair<size_t, std::unordered_set<RectUiNode*>>> vRectNodes;

            /**
             * Returns total number of node arrays.
             *
             * @return Node array count.
             */
            size_t getTotalNodeArray() const { return vTextNodes.size() + vRectNodes.size(); }
        };

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
