#pragma once

// Standard.
#include <mutex>
#include <array>
#include <unordered_map>
#include <unordered_set>

// Custom.
#include "render/MeshDrawLayer.hpp"

class ShaderProgram;
class MeshNode;
class CameraProperties;
class LightSourceManager;
class Renderer;

/** Keeps track of spawned 3D nodes and handles mesh rendering. */
class MeshNodeManager {
    // Only world is expected to create this manager.
    friend class World;

public:
    /** Spawned and visible mesh nodes of a world. */
    struct SpawnedVisibleNodes {
        /**
         * Meshes with opaque material.
         *
         * @remark It's safe to store raw pointers here because nodes will notify the manager before they
         * despawn.
         */
        std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>> opaqueMeshes;

        /** Meshes with transparent material. */
        std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>> transparentMeshes;
    };

    ~MeshNodeManager();

    /**
     * Queues OpenGL draw commands to draw all spawned and visible meshes
     * on the currently set framebuffer.
     *
     * @param pRenderer          Renderer.
     * @param pCameraProperties  Camera properties.
     * @param lightSourceManager Light source manager.
     */
    void drawMeshes(Renderer* pRenderer, CameraProperties* pCameraProperties, LightSourceManager& lightSourceManager);

    /**
     * Called by mesh nodes during their spawn.
     *
     * @param pNode Mesh node.
     */
    void onMeshNodeSpawning(MeshNode* pNode);

    /**
     * Called by mesh nodes during their despawn.
     *
     * @param pNode Mesh node.
     */
    void onMeshNodeDespawning(MeshNode* pNode);

    /**
     * Called by mesh nodes after they change their visibility.
     *
     * @param pNode          Mesh node.
     * @param bNewVisibility New visibility state.
     */
    void onSpawnedMeshNodeChangingVisibility(MeshNode* pNode, bool bNewVisibility);

private:
    MeshNodeManager() = default;

    /**
     * Queues OpenGL draw commands to draw the specified meshes on the currently set framebuffer.
     *
     * @param meshes             Meshes to draw.
     * @param pCameraProperties  Camera properties.
     * @param lightSourceManager Light source manager.
     */
    static void drawMeshes(
        const std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>>& meshes,
        CameraProperties* pCameraProperties,
        LightSourceManager& lightSourceManager);

    /**
     * Adds mesh node to @ref mtxSpawnedVisibleNodes.
     *
     * @param pNode Mesh node.
     */
    void addMeshNodeToRendering(MeshNode* pNode);

    /**
     * Removes mesh node from @ref mtxSpawnedVisibleNodes.
     *
     * @param pNode Mesh node.
     */
    void removeMeshNodeFromRendering(MeshNode* pNode);

    /** Groups info about spawned and visible mesh nodes (per draw layer). */
    std::pair<std::mutex, std::array<SpawnedVisibleNodes, static_cast<size_t>(MeshDrawLayer::COUNT)>>
        mtxSpawnedVisibleNodes;
};
