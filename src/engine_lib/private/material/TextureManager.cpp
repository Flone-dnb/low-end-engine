#include "material/TextureManager.h"

// Standard.
#include <format>
#include <cstring>

// Custom.
#include "misc/ProjectPaths.h"
#include "io/Log.h"
#include "render/GpuResourceManager.h"
#include "misc/Profiler.hpp"

// External.
#include "nameof.hpp"
#include "glad/glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

TextureManager::~TextureManager() {
    std::scoped_lock guard(mtxLoadedTextures.first);

    // Make sure no resource is loaded.
    if (!mtxLoadedTextures.second.empty()) [[unlikely]] {
        // Prepare a description of all not released resources.
        std::string sLoadedTextures;
        for (const auto& [sPath, resourceInfo] : mtxLoadedTextures.second) {
            sLoadedTextures += std::format(
                "- \"{}\", alive handles that reference this path: {}\n",
                sPath,
                resourceInfo.iActiveTextureHandleCount);
        }

        Error::showErrorAndThrowException(std::format(
            "texture manager is being destroyed but there are still {} texture(s) "
            "loaded in the memory:\n"
            "{}",
            mtxLoadedTextures.second.size(),
            sLoadedTextures));
    }
}

size_t TextureManager::getTextureInMemoryCount() {
    std::scoped_lock guard(mtxLoadedTextures.first);

    return mtxLoadedTextures.second.size();
}

void TextureManager::setUsePointFiltering(bool bUsePointFiltering) {
    bIsUsingPointFiltering = bUsePointFiltering;
}

std::variant<std::unique_ptr<TextureHandle>, Error>
TextureManager::getTexture(const std::string& sPathToTextureRelativeRes, TextureUsage usage) {
    PROFILE_FUNC

    std::scoped_lock guard(mtxLoadedTextures.first);

    // See if a texture by this path is already loaded.
    const auto it = mtxLoadedTextures.second.find(sPathToTextureRelativeRes);
    if (it == mtxLoadedTextures.second.end()) {
        // Load texture and create a new handle.
        auto result = loadTextureAndCreateNewHandle(sPathToTextureRelativeRes, usage);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            return error;
        }

        // Return created handle.
        return std::get<std::unique_ptr<TextureHandle>>(std::move(result));
    }

    // Just create a new handle.
    return createNewHandleForLoadedTexture(sPathToTextureRelativeRes, usage);
}

void TextureManager::releaseTextureIfNotUsed(const std::string& sPathToTextureRelativeRes) {
    std::scoped_lock guard(mtxLoadedTextures.first);

    // Make sure a resource by this path is actually loaded.
    const auto it = mtxLoadedTextures.second.find(sPathToTextureRelativeRes);
    if (it == mtxLoadedTextures.second.end()) [[unlikely]] {
        // This should not happen, something is wrong.
        Log::error(std::format(
            "a texture handle just notified the texture manager about "
            "no longer referencing a texture resource at \"{}\" "
            "but the manager does not store resources from this path",
            sPathToTextureRelativeRes));
        return;
    }

    // Self check: make sure the handle counter is not zero.
    if (it->second.iActiveTextureHandleCount == 0) [[unlikely]] {
        Log::error(std::format(
            "a texture handle just notified the texture manager "
            "about no longer referencing a texture resource at \"{}\", "
            "the manager has such a resource entry but the current "
            "handle counter is zero",
            sPathToTextureRelativeRes));
        return;
    }

    it->second.iActiveTextureHandleCount -= 1;

    if (it->second.iActiveTextureHandleCount == 0) {
        mtxLoadedTextures.second.erase(it);
    }
}

std::unique_ptr<TextureHandle> TextureManager::createNewHandleForLoadedTexture(
    const std::string& sPathToTextureRelativeRes, TextureUsage usage) {
    std::scoped_lock guard(mtxLoadedTextures.first);

    // Find texture.
    const auto texIt = mtxLoadedTextures.second.find(sPathToTextureRelativeRes);
    if (texIt == mtxLoadedTextures.second.end()) [[unlikely]] {
        // This should not happen.
        Error::showErrorAndThrowException(std::format(
            "requested to create a new texture handle for not loaded path \"{}\" "
            "(this is a bug, report to developers)",
            sPathToTextureRelativeRes));
    }

    // Make sure the usage is the same.
    if (usage != texIt->second.usage) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "texture usage mismatch: when the texture \"{}\" was first requested its usage was {} now "
            "another handle is requested for this texture but the usage is now {}",
            texIt->first,
            NAMEOF_ENUM(texIt->second.usage),
            NAMEOF_ENUM(usage)));
    }

    // Increment texture handle count.
    texIt->second.iActiveTextureHandleCount += 1;

    // Self check: make sure handle counter will not hit type limit.
    if (texIt->second.iActiveTextureHandleCount == ULLONG_MAX) [[unlikely]] {
        Log::warn(std::format(
            "texture handle counter for resource \"{}\" just hit type limit "
            "with value: {}, new texture handle for this resource will make the counter invalid",
            sPathToTextureRelativeRes,
            texIt->second.iActiveTextureHandleCount));
    }

    return std::unique_ptr<TextureHandle>(
        new TextureHandle(this, texIt->second.iTextureId, sPathToTextureRelativeRes));
}

