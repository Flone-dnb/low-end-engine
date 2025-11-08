#pragma once

// Standard.
#include <mutex>

// Custom.
#include "math/GLMath.hpp"
#include "game/node/MeshNode.h"
#include "game/geometry/SkeletalMeshNodeGeometry.h"
#include "material/Material.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/ShaderConstantsSetter.hpp"

class SkeletonNode;

/** 3D geometry controlled by a SkeletonNode (which is expected to be a parent node). */
class SkeletalMeshNode : public MeshNode {
public:
    SkeletalMeshNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SkeletalMeshNode(const std::string& sNodeName);

    virtual ~SkeletalMeshNode() override = default;

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
    virtual std::string_view getPathToDefaultVertexShader() override;

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setSkeletalMeshGeometryBeforeSpawned(const SkeletalMeshNodeGeometry& meshGeometry);

    /**
     * Sets mesh geometry to use.
     *
     * @warning If this function is used while the node is spawned an error message will be shown.
     *
     * @param meshGeometry Mesh geometry.
     */
    void setSkeletalMeshGeometryBeforeSpawned(SkeletalMeshNodeGeometry&& meshGeometry);

    /**
     * Returns a copy of the mesh's geometry.
     *
     * @return Geometry.
     */
    SkeletalMeshNodeGeometry copySkeletalMeshData() const { return skeletalMeshGeometry; }

protected:
    /** Called after this object was finished deserializing from file. */
    virtual void onAfterDeserialized() override;

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
     * Called after this node or one of the node's parents (in the parent hierarchy)
     * was attached to a new parent node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    virtual void onAfterAttachedToNewParent(bool bThisNodeBeingAttached) override;

    /**
     * Creates VAO for the node.
     *
     * @return Created VAO.
     */
    virtual std::unique_ptr<VertexArrayObject> createVertexArrayObject() override;

    /**
     * Because SkeletalMeshNode is a child type of MeshNode but uses a different type of a geometry,
     * skeletal mesh overrides this function to make sure if the wrong function (to set the geometry) is
     * called we will show an error message.
     *
     * @return Type of this node.
     */
    virtual bool isUsingSkeletalMeshGeometry() override;

    /**
     * Calculates AABB from @ref skeletalMeshGeometry.
     *
     * Derived types may override this function to calculate bounding box
     * from other geometry.
     *
     * @return AABB.
     */
    virtual AABB calculateBoundingBoxFromGeometry() override;

    /**
     * Called after the rendering handle was received.
     *
     * @param pRenderingHandle New rendering handle.
     */
    virtual void onRenderingHandleInitialized(MeshRenderingHandle* pRenderingHandle) override;

private:
    /** Mesh geometry. */
    SkeletalMeshNodeGeometry skeletalMeshGeometry;

    /**
     * `nullptr` if this node is not spawned or the parent node is not a skeleton,
     * otherwise cached pointer to the direct skeleton node parent.
     */
    SkeletonNode* pSpawnedSkeleton = nullptr;
};
