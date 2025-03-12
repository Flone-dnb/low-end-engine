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
    GL_CHECK_ERROR(glDeleteBuffers(1, &iIndexBufferObjectId));
}

VertexArrayObject::VertexArrayObject(
    unsigned int iVertexArrayObjectId,
    unsigned int iVertexBufferObjectId,
    unsigned int iIndexBufferObjectId,
    int iIndexCount)
    : iVertexArrayObjectId(iVertexArrayObjectId), iVertexBufferObjectId(iVertexBufferObjectId),
      iIndexBufferObjectId(iIndexBufferObjectId), iIndexCount(iIndexCount) {}
