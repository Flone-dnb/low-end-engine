#pragma once

// Standard.
#include <mutex>

// Custom.
#include "math/GLMath.hpp"
#include "game/node/SpatialNode.h"
#include "game/geometry/MeshGeometry.h"
#include "render/Material.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/ShaderConstantsSetter.hpp"

class MeshNode;

/** Represents a node that can have 3D geometry to display (mesh). */
class MeshNode : public SpatialNode {
    // Guard is allowed to modify geometry.
    friend class MeshGeometryGuard;

public:
    MeshNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    MeshNode(const std::string& sNodeName);

    virtual ~MeshNode() override = default;

    /**
     * Returns path to .glsl file relative `res` directory that's used as a default vertex shader for mesh
     * nodes.
     *
     * @return Path to .glsl file relative `res` directory.
     */
    static inline std::string_view getDefaultVertexShaderForMeshNode() {
        return "engine/shaders/node/MeshNode.vert.glsl";
    }

    /**
     * Returns path to .glsl file relative `res` directory that's used as a default fragment shader for mesh
     * nodes.
     *
     * @return Path to .glsl file relative `res` directory.
     */
    static inline std::string_view getDefaultFragmentShaderForMeshNode() {
        return "engine/shaders/node/MeshNode.frag.glsl";
    }

    /**
     * Sets material to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown. To modify
     * material's properties use @ref getMaterial.
     *
     * @param material Material to use.
     */
    void setMaterialBeforeSpawned(const Material& material);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setMeshDataBeforeSpawned(const MeshGeometry& meshGeometry);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setMeshDataBeforeSpawned(MeshGeometry&& meshGeometry);

    /**
     * Sets whether this mesh is visible or not.
     *
     * @param bVisible Whether this mesh is visible or not.
     */
    void setIsVisible(bool bVisible);

    /**
     * Returns material.
     *
     * @remark You can modify this material's properties even while the node is spawned.
     *
     * @return Material.
     */
    Material& getMaterial();

    /**
     * Returns VAO.
     *
     * @warning Shows an error if the node is not spawned.
     *
     * @return VAO.
     */
    VertexArrayObject& getVertexArrayObjectWhileSpawned() {
#if defined(DEBUG)
        if (pVao == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("VAO is invalid on node \"{}\"", getNodeName()));
        }
#endif
        return *pVao.get();
    }

    /**
     * Returns object used to add setter functions for shader `uniform` variables.
     *
     * @warning Shows an error if the node is not spawned.
     *
     * @return Manager.
     */
    ShaderConstantsSetter& getShaderConstantsSetterWhileSpawned() {
#if defined(DEBUG)
        if (!shaderConstantsSetter.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("constants manager is invalid on node \"{}\"", getNodeName()));
        }
#endif
        return *shaderConstantsSetter;
    }

    /**
     * Tells whether this mesh is currently visible or not.
     *
     * @return Whether the mesh is visible or not.
     */
    bool isVisible();

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /**
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     */
    virtual void onWorldLocationRotationScaleChanged() override;

private:
    /** Groups variables that will be set to shader uniforms. */
    struct ShaderConstants {
        /** World matrix. */
        glm::mat4 worldMatrix;

        /** Normal matrix. */
        glm::mat3 normalMatrix;
    };

    /** Must be called after @ref geometry was changed. */
    void onAfterMeshGeometryChanged();

    /** Mesh geometry. */
    MeshGeometry geometry;

    /** Material. */
    Material material;

    /** Not `nullptr` while spawned. */
    std::unique_ptr<VertexArrayObject> pVao;

    /** Only valid while spawned. */
    std::optional<ShaderConstantsSetter> shaderConstantsSetter;

    /** Groups mutex guarded data. */
    std::pair<std::mutex, ShaderConstants> mtxShaderConstants;

    /** Whether mesh is visible or not. */
    std::pair<std::mutex, bool> mtxIsVisible;
};
