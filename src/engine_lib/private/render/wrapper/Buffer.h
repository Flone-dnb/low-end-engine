#pragma once

/**
 * Manages OpenGL buffer object.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class Buffer {
    // Only GPU resource manager is expected to create objects of this type.
    friend class GpuResourceManager;

public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = delete;
    Buffer& operator=(Buffer&&) noexcept = delete;

    ~Buffer();

    /**
     * Returns ID of the buffer.
     *
     * @return ID.
     */
    unsigned int getBufferId() const { return iBufferId; }

    /**
     * If this buffer was created as a dynamic buffer copied data from the specified pointer
     * to the buffer.
     *
     * @param iStartOffset Offset in bytes from the start of the buffer to copy the data to.
     * @param iDataSize    Size in bytes to copy from the specified data pointer.
     * @param pData        Pointer to copy the data from.
     */
    void copyDataToBuffer(unsigned int iStartOffset, unsigned int iDataSize, const void* pData) const;

private:
    /**
     * Initializes the object.
     *
     * @param iSizeInBytes Size of the buffer in bytes.
     * @param iBufferId    ID of the Buffer.
     * @param iGlType      OpenGL type of this buffer.
     * @param bIsDynamic   `true` if buffer data can be updated.
     */
    Buffer(unsigned int iSizeInBytes, unsigned int iBufferId, int iGlType, bool bIsDynamic);

    /** Size of the buffer in bytes. */
    const unsigned int iSizeInBytes = 0;

    /** ID of the buffer. */
    const unsigned int iBufferId = 0;

    /** OpenGL type of this buffer. */
    const int iGlType = 0;

    /** `true` if @ref copyDataToBuffer can be used. */
    const bool bIsDynamic = false;
};
