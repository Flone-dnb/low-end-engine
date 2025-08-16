#pragma once

// Standard.
#include <mutex>

// Custom.
#include "render/MeshDrawLayer.hpp"
#include "math/GLMath.hpp"
#include "game/node/SpatialNode.h"
#include "game/geometry/MeshGeometry.h"
#include "material/Material.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/ShaderConstantsSetter.hpp"

/** Represents a node that can have 3D geometry to display (mesh). */
class MeshNode : public SpatialNode {
    // Guard is allowed to modify geometry.
    friend class MeshGeometryGuard;

    // Notifies the mesh to re-register itself to rendering if material transparency mode changed.
    friend class Material;

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
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

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
    void setMaterialBeforeSpawned(Material&& material);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setMeshGeometryBeforeSpawned(const MeshGeometry& meshGeometry);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setMeshGeometryBeforeSpawned(MeshGeometry&& meshGeometry);

    /**
     * Sets whether this mesh is visible or not.
     *
     * @param bVisible Whether this mesh is visible or not.
     */
    void setIsVisible(bool bVisible);

    /**
     * Determine the layer in which a mesh is drawn. Meshes of layer 1 will be drawn first, then meshes of
     * layer 2 and so on.
     *
     * @param layer Layer to use.
     */
    void setDrawLayer(MeshDrawLayer layer);

    /**
     * `true` (default) to enable shadows on the mesh according to the light sources and ambient light in the
     * scene.
     *
     * @param bEnable `true` to enable.
     */
    void setEnableSelfShadow(bool bEnable);

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
     * Returns a copy of the mesh's geometry.
     *
     * @return Geometry.
     */
    MeshGeometry copyMeshData() const { return geometry; }

    /**
     * Determine the layer in which a mesh is drawn.
     *
     * @return Layer.
     */
    MeshDrawLayer getDrawLayer() const { return drawLayer; }

    /**
     * Tells whether this mesh is currently visible or not.
     *
     * @return Whether the mesh is visible or not.
     */
    bool isVisible();

    /**
     * Returns `true` to enable shadows on the mesh according to the light sources and ambient light in the
     * scene.
     *
     * @return Self shadow state.
     */
    bool isSelfShadowEnabled() const { return bEnableSelfShadow; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /**
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onWorldLocationRotationScaleChanged() override;

private:
    /** Matrices prepared to set to shaders. */
    struct CachedWorldMatrices {
        /** World matrix. */
        glm::mat4 worldMatrix;

        /** Normal matrix. */
        glm::mat3 normalMatrix;
    };

    /** Must be called after @ref geometry was changed. */
    void onAfterMeshGeometryChanged();

    /** Creates GPU resources and adds this object to be rendered. */
    void registerToRendering();

    /** Removes this object from rendering and destroys GPU resources. */
    void unregisterFromRendering();

    /** Mesh geometry. */
    MeshGeometry geometry;

    /** Material. */
    Material material;

    /** Not `nullptr` while spawned. */
    std::unique_ptr<VertexArrayObject> pVao;

    /** Only valid while spawned. */
    std::optional<ShaderConstantsSetter> shaderConstantsSetter;

    /** Matrices to set to shaders. */
    CachedWorldMatrices cachedWorldMatrices;

    /** Whether mesh is visible or not. */
    std::pair<std::mutex, bool> mtxIsVisible;

    /** Determine the layer in which a mesh is drawn. */
    MeshDrawLayer drawLayer = MeshDrawLayer::LAYER1;

    /** `true` to enable shadows on the mesh according to the light sources and ambient light in the scene. */
    bool bEnableSelfShadow = true;
};
