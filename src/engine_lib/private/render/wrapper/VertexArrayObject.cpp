#include "render/wrapper/VertexArrayObject.h"

// External.
#include "glad/glad.h"
#include "misc/Error.h"

VertexArrayObject::~VertexArrayObject() {
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
