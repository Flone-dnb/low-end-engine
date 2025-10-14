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
#include "fpng.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

std::optional<Error> TextureManager::importTextureFromFile(
    const std::filesystem::path& pathToImport, const std::string& sPathToDirToImportRelativeRes) {
    // Make sure the source exists.
    if (!std::filesystem::exists(pathToImport)) [[unlikely]] {
        return Error(std::format("expected the path \"{}\" to exist", pathToImport.string()));
    }
    if (std::filesystem::is_directory(pathToImport)) [[unlikely]] {
        return Error(std::format("expected the path \"{}\" to be a file", pathToImport.string()));
    }

    // Make sure the resulting directory exists.
    const auto pathToResultingDir =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToDirToImportRelativeRes;
    if (!std::filesystem::exists(pathToResultingDir)) [[unlikely]] {
        return Error(
            std::format("expected the resulting directory \"{}\" to exist", pathToResultingDir.string()));
    }
    if (!std::filesystem::is_directory(pathToResultingDir)) [[unlikely]] {
        return Error(
            std::format("expected the resulting path \"{}\" to be a directory", pathToResultingDir.string()));
    }

    const auto pathToResultingImage = pathToResultingDir / (pathToImport.stem().string() + ".png");

    // Load image pixels.
    int iWidth = 0;
    int iHeight = 0;
    int iChannels = 0;
    unsigned char* pPixels = stbi_load(pathToImport.string().c_str(), &iWidth, &iHeight, &iChannels, 0);
    if (pPixels == nullptr) [[unlikely]] {
        return Error(std::format("failed to load image from path \"{}\"", pathToImport.string()));
    }

    // Convert.
    if (!fpng::fpng_encode_image_to_file(
            pathToResultingImage.string().c_str(),
            pPixels,
            static_cast<unsigned int>(iWidth),
            static_cast<unsigned int>(iHeight),
            static_cast<unsigned int>(iChannels))) [[unlikely]] {
        stbi_image_free(pPixels);
        return Error(std::format("failed to import the image \"{}\"", pathToImport.string()));
    }

    stbi_image_free(pPixels);

    return {};
}

std::optional<Error> TextureManager::importTextureFromMemory(
    const std::string& sPathToResultRelativeRes,
    const std::vector<unsigned char>& vImageData,
    unsigned int iWidth,
    unsigned int iHeight,
    unsigned int iChannelCount) {
    // Make sure the resulting path does not exist.
    const auto pathToResult =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToResultRelativeRes;
    if (std::filesystem::exists(pathToResult)) [[unlikely]] {
        return Error(std::format("expected the resulting path \"{}\" to not exist", pathToResult.string()));
    }
    if (!std::filesystem::exists(pathToResult.parent_path())) [[unlikely]] {
        return Error(
            std::format("expected the directory \"{}\" to exist", pathToResult.parent_path().string()));
    }

    // Convert.
    if (!fpng::fpng_encode_image_to_file(
            pathToResult.string().c_str(), vImageData.data(), iWidth, iHeight, iChannelCount)) [[unlikely]] {
        return Error(std::format("failed to import the image to \"{}\"", pathToResult.string()));
    }

    return {};
}

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

    const auto sPathToTexture = pathToTexture.string();
    const auto iGlFormat = GL_RGBA;
    const auto iGlInternalFormat = iGlFormat;

    uint32_t iWidth = 0;
    uint32_t iHeight = 0;
    uint32_t iChannelCount = 0;
    std::vector<uint8_t> vPixels;
    const auto iImageLoadResult =
        fpng::fpng_decode_file(sPathToTexture.c_str(), vPixels, iWidth, iHeight, iChannelCount, 4);
    if (iImageLoadResult != fpng::FPNG_DECODE_SUCCESS) [[unlikely]] {
        if (iImageLoadResult == fpng::FPNG_DECODE_NOT_FPNG) {
            return Error(std::format(
                "failed to load the image \"{}\" because it was not imported using the engine's texture "
                "importer",
                sPathToTexture));
        }
        return Error(std::format(
            "an error occurred while loading the image \"{}\", error code: {}",
            sPathToTexture,
            iImageLoadResult));
    }

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
            vPixels.data()));

        if (usage == TextureUsage::DIFFUSE) {
            GL_CHECK_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
        }

        // Set texture wrapping.
        GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

        // Set texture filtering.
        if (bIsUsingPointFiltering) {
            if (usage == TextureUsage::DIFFUSE) {
                GL_CHECK_ERROR(
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR));
            } else {
                GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            }
            GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        } else {
            if (usage == TextureUsage::DIFFUSE) {
                GL_CHECK_ERROR(
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            } else {
                GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            }
            GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Add new resource to be considered.
    TextureResource resourceInfo;
    resourceInfo.iActiveTextureHandleCount = 0; // 0 because `createNewTextureHandle` will increment it
    resourceInfo.iTextureId = iTextureId;
    resourceInfo.usage = usage;
    mtxLoadedTextures.second[sPathToTextureRelativeRes] = resourceInfo;

    return createNewHandleForLoadedTexture(sPathToTextureRelativeRes, usage);
}
