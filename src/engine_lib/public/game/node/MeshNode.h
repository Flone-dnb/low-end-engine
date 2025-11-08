#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/node/SpatialNode.h"
#include "game/geometry/MeshNodeGeometry.h"
#include "material/Material.h"
#include "render/wrapper/VertexArrayObject.h"
#include "game/geometry/shapes/AABB.h"

class MeshRenderingHandle;

/** Represents a node that can have 3D geometry to display (mesh). */
class MeshNode : public SpatialNode {
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

    virtual ~MeshNode() override;

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
    virtual std::string_view getPathToDefaultVertexShader() {
        return "engine/shaders/node/MeshNode.vert.glsl";
    }

    /**
     * Returns path to .glsl file relative `res` directory that's used as a default fragment shader for mesh
     * nodes.
     *
     * @return Path to .glsl file relative `res` directory.
     */
    static inline std::string_view getPathToDefaultFragmentShader() {
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
    void setMeshGeometryBeforeSpawned(const MeshNodeGeometry& meshGeometry);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setMeshGeometryBeforeSpawned(MeshNodeGeometry&& meshGeometry);

    /**
     * Sets whether this mesh is visible or not.
     *
     * @param bNewVisible Whether this mesh is visible or not.
     */
    void setIsVisible(bool bNewVisible);

    /**
     * Returns material.
     *
     * @remark You can modify this material's properties even while the node is spawned.
     *
     * @return Material.
     */
    Material& getMaterial();

    /**
     * Returns a copy of the mesh's geometry.
     *
     * @return Geometry.
     */
    MeshNodeGeometry copyMeshData() const { return meshGeometry; }

    /**
     * Tells whether this mesh is currently visible or not.
     *
     * @return Whether the mesh is visible or not.
     */
    bool isVisible() const { return bIsVisible; }

    /**
     * Returns rendering handle.
     *
     * @warning Do not delete (free) returned pointer.
     * @warning Rendering data is automatically updated inside of the node, you don't need
     * to use the handle to update the rendering data. This function is made public
     * in case you add some custom variables to mesh rendering data, this way you would
     * have a way to update your custom variables.
     *
     * @return `nullptr` if not being rendered.
     */
    MeshRenderingHandle* getRenderingHandle() const { return pRenderingHandle.get(); }

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

    /**
     * Creates VAO for the node.
     *
     * @return Created VAO.
     */
    virtual std::unique_ptr<VertexArrayObject> createVertexArrayObject();

    /**
     * Because SkeletalMeshNode is a child type of MeshNode but uses a different type of a geometry,
     * skeletal mesh overrides this function to make sure if the wrong function (to set the geometry) is
     * called we will show an error message.
     *
     * @return Type of this node.
     */
    virtual bool isUsingSkeletalMeshGeometry() { return false; }

    /**
     * Calculates AABB from @ref meshGeometry.
     *
     * Derived types may override this function to calculate bounding box
     * from other geometry.
     *
     * @return AABB.
     */
    virtual AABB calculateBoundingBoxFromGeometry();

    /**
     * Called after the rendering handle was received.
     *
     * @param pRenderingHandle New rendering handle.
     */
    virtual void onRenderingHandleInitialized(MeshRenderingHandle* pRenderingHandle) {}

    /** Makes sure mesh node geometry is empty (used by derived nodes with different type of geometry. */
    void clearMeshNodeGeometry();

private:
    /** Creates GPU resources and adds this object to be rendered. */
    void registerToRendering();

    /** Removes this object from rendering and destroys GPU resources. */
    void unregisterFromRendering();

    /**
     * Copies up to date data for rendering.
     *
     * @remark Does nothing if not registered in the rendering.
     *
     * @param bJustRegistered Specify `true` if @ref pRenderingHandle was just initialized
     * and this function is called to initialize the data.
     */
    void updateRenderData(bool bJustRegistered = false);

    /** Mesh geometry. */
    MeshNodeGeometry meshGeometry;

    /** Material. */
    Material material;

    /** Not `nullptr` while spawned. */
    std::unique_ptr<VertexArrayObject> pVao;

    /** Not `nullptr` if spawned and visible. */
    std::unique_ptr<MeshRenderingHandle> pRenderingHandle;

    /** Whether mesh is visible or not. */
    bool bIsVisible = true;
};
