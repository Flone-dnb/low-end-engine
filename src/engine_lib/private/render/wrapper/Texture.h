#pragma once

// Standard.
#include <utility>

/**
 * Manages OpenGL texture object.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class Texture {
    // Only GPU resource manager and font manager are expected to create objects of this type.
    friend class GpuResourceManager;
    friend class FontManager;

public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept = delete;
    Texture& operator=(Texture&&) noexcept = delete;

    ~Texture();

    /**
     * Returns ID of the texture.
     *
     * @return ID.
     */
    unsigned int getTextureId() const { return iTextureId; }

    /**
     * Returns size of the texture in pixels.
     *
     * @return Size.
     */
    std::pair<unsigned int, unsigned int> getSize() const { return size; }

    /**
     * Returns OpenGL format of the texture.
     *
     * @return Format.
     */
    int getGlFormat() const { return iGlFormat; }

private:
    /**
     * Initializes the object.
     *
     * @param iTextureId OpenGL ID of the texture.
     * @param iWidth     Width of the texture in pixels.
     * @param iHeight    Height of the texture in pixels.
     * @param iGlFormat  OpenGL format of the texture.
     */
    Texture(unsigned int iTextureId, unsigned int iWidth, unsigned int iHeight, int iGlFormat);

    /** OpenGL ID of the texture. */
    const unsigned int iTextureId = 0;

    /** Size (in pixels) of the texture. */
    const std::pair<unsigned int, unsigned int> size = {0, 0};

    /** OpenGL format of the texture. */
    const int iGlFormat = 0;
};