std::variant<std::unique_ptr<TextureHandle>, Error> TextureManager::loadTextureAndCreateNewHandle(
    const std::string& sPathToTextureRelativeRes, TextureUsage usage) {
    PROFILE_FUNC

    std::scoped_lock guard(mtxLoadedTextures.first, GpuResourceManager::mtx);

    if (usage != TextureUsage::CUBEMAP_NO_MIPMAP) {
        // Construct full path to the texture.
        auto pathToTexture =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToTextureRelativeRes;

        // Make sure it's a file.
        if (!std::filesystem::exists(pathToTexture)) [[unlikely]] {
            return Error(std::format("expected the path \"{}\" to exist", pathToTexture.string()));
        }
        if (std::filesystem::is_directory(pathToTexture)) [[unlikely]] {
            return Error(std::format("expected the path \"{}\" to point to a file", pathToTexture.string()));
        }

        // Load pixel data.
        const auto sPathToTexture = pathToTexture.string();
        int iWidth = 0;
        int iHeight = 0;
        int iChannelCount = 0;
        const auto pImageData = stbi_load(sPathToTexture.c_str(), &iWidth, &iHeight, &iChannelCount, 0);
        if (pImageData == nullptr) [[unlikely]] {
            return Error(std::format("failed to load texture \"{}\"", sPathToTexture));
        }

        const auto iGlFormat = iChannelCount == 4 ? GL_RGBA : GL_RGB;
        const auto iGlInternalFormat = iGlFormat;

        // Create a new texture object.
        unsigned int iTextureId = 0;
        GL_CHECK_ERROR(glGenTextures(1, &iTextureId));

        glBindTexture(GL_TEXTURE_2D, iTextureId);
        {
            // Copy pixels to the GPU resource.
            GL_CHECK_ERROR(glTexImage2D(
                GL_TEXTURE_2D,
                0,
                iGlInternalFormat,
                iWidth,
                iHeight,
                0,
                iGlFormat,
                GL_UNSIGNED_BYTE,
                pImageData));

            if (usage == TextureUsage::DIFFUSE) {
                GL_CHECK_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
            }

            // Set texture wrapping.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Set texture filtering.
            if (bIsUsingPointFiltering) {
                if (usage == TextureUsage::DIFFUSE) {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                } else {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            } else {
                if (usage == TextureUsage::DIFFUSE) {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                } else {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pImageData);

        // Add new resource to be considered.
        TextureResource resourceInfo;
        resourceInfo.iActiveTextureHandleCount = 0; // 0 because `createNewTextureHandle` will increment it
        resourceInfo.iTextureId = iTextureId;
        resourceInfo.usage = usage;
        mtxLoadedTextures.second[sPathToTextureRelativeRes] = resourceInfo;
    } else {
        // Stores 6 textures.
        auto pathToTexDir =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToTextureRelativeRes;

        if (!std::filesystem::exists(pathToTexDir)) [[unlikely]] {
            return Error(std::format("expected the path \"{}\" to exist", pathToTexDir.string()));
        }
        if (!std::filesystem::is_directory(pathToTexDir)) [[unlikely]] {
            return Error(std::format(
                "expected the path \"{}\" to point to a directory that stores 6 textures for the cubemap",
                pathToTexDir.string()));
        }

        const std::array<std::string_view, 6> vFilenames = {
            "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png"};

        unsigned int iCubemapId = 0;
        glGenTextures(1, &iCubemapId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, iCubemapId);

        for (size_t i = 0; i < vFilenames.size(); i++) {
            const auto sFilename = vFilenames[i];
            const auto pathToTex = pathToTexDir / sFilename;
            if (!std::filesystem::exists(pathToTex)) [[unlikely]] {
                return Error(std::format(
                    "in the directory \"{}\" expected to find 6 textures for the cubemap, texture file with "
                    "the name \"{}\" is missing",
                    pathToTexDir.filename().string(),
                    sFilename));
            }

            // Load pixel data.
            const auto sPathToTexture = pathToTex.string();
            int iWidth = 0;
            int iHeight = 0;
            int iChannelCount = 0;
            const auto pImageData = stbi_load(sPathToTexture.c_str(), &iWidth, &iHeight, &iChannelCount, 0);
            if (pImageData == nullptr) [[unlikely]] {
                return Error(std::format("failed to load texture \"{}\"", sPathToTexture));
            }

            const auto iGlFormat = iChannelCount == 4 ? GL_RGBA : GL_RGB;
            const auto iGlInternalFormat = iGlFormat;

            // Copy pixels to the GPU resource.
            GL_CHECK_ERROR(glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(i),
                0,
                iGlInternalFormat,
                iWidth,
                iHeight,
                0,
                iGlFormat,
                GL_UNSIGNED_BYTE,
                pImageData));

            stbi_image_free(pImageData);
        }

        // Set texture filtering.
        if (bIsUsingPointFiltering) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        // Set texture wrapping.
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        // Add new resource to be considered.
        TextureResource resourceInfo;
        resourceInfo.iActiveTextureHandleCount = 0; // 0 because `createNewTextureHandle` will increment it
        resourceInfo.iTextureId = iCubemapId;
        resourceInfo.usage = usage;
        mtxLoadedTextures.second[sPathToTextureRelativeRes] = resourceInfo;
    }

    return createNewHandleForLoadedTexture(sPathToTextureRelativeRes, usage);
}
