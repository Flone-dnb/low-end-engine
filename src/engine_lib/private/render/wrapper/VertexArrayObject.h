#pragma once

// Standard.
#include <optional>

// Custom.
#include "misc/Error.h"

/**
 * Groups OpenGL-related data used to draw a mesh.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class VertexArrayObject {
public:
    VertexArrayObject() = delete;

    VertexArrayObject(const VertexArrayObject&) = delete;
    VertexArrayObject& operator=(const VertexArrayObject&) = delete;
    VertexArrayObject(VertexArrayObject&&) noexcept = delete;
    VertexArrayObject& operator=(VertexArrayObject&&) noexcept = delete;

    ~VertexArrayObject();

    /**
     * Initializes a new object.
     *
     * @param iVertexArrayObjectId  ID of the vertex array object.
     * @param iVertexBufferObjectId ID of the vertex buffer object.
     * @param iVertexCount          Number of vertices in the vertex buffer.
     * @param iIndexBufferObjectId  ID of the index buffer object (if used).
     * @param iIndexCount           Number of indices to draw (from used index buffer).
     */
    VertexArrayObject(
        unsigned int iVertexArrayObjectId,
        unsigned int iVertexBufferObjectId,
        unsigned int iVertexCount,
        std::optional<unsigned int> iIndexBufferObjectId = {},
        std::optional<int> iIndexCount = {});

    /**
     * Returns VAO.
     *
     * @return VAO.
     */
    unsigned int getVertexArrayObjectId() const { return iVertexArrayObjectId; }

    /**
     * Returns VBO.
     *
     * @return VBO.
     */
    unsigned int getVertexBufferObjectId() const { return iVertexBufferObjectId; }

    /**
     * Returns the total number of vertices in the vertex buffer.
     *
     * @return Vertex count.
     */
    unsigned int getVertexCount() const { return iVertexCount; }

    /**
     * Returns the total number of indices to draw.
     *
     * @return Index count.
     */
    int getIndexCount() {
#if defined(DEBUG)
        if (!iIndexCount.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException("index buffer is not used on this VAO");
        }
#endif
        return *iIndexCount;
    }

private:
    /** ID of the vertex array object. */
    unsigned int iVertexArrayObjectId = 0;

    /** ID of the vertex buffer object. */
    unsigned int iVertexBufferObjectId = 0;

    /** Number of vertices in the vertex buffer. */
    unsigned int iVertexCount = 0;

    /** ID of the index buffer object. */
    std::optional<unsigned int> iIndexBufferObjectId;

    /** Number of indices to draw. */
    std::optional<int> iIndexCount;
};
