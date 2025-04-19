#pragma once

// Standard.
#include <string>

class TextureManager;

/**
 * RAII-style object that tells the manager to not release the texture from the memory while it's
 * being used. A texture resource will be released from the memory when no texture handle that
 * references the same resource path will exist.
 */
class TextureHandle {
    // We expect that only texture manager will create texture handles.
    friend class TextureManager;

public:
    TextureHandle() = delete;

    TextureHandle(const TextureHandle&) = delete;
    TextureHandle& operator=(const TextureHandle&) = delete;

    TextureHandle(TextureHandle&&) = delete;
    TextureHandle& operator=(TextureHandle&&) = delete;

    /** Notifies manager about handle no longer referencing the texture. */
    ~TextureHandle();

    /**
     * Returns OpenGL ID of the texture.
     *
     * @return ID.
     */
    unsigned int getTextureId() const { return iTextureId; }

private:
    /**
     * Creates a new texture handle that references a specific texture resource.
     *
     * @param pTextureManager  Texture manager that created this handle. It will be notified
     * when the texture handle is being destroyed.
     * @param iTextureId       OpenGL ID of the texture.
     * @param sPathToTextureRelativeRes Path to the texture, relative `res` directory.
     */
    TextureHandle(
        TextureManager* pTextureManager,
        unsigned int iTextureId,
        const std::string& sPathToTextureRelativeRes);

    /** OpenGL ID of the texture. */
    const unsigned int iTextureId = 0;

    /** Path to the texture, relative `res` directory. */
    const std::string sPathToTextureRelativeRes;

    /** Do not delete (free) this pointer. Texture manager that created this object. */
    TextureManager* const pTextureManager = nullptr;
};
