#pragma once

// Standard.
#include <filesystem>

// Custom.
#include "math/GLMath.hpp"

/**
 * Vertex of a mesh.
 *
 * @remark Size and layout is equal to the vertex struct we use in shaders.
 */
struct MeshNodeVertex { // not using inheritance to avoid extra fields that are not related to vertex
    MeshNodeVertex() = default;
    ~MeshNodeVertex() = default;

    /** Copy constructor. */
    MeshNodeVertex(const MeshNodeVertex&) = default;
    /** Copy assignment. @return this. */
    MeshNodeVertex& operator=(const MeshNodeVertex&) = default;

    /** Move constructor. */
    MeshNodeVertex(MeshNodeVertex&&) noexcept = default;
    /** Move assignment. @return this. */
    MeshNodeVertex& operator=(MeshNodeVertex&&) noexcept = default;

    /** Describes to OpenGL how vertex data should be interpreted. */
    static void setVertexAttributes();

    /**
     * Equality operator.
     *
     * @param other Other object.
     *
     * @return Whether objects are equal or not.
     */
    bool operator==(const MeshNodeVertex& other) const;

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
class MeshNodeGeometry {
public:
    /** Type of an index in the index buffer. */
    using MeshIndexType = unsigned short;

    MeshNodeGeometry() = default;
    ~MeshNodeGeometry() = default;

    /** Copy constructor. */
    MeshNodeGeometry(const MeshNodeGeometry&) = default;
    /** Copy assignment. @return this. */
    MeshNodeGeometry& operator=(const MeshNodeGeometry&) = default;

    /** Move constructor. */
    MeshNodeGeometry(MeshNodeGeometry&&) noexcept = default;
    /** Move assignment. @return this. */
    MeshNodeGeometry& operator=(MeshNodeGeometry&&) noexcept = default;

    /**
     * Deserializes the geometry from the file (also see @ref serialize).
     *
     * @param pathToFile File to deserialize from.
     *
     * @return Geometry.
     */
    static MeshNodeGeometry deserialize(const std::filesystem::path& pathToFile);

    /**
     * Equality operator.
     *
     * @param other Other object.
     *
     * @return Whether the geometry is the same or not.
     */
    bool operator==(const MeshNodeGeometry& other) const;

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
    std::vector<MeshNodeVertex>& getVertices() { return vVertices; }

    /**
     * Returns mesh vertices.
     *
     * @return Mesh vertices.
     */
    const std::vector<MeshNodeVertex>& getVertices() const { return vVertices; }

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
    std::vector<MeshNodeVertex> vVertices;

    /** Indices for mesh's index buffer. */
    std::vector<MeshIndexType> vIndices;
};
