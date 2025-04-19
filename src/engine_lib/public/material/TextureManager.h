#pragma once

// Standard.
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <variant>

// Custom.
#include "misc/Error.h"
#include "material/TextureHandle.h"
#include "material/TextureUsage.hpp"

/** Controls texture loading and owns all textures. */
class TextureManager {
    // Texture handles will notify texture manager in destructor to mark referenced texture resource
    // as not used so that texture manager can release the texture resource from memory
    // if no texture handle is referencing it.
    friend class TextureHandle;

    // Only renderer is supposed to create this.
    friend class Renderer;

public:
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    /** Makes sure that no texture is still loaded in the memory. */
    ~TextureManager();

    /**
     * Returns the current number of textures loaded in the memory.
     *
     * @return Textures in memory.
     */
    size_t getTextureInMemoryCount();

    /**
     * Sets the global setting for texture filtering.
     *
     * @remark If you want to change texture filtering it's recommended to use this setting in the beginning
     * of the game when no texture is created yet.
     *
     * @param bUsePointFiltering `true` (default) to use point filtering, `false` to use linear filtering.
     */
    void setUsePointFiltering(bool bUsePointFiltering);

    /**
     * Looks if the specified texture is loaded in the GPU memory or not and if not loads it
     * in the GPU memory and returns a new handle that references this texture (if the texture is
     * already loaded just returns a new handle).
     *
     * @param sPathToTextureRelativeRes Path to the texture file relative to the `res` directory.
     * @param usage                     Describes how the texture is going to be used.
     *
     * @return Error if something went wrong, otherwise RAII-style object that tells the manager to not
     * release the texture from the memory while it's being used. A texture resource will be
     * released from the memory when no texture handle that references this path will exist.
     * Returning `std::unique_ptr` so that the handle can be "moved" and "reset" and we don't
     * need to care about implementing this functionality for the handle class.
     */
    std::variant<std::unique_ptr<TextureHandle>, Error>
    getTexture(const std::string& sPathToTextureRelativeRes, TextureUsage usage);

    /**
     * Returns the current state of the global setting for texture filtering.
     *
     * @return `true` (default) to use point filtering, `false` to use linear filtering.
     */
    bool isUsingPointFiltering() const { return bIsUsingPointFiltering; }

private:
    /** Groups information about a texture. */
    struct TextureResource {
        /** OpenGL ID of the texture. */
        unsigned int iTextureId = 0;

        /** Describes how much active texture handles there are that point to this texture. */
        size_t iActiveTextureHandleCount = 0;

        /** Initial usage that was specified when the texture was first requested. */
        TextureUsage usage = TextureUsage::DIFFUSE;
    };

    TextureManager() = default;

    /**
     * Called by texture handles in their destructor to notify the manager about a texture
     * handle no longer referencing a texture so that the manager can release
     * the texture if no other texture handle is referencing it.
     *
     * @param sPathToTextureRelativeRes Path to texture relative to the `res` directory.
     */
    void releaseTextureIfNotUsed(const std::string& sPathToTextureRelativeRes);

    /**
     * Creates a new texture handle for the specified path by using @ref mtxLoadedTextures.
     *
     * @param sPathToTextureRelativeRes Path to texture (file/directory) relative to `res` directory.
     * @param usage                      Describes how the texture is going to be used.
     *
     * @return New texture handle.
     */
    std::unique_ptr<TextureHandle>
    createNewHandleForLoadedTexture(const std::string& sPathToTextureRelativeRes, TextureUsage usage);

    /**
     * Loads the texture from the specified path and creates a new handle using
     * @ref createNewHandleForLoadedTexture.
     *
     * @param sPathToTextureRelativeRes Path to texture (file/directory) relative to `res` directory.
     * @param usage                      Describes how the texture is going to be used.
     *
     * @return Error if something went wrong, otherwise created texture handle.
     */
    std::variant<std::unique_ptr<TextureHandle>, Error>
    loadTextureAndCreateNewHandle(const std::string& sPathToTextureRelativeRes, TextureUsage usage);

    /**
     * Stores pairs of "path to texture (file/directory) relative to `res` directory" - "loaded
     * texture resource".
     */
    std::pair<std::recursive_mutex, std::unordered_map<std::string, TextureResource>> mtxLoadedTextures;

    /** Global setting for texture filtering, `true` for point filtering, `false` for linear. */
    bool bIsUsingPointFiltering = true;
};
