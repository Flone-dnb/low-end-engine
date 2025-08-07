#include "render/wrapper/Buffer.h"

// Standard.
#include <mutex>

// Custom.
#include "misc/Error.h"
#include "render/GpuResourceManager.h"

// External.
#include "glad/glad.h"

Buffer::~Buffer() { GL_CHECK_ERROR(glDeleteBuffers(1, &iBufferId)); }

void Buffer::copyDataToBuffer(unsigned int iStartOffset, unsigned int iDataSize, const void* pData) const {
    if (!bCpuWriteAccess) [[unlikely]] {
        Error::showErrorAndThrowException(
            "can't copy data because this buffer does not have CPU-write access");
    }

    // Prevent working with content from multiple threads.
    std::scoped_lock guard(GpuResourceManager::mtx);

    glBindBuffer(iGlType, iBufferId);
    glBufferSubData(iGlType, iStartOffset, iDataSize, pData);
    glBindBuffer(iGlType, 0);
}

Buffer::Buffer(unsigned int iSizeInBytes, unsigned int iBufferId, int iGlType, bool bCpuWriteAccess)
    : iSizeInBytes(iSizeInBytes), iBufferId(iBufferId), iGlType(iGlType), bCpuWriteAccess(bCpuWriteAccess) {}
