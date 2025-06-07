#pragma once

// Standard.
#include <mutex>
#include <filesystem>
#include <memory>
#include <vector>

// Custom.
#include "math/GLMath.hpp"

class Texture;
class Renderer;

typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

class FontGlyphsGuard;

/** Simplifies loading .ttf files from disk to the GPU memory. */
class FontManager {
    // Only renderer is expected to create objects if this type.
    friend class Renderer;

    // Loads new glyphs on demand.
    friend class FontGlyphsGuard;

public:
    /** Groups information about a loaded character glyph. */
    struct CharacterGlyph {
        /** Loaded texture. */
        std::unique_ptr<Texture> pTexture;

        /** Width and height of the texture in pixels. */
        glm::ivec2 size = glm::ivec2(0, 0);

        /** Offset from baseline to the top-left corner of the glyph. */
        glm::ivec2 bearing = glm::ivec2(0, 0);

        /** Horizontal offset (in 1/64 pixels) until the next glyph is reached. */
        unsigned int advance = 0;
    };

    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) noexcept = delete;
    FontManager& operator=(FontManager&&) noexcept = delete;

    /**
     * Loads glyphs from the specified font to be used (clears previously loaded glyphs).
     *
     * @param pathToFont       Font to load.
     * @param fontHeightToLoad Font height (relative to screen height, width is determines automatically) in
     * range [0.0F; 1.0F] to load. This value will be used as the base size but most likely will be scaled
     * when drawing text nodes according to the size of each text node. This value must be equal to an average
     * size of the text, if it's too small big text will be blurry, if it will be too big small text will look
     * bad.
     */
    void loadFont(const std::filesystem::path& pathToFont, float fontHeightToLoad = 0.1F);

    /**
     * Ensures the specified range of characters are loaded in the memory (does nothing if already loaded).
     *
     * @param characterCodeRange Pair of "index start" - "index end" (inclusive) to cache.
     */
    void cacheGlyphs(const std::pair<unsigned long, unsigned long>& characterCodeRange);

    /**
     * Returns an object that allows quering glyph information.
     *
     * @return RAII-style object.
     */
    FontGlyphsGuard getGlyphs();

    /**
     * Last specified font height to load from @ref loadFont.
     *
     * @return Font height.
     */
    float getFontHeightToLoad() const { return fontHeightToLoad; }

private:
    /**
     * Creates a new font manager.
     *
     * @param pRenderer  Renderer.
     *
     * @return Created manager.
     */
    static std::unique_ptr<FontManager> create(Renderer* pRenderer);

    /**
     * Used internally, see @ref create.
     *
     * @param pRenderer Renderer.
     */
    FontManager(Renderer* pRenderer);

    /** Called after window size changed. */
    void onWindowSizeChanged();

    /** Sets font size for glyphs that will be loaded. */
    void updateSizeForNextGlyphs();

    /** Pairs of "character code" - "loaded glyph". */
    std::pair<std::recursive_mutex, std::unordered_map<unsigned long, CharacterGlyph>> mtxLoadedGlyphs;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;

    /** Last specified font height to load from @ref loadFont. */
    float fontHeightToLoad = 0.0F;

    /** FreeType library. */
    FT_Library pFtLibrary = nullptr;

    /** Currently loaded font face. */
    FT_Face pFtFace = nullptr;

    /** Currently used font. */
    std::filesystem::path pathToFont;
};

/** RAII-style type that allows quering glyphs textures. */
class FontGlyphsGuard {
    // Only font manager is allowed to create objects of this type.
    friend class FontManager;

public:
    FontGlyphsGuard() = delete;
    FontGlyphsGuard(const FontGlyphsGuard&) = delete;
    FontGlyphsGuard& operator=(const FontGlyphsGuard&) = delete;
    FontGlyphsGuard(FontGlyphsGuard&&) = delete;
    FontGlyphsGuard& operator=(FontGlyphsGuard&&) = delete;

    ~FontGlyphsGuard() { pManager->mtxLoadedGlyphs.first.unlock(); }

    /**
     * Loads a glyph with the specified character code or just returns it if it was previously requested.
     *
     * @param iCharacterCode Index of the char to load.
     *
     * @return Loaded glyph.
     */
    const FontManager::CharacterGlyph& getGlyph(unsigned long iCharacterCode);

private:
    /**
     * Creates a new object.
     *
     * @param pManager Font manager.
     */
    FontGlyphsGuard(FontManager* pManager) : pManager(pManager) { pManager->mtxLoadedGlyphs.first.lock(); }

    /** Font manager. */
    FontManager* const pManager = nullptr;
};
