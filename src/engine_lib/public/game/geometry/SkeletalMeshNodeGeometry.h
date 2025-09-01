#pragma once

// Standard.
#include <filesystem>
#include <array>

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/MeshIndexType.hpp"

/**
 * Vertex of a mesh.
 *
 * @remark Size and layout is equal to the vertex struct we use in shaders.
 */
struct SkeletalMeshNodeVertex { // not using inheritance to avoid extra fields that are not related to vertex
    /** Type for the index to a bone that influences a vertex. */
    using BoneIndexType = unsigned char;

    SkeletalMeshNodeVertex() = default;
    ~SkeletalMeshNodeVertex() = default;

    /** Copy constructor. */
    SkeletalMeshNodeVertex(const SkeletalMeshNodeVertex&) = default;
    /** Copy assignment. @return this. */
    SkeletalMeshNodeVertex& operator=(const SkeletalMeshNodeVertex&) = default;

    /** Move constructor. */
    SkeletalMeshNodeVertex(SkeletalMeshNodeVertex&&) noexcept = default;
    /** Move assignment. @return this. */
    SkeletalMeshNodeVertex& operator=(SkeletalMeshNodeVertex&&) noexcept = default;

    /** Describes to OpenGL how vertex data should be interpreted. */
    static void setVertexAttributes();

    /**
     * Equality operator.
     *
     * @param other Other object.
     *
     * @return Whether objects are equal or not.
     */
    bool operator==(const SkeletalMeshNodeVertex& other) const;

    // --------------------------------------------------------------------------------------

    /** Position of the vertex in a 3D space. */
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Normal vector of the vertex. */
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);

    /** UV coordinates of the vertex. */
    glm::vec2 uv = glm::vec2(0.0f, 0.0f);

    /** Indices of bones on the skeleton that affect this vertex. */
    std::array<BoneIndexType, 4> vBoneIndices;

    /** Weights in range [0; 1] of bones from @ref vBoneIndices. */
    std::array<float, 4> vBoneWeights;

    // --------------------------------------------------------------------------------------

    // ! only vertex related fields (same as in shader) can be added here !
    // (not deriving from `Serializable` to avoid extra fields that are not related to vertex)

    // --------------------------------------------------------------------------------------
};

/** Stores geometry (vertices and indices) for SkeletalMeshNode. */
class SkeletalMeshNodeGeometry {
public:
    SkeletalMeshNodeGeometry() = default;
    ~SkeletalMeshNodeGeometry() = default;

    /** Copy constructor. */
    SkeletalMeshNodeGeometry(const SkeletalMeshNodeGeometry&) = default;
    /** Copy assignment. @return this. */
    SkeletalMeshNodeGeometry& operator=(const SkeletalMeshNodeGeometry&) = default;

    /** Move constructor. */
    SkeletalMeshNodeGeometry(SkeletalMeshNodeGeometry&&) noexcept = default;
    /** Move assignment. @return this. */
    SkeletalMeshNodeGeometry& operator=(SkeletalMeshNodeGeometry&&) noexcept = default;

    /**
     * Deserializes the geometry from the file (also see @ref serialize).
     *
     * @param pathToFile File to deserialize from.
     *
     * @return Geometry.
     */
    static SkeletalMeshNodeGeometry deserialize(const std::filesystem::path& pathToFile);

    /**
     * Equality operator.
     *
     * @param other Other object.
     *
     * @return Whether the geometry is the same or not.
     */
    bool operator==(const SkeletalMeshNodeGeometry& other) const;

    /**
     * Serializes the geometry data into a file.
     *
     * @param pathToFile File to serialize to.
     */
    void serialize(const std::filesystem::path& pathToFile) const;

    /**
     * Returns mesh vertices.
     *
     * @return Mesh vertices.
     */
    std::vector<SkeletalMeshNodeVertex>& getVertices() { return vVertices; }

    /**
     * Returns mesh vertices.
     *
     * @return Mesh vertices.
     */
    const std::vector<SkeletalMeshNodeVertex>& getVertices() const { return vVertices; }

    /**
     * Returns mesh indices.
     *
     * @return Mesh indices.
     */
    std::vector<MeshIndexType>& getIndices() { return vIndices; }

    /**
     * Returns mesh indices.
     *
     * @return Mesh indices.
     */
    const std::vector<MeshIndexType>& getIndices() const { return vIndices; }

private:
    /** Vertices for mesh's vertex buffer. */
    std::vector<SkeletalMeshNodeVertex> vVertices;

    /** Indices for mesh's index buffer. */
    std::vector<MeshIndexType> vIndices;
};
