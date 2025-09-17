#include "render/wrapper/VertexArrayObject.h"

// External.
#include "glad/glad.h"
#include "misc/Error.h"

VertexArrayObject::~VertexArrayObject() {
    // Don't need to wait for the GPU to finish using this data because:
    // When a buffer, texture, sampler, render buffer, query, or sync object is deleted, its name immediately
    // becomes invalid (e.g. is marked unused), but the underlying object will not be deleted until it is no
    // longer in use.

    GL_CHECK_ERROR(glDeleteVertexArrays(1, &iVertexArrayObjectId));
    GL_CHECK_ERROR(glDeleteBuffers(1, &iVertexBufferObjectId));
    if (iIndexBufferObjectId.has_value()) {
        GL_CHECK_ERROR(glDeleteBuffers(1, &*iIndexBufferObjectId));
    }
}

VertexArrayObject::VertexArrayObject(
    unsigned int iVertexArrayObjectId,
    unsigned int iVertexBufferObjectId,
    unsigned int iVertexCount,
    std::optional<unsigned int> iIndexBufferObjectId,
    std::optional<int> iIndexCount)
    : iVertexArrayObjectId(iVertexArrayObjectId), iVertexBufferObjectId(iVertexBufferObjectId),
      iVertexCount(iVertexCount), iIndexBufferObjectId(iIndexBufferObjectId), iIndexCount(iIndexCount) {}
