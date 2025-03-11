#pragma once

// Custom.
#include "math/GLMath.hpp"

/**
 * Vertex of a mesh.
 *
 * @remark Size and layout is equal to the vertex struct we use in shaders.
 */
struct MeshVertex { // not using inheritance to avoid extra fields that are not related to vertex
    MeshVertex() = default;
    ~MeshVertex() = default;

    /** Copy constructor. */
    MeshVertex(const MeshVertex&) = default;
    /** Copy assignment. @return this. */
    MeshVertex& operator=(const MeshVertex&) = default;

    /** Move constructor. */
    MeshVertex(MeshVertex&&) noexcept = default;
    /** Move assignment. @return this. */
    MeshVertex& operator=(MeshVertex&&) noexcept = default;

    /** Describes to OpenGL how vertex data should be interpreted. */
    static void setVertexAttributes();

    /**
     * Equality operator.
     *
     * @param other Other object.
     *
     * @return Whether objects are equal or not.
     */
    bool operator==(const MeshVertex& other) const;

    // --------------------------------------------------------------------------------------

    /** Position of the vertex in a 3D space. */
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Normal vector of the vertex. */
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);

    /** UV coordinates of the vertex. */
    glm::vec2 uv = glm::vec2(0.0f, 0.0f);

    // --------------------------------------------------------------------------------------

    // ! only vertex related fields (same as in shader) can be added here !
    // (not deriving from `Serializable` to avoid extra fields that are not related to vertex)

    // --------------------------------------------------------------------------------------
};

/** Stores mesh geometry (vertices and indices). */
class MeshGeometry {
public:
    /** Type of an index in the index buffer. */
    using MeshIndexType = unsigned short;

    MeshGeometry() = default;
    ~MeshGeometry() = default;

    /** Copy constructor. */
    MeshGeometry(const MeshGeometry&) = default;
    /** Copy assignment. @return this. */
    MeshGeometry& operator=(const MeshGeometry&) = default;

    /** Move constructor. */
    MeshGeometry(MeshGeometry&&) noexcept = default;
    /** Move assignment. @return this. */
    MeshGeometry& operator=(MeshGeometry&&) noexcept = default;

    /**
     * Returns mesh vertices.
     *
     * @return Mesh vertices.
     */
    std::vector<MeshVertex>& getVertices() { return vVertices; }

    /**
     * Returns mesh vertices.
     *
     * @return Mesh vertices.
     */
    const std::vector<MeshVertex>& getVertices() const { return vVertices; }

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
    std::vector<MeshVertex> vVertices;

    /** Indices for mesh's index buffer. */
    std::vector<MeshIndexType> vIndices;
};
