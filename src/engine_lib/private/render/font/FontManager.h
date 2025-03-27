#pragma once

// Standard.
#include <mutex>
#include <filesystem>
#include <memory>

// Custom.
#include "math/GLMath.hpp"

class Texture;
class Renderer;

/** Simplifies loading .ttf files from disk to the GPU memory. */
class FontManager {
    // Only renderer is expected to create objects if this type.
    friend class Renderer;

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

    /**
     * Returns size in range [0.0F; 1.0F] in which the font will be loaded.
     *
     * @return Size.
     */
    static constexpr float getFontSizeToLoad() { return fontSizeToLoad; }

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) noexcept = delete;
    FontManager& operator=(FontManager&&) noexcept = delete;

    /**
     * Creates a new font manager.
     *
     * @param pRenderer  Renderer.
     * @param pathToFont Path to a .ttf font to use.
     *
     * @return Created manager.
     */
    static std::unique_ptr<FontManager> create(Renderer* pRenderer, const std::filesystem::path& pathToFont);

    /**
     * Returns pairs of "character code" - "loaded glyph".
     *
     * @return Loaded glyphs.
     */
    std::pair<std::mutex, std::unordered_map<unsigned long, CharacterGlyph>>& getLoadedGlyphs() {
        return mtxLoadedGlyphs;
    }

private:
    /**
     * Used internally, see @ref create.
     *
     * @param pRenderer Renderer.
     */
    FontManager(Renderer* pRenderer);

    /**
     * Loads glyphs from the specified font to be used (clears previously loaded glyphs).
     *
     * @param pathToFont Path to .ttf file.
     */
    void loadFont(const std::filesystem::path& pathToFont);

    /** Pairs of "character code" - "loaded glyph". */
    std::pair<std::mutex, std::unordered_map<unsigned long, CharacterGlyph>> mtxLoadedGlyphs;

    /** Renderer. */
    Renderer* const pRenderer = nullptr;

    /**
     * Font size (relative to screen height) in range [0.0F; 1.0F] to load. We will scale this value when
     * drawing text nodes.
     *
     * @remark This value must be equal to an average size of the text, if it's too small
     * big text will be blurry, if it will be too big small text will look bad.
     */
    static constexpr float fontSizeToLoad = 0.12F;
};
