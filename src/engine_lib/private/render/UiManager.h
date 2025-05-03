#pragma once

// Standard.
#include <unordered_set>
#include <mutex>
#include <array>
#include <memory>

// Custom.
#include "render/wrapper/VertexArrayObject.h"

// External.
#include "math/GLMath.hpp"

class Renderer;
class TextNode;
class ShaderProgram;

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
    void renderUi(unsigned int iDrawFramebufferId);

    /**
     * Called by UI nodes after they are spawned.
     *
     * @param pNode UI node.
     */
    void onNodeSpawning(TextNode* pNode);

    /**
     * Called by spawned UI nodes after they changed their visibility.
     *
     * @param pNode UI node.
     */
    void onSpawnedNodeChangedVisibility(TextNode* pNode);

    /**
     * Called by UI nodes before they are despawned.
     *
     * @param pNode UI node.
     */
    void onNodeDespawning(TextNode* pNode);

private:
    /** Types of UI shaders. */
    enum class UiShaderType : unsigned char {
        TEXT = 0,
        // ... new types go here ...

        COUNT, //< marks the size of this enum
    };

    /** Groups mutex-guarded data. */
    struct Data {
        /**
         * All spawned and visible text nodes.
         *
         * @remark It's safe to store raw pointers here because node will notify this manager when
         * the node is becoming invisible or despawning.
         */
        std::unordered_set<TextNode*> spawnedVisibleTextNodes;

        /** Loaded UI shaders (not `nullptr` if loaded). */
        std::array<std::shared_ptr<ShaderProgram>, static_cast<size_t>(UiShaderType::COUNT)> vLoadedShaders;

        /** Quad used for rendering text. */
        std::unique_ptr<VertexArrayObject> pQuadVaoForText;
    };

    /**
     * Creates a new manager.
     *
     * @param pRenderer Renderer.
     */
    UiManager(Renderer* pRenderer);

    /** UI-related data. */
    std::pair<std::mutex, Data> mtxData;

    /** Orthographic projection matrix for rendering text. */
    glm::mat4 projectionMatrix;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;

    /** Number of vertices used for rendering a single glyph of text. */
    static constexpr int iTextQuadVertexCount = 6; // NOLINT
};
