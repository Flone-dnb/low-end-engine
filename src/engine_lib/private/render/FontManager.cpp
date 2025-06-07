#include "render/FontManager.h"

// Standard.
#include <format>

// Custom.
#include "misc/Error.h"
#include "render/Renderer.h"
#include "game/Window.h"
#include "render/wrapper/Texture.h"
#include "render/GpuResourceManager.h"
#include "misc/ProjectPaths.h"
#include "misc/Profiler.hpp"

// External.
#include "glad/glad.h"
#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmm.h"

std::unique_ptr<FontManager> FontManager::create(Renderer* pRenderer) {
    return std::unique_ptr<FontManager>(new FontManager(pRenderer));
}

FontManager::FontManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    // Create library.
    const auto iErrorCode = FT_Init_FreeType(&pFtLibrary);
    if (iErrorCode != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to create FreeType library, error: {}", iErrorCode));
    }
}

FontManager::~FontManager() {
    if (pFtFace != nullptr) {
        // Destroy face.
        const auto iErrorCode = FT_Done_Face(pFtFace);
        if (iErrorCode != 0) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to destroy FreeType face, error: {}", iErrorCode));
        }
    }

    if (pFtLibrary != nullptr) {
        // Destroy library.
        const auto iErrorCode = FT_Done_FreeType(pFtLibrary);
        if (iErrorCode != 0) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to destroy FreeType library, error: {}", iErrorCode));
        }
    }
}

void FontManager::loadFont(const std::filesystem::path& pathToFont, float fontHeightToLoad) {
    if (!std::filesystem::exists(pathToFont)) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("the specified path to font {} does not exist", pathToFont.string()));
    }

    fontHeightToLoad = std::clamp(fontHeightToLoad, 0.0F, 1.0F);
    this->fontHeightToLoad = fontHeightToLoad;
    this->pathToFont = pathToFont;

    std::scoped_lock guard(mtxLoadedGlyphs.first);
    mtxLoadedGlyphs.second.clear(); // unload all previously loaded glyphs

    if (pFtFace != nullptr) {
        // Unload previously loaded face.
        const auto iErrorCode = FT_Done_Face(pFtFace);
        if (iErrorCode != 0) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to destroy FreeType face, error: {}", iErrorCode));
        }
    }

    // Create face.
    auto iErrorCode = FT_New_Face(pFtLibrary, pathToFont.string().c_str(), 0, &pFtFace);
    if (iErrorCode != 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create FreeType face from the font \"{}\", error: {}",
            pathToFont.filename().string(),
            iErrorCode));
    }

    updateSizeForNextGlyphs();

    // Cache ASCII glyphs.
    cacheGlyphs({32, 126});
}

void FontManager::cacheGlyphs(const std::pair<unsigned long, unsigned long>& characterCodeRange) {
    PROFILE_FUNC;

    if (characterCodeRange.first > characterCodeRange.second ||
        characterCodeRange.second < characterCodeRange.first) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("char range {}-{} is invalid", characterCodeRange.first, characterCodeRange.second));
    }

    std::scoped_lock gpuGuard(mtxLoadedGlyphs.first, GpuResourceManager::mtx);

    // Set byte-alignment to 1 because we will create single-channel textures.
    int iPreviousUnpackAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &iPreviousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    {
        for (unsigned long iCharCode = characterCodeRange.first; iCharCode <= characterCodeRange.second;
             iCharCode++) {
            const auto charIt = mtxLoadedGlyphs.second.find(iCharCode);
            if (charIt != mtxLoadedGlyphs.second.end()) {
                // Already cached.
                continue;
            }

            // Load glyph.
            auto iErrorCode = FT_Load_Char(pFtFace, iCharCode, FT_LOAD_RENDER);
            if (iErrorCode != 0) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("failed to load character {}, error: {}", iCharCode, iErrorCode));
            }

            unsigned int iTextureId = 0;
            glGenTextures(1, &iTextureId);

            // Load texture.
            glBindTexture(GL_TEXTURE_2D, iTextureId);
            {
                GL_CHECK_ERROR(glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    pFtFace->glyph->bitmap.width,
                    pFtFace->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    pFtFace->glyph->bitmap.buffer));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            // Save.
            mtxLoadedGlyphs.second[iCharCode] = CharacterGlyph{
                .pTexture = std::unique_ptr<Texture>(new Texture(iTextureId)),
                .size = glm::ivec2(pFtFace->glyph->bitmap.width, pFtFace->glyph->bitmap.rows),
                .bearing = glm::ivec2(pFtFace->glyph->bitmap_left, pFtFace->glyph->bitmap_top),
                .advance = static_cast<unsigned int>(pFtFace->glyph->advance.x)};
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, iPreviousUnpackAlignment);
}

FontGlyphsGuard FontManager::getGlyphs() {
    if (pFtFace == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("font is not loaded");
    }
    return FontGlyphsGuard(this);
}

void FontManager::updateSizeForNextGlyphs() {
    if (pFtFace == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected FreeType face to be valid");
    }

    const auto iFontHeightInPixels = static_cast<unsigned int>(
        static_cast<float>(pRenderer->getWindow()->getWindowSize().second) * fontHeightToLoad);
    FT_Set_Pixel_Sizes(pFtFace, 0, iFontHeightInPixels); // 0 for width to automatically determine it
}

void FontManager::onWindowSizeChanged() {
    std::scoped_lock guard(mtxLoadedGlyphs.first);
    mtxLoadedGlyphs.second.clear(); // unload all previously loaded glyphs

    if (pFtFace != nullptr) {
        updateSizeForNextGlyphs();
    }
}

const FontManager::CharacterGlyph& FontGlyphsGuard::getGlyph(unsigned long iCharacterCode) {
    PROFILE_FUNC;

    // mutex is already locked

    // Find in cache.
    auto charIt = pManager->mtxLoadedGlyphs.second.find(iCharacterCode);
    if (charIt != pManager->mtxLoadedGlyphs.second.end()) {
        return charIt->second;
    }

    // Load.
    pManager->cacheGlyphs({iCharacterCode, iCharacterCode});

    // Query again.
    charIt = pManager->mtxLoadedGlyphs.second.find(iCharacterCode);
    if (charIt == pManager->mtxLoadedGlyphs.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("expected the glyph {} to be loaded", iCharacterCode));
    }

    return charIt->second;
}
