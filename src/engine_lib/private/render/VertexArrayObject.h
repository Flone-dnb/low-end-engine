#pragma once

/**
 * Groups OpenGL-related data used to draw a mesh.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class VertexArrayObject {
    // Only GPU resource manager is expected to create objects of this type.
    friend class GpuResourceManager;

public:
    VertexArrayObject() = delete;

    VertexArrayObject(const VertexArrayObject&) = delete;
    VertexArrayObject& operator=(const VertexArrayObject&) = delete;
    VertexArrayObject(VertexArrayObject&&) noexcept = delete;
    VertexArrayObject& operator=(VertexArrayObject&&) noexcept = delete;

    ~VertexArrayObject();

    /**
     * Returns VAO.
     *
     * @return VAO.
     */
    unsigned int getVertexArrayObjectId() { return iVertexArrayObjectId; }

    /**
     * Returns number of indices to draw.
     *
     * @return Index count.
     */
    int getIndexCount() { return iIndexCount; }

private:
    /**
     * Initializes a new object.
     *
     * @param iVertexArrayObjectId  ID of the vertex array object.
     * @param iVertexBufferObjectId ID of the vertex buffer object.
     * @param iIndexBufferObjectId  ID of the index buffer object.
     * @param iIndexCount           Number of indices to draw.
     */
    VertexArrayObject(
        unsigned int iVertexArrayObjectId,
        unsigned int iVertexBufferObjectId,
        unsigned int iIndexBufferObjectId,
        int iIndexCount);

    /** ID of the vertex array object. */
    unsigned int iVertexArrayObjectId = 0;

    /** ID of the vertex buffer object. */
    unsigned int iVertexBufferObjectId = 0;

    /** ID of the index buffer object. */
    unsigned int iIndexBufferObjectId = 0;

    /** Number of indices to draw. */
    int iIndexCount = 0;
};
