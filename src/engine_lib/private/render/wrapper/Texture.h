#pragma once

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

private:
    /**
     * Initializes the object.
     *
     * @param iTextureId OpenGL ID of the texture.
     */
    Texture(unsigned int iTextureId);

    /** OpenGL ID of the texture. */
    const unsigned int iTextureId = 0;
};
