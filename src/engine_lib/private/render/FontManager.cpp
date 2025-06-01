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

// External.
#include "glad/glad.h"
#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmm.h"

FontManager::~FontManager() {}

std::unique_ptr<FontManager> FontManager::create(Renderer* pRenderer) {
    return std::unique_ptr<FontManager>(new FontManager(pRenderer));
}

FontManager::FontManager(Renderer* pRenderer) : pRenderer(pRenderer) {}

void FontManager::loadGlyphs(std::vector<FontLoadInfo> vFontsToLoad) {
    if (vFontsToLoad.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("at least 1 font must be specified");
    }

    std::scoped_lock guard(mtxLoadedGlyphs.first);
    mtxLoadedGlyphs.second.clear();

    // Create library.
    FT_Library pLibrary = nullptr;
    auto iErrorCode = FT_Init_FreeType(&pLibrary);
    if (iErrorCode != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to create FreeType library, error: {}", iErrorCode));
    }

    // Make sure glyph for unknown char will be loaded.
    bool bUnknownCharGlyphWillBeLoaded = false;
    for (const auto& fontInfo : vFontsToLoad) {
        for (const auto& [iStart, iEnd] : fontInfo.charCodesToLoad) {
            if (iUnknownCharGlyphCode >= iStart && iUnknownCharGlyphCode <= iEnd) {
                bUnknownCharGlyphWillBeLoaded = true;
                break;
            }
        }
        if (bUnknownCharGlyphWillBeLoaded) {
            break;
        }
    }
    if (!bUnknownCharGlyphWillBeLoaded) {
        vFontsToLoad.push_back(FontLoadInfo{
            .pathToFont = ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" /
                          "RedHatDisplay-Light.ttf",
            .charCodesToLoad = {{iUnknownCharGlyphCode, iUnknownCharGlyphCode}}});
    }

    for (const auto& fontInfo : vFontsToLoad) {
        if (!std::filesystem::exists(fontInfo.pathToFont)) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("path \"{}\" does not exist", fontInfo.pathToFont.string()));
        }

        // Create face.
        FT_Face pFace = nullptr;
        iErrorCode = FT_New_Face(pLibrary, fontInfo.pathToFont.string().c_str(), 0, &pFace);
        if (iErrorCode != 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to create face from the font \"{}\", error: {}",
                fontInfo.pathToFont.filename().string(),
                iErrorCode));
        }

        // Select font size.
        const auto iFontHeightInPixels = static_cast<unsigned int>(
            static_cast<float>(pRenderer->getWindow()->getWindowSize().second) * fontHeightToLoad);
        FT_Set_Pixel_Sizes(pFace, 0, iFontHeightInPixels); // 0 for width to automatically determine it

        {
            std::scoped_lock guard(GpuResourceManager::mtx);

            // Set byte-alignment to 1 because we will create single-channel textures.
            int iPreviousUnpackAlignment = 0;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &iPreviousUnpackAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            {
                // Load character ranges.
                for (const auto& [iCodeStart, iCodeEnd] : fontInfo.charCodesToLoad) {
                    if (iCodeStart > iCodeEnd) [[unlikely]] {
                        Error::showErrorAndThrowException(std::format(
                            "the specified character range [{} - {}] is invalid", iCodeStart, iCodeEnd));
                    }

                    for (unsigned long i = iCodeStart; i <= iCodeEnd; i++) {
                        // Load glyph.
                        auto iErrorCode = FT_Load_Char(pFace, i, FT_LOAD_RENDER);
                        if (iErrorCode != 0) [[unlikely]] {
                            Error::showErrorAndThrowException(
                                std::format("failed to load character {}, error: {}", i, iErrorCode));
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
                                pFace->glyph->bitmap.width,
                                pFace->glyph->bitmap.rows,
                                0,
                                GL_RED,
                                GL_UNSIGNED_BYTE,
                                pFace->glyph->bitmap.buffer));

                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        }
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Save.
                        mtxLoadedGlyphs.second[i] = CharacterGlyph{
                            .pTexture = std::unique_ptr<Texture>(new Texture(iTextureId)),
                            .size = glm::ivec2(pFace->glyph->bitmap.width, pFace->glyph->bitmap.rows),
                            .bearing = glm::ivec2(pFace->glyph->bitmap_left, pFace->glyph->bitmap_top),
                            .advance = static_cast<unsigned int>(pFace->glyph->advance.x)};
                    }
                }
            }
            glPixelStorei(GL_UNPACK_ALIGNMENT, iPreviousUnpackAlignment);
        }

        // Destroy face.
        iErrorCode = FT_Done_Face(pFace);
        if (iErrorCode != 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("failed to destroy face, error: {}", iErrorCode));
        }
    }

    // Destroy library.
    iErrorCode = FT_Done_FreeType(pLibrary);
    if (iErrorCode != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to destroy FreeType library, error: {}", iErrorCode));
    }
}
