#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "render/UiLayer.hpp"

class TextRenderingHandle;

/** Data used to submit a text glyph for drawing. */
struct GlythRenderData {
    /** Offset relative to the pivot of the text. */
    glm::vec2 relativePos;

    /** Size in pixels. */
    glm::vec2 screenSize;

    /** Glyph texture ID. */
    unsigned int iTextureId = 0;
};

/** Groups data needed to submit text for drawing. */
struct TextRenderData {
    /** Glyphs to submit for drawing. */
    std::vector<GlythRenderData> vGlyphs;

    /** Color of the text in the RGBA format. */
    glm::vec4 textColor;

    /** Used by the manager to update handle's render data index. */
    TextRenderingHandle* pHandle = nullptr;

    /** Top-left corner of the text in range [0.0; 1.0] relative to screen. */
    glm::vec2 pos;

    /** Y-axis clip rect (start and size). */
    glm::vec2 yClip;
};

// ------------------------------------------------------------------------------------------------

class UiNodeManager;

/**
 * While you hold an object of this type the text will be rendered,
 * if you destroy this handle the text will be removed from the rendering.
 */
class TextRenderingHandle {
    // UI node manager provides a handle when you register a text to be rendered.
    friend class UiNodeManager;

public:
    TextRenderingHandle() = delete;
    ~TextRenderingHandle();

    TextRenderingHandle(const TextRenderingHandle&) = delete;
    TextRenderingHandle& operator=(const TextRenderingHandle&) = delete;

private:
    /**
     * Creates a new handle.
     *
     * @param pManager   Manager.
     * @param uiLayer    Layer of the UI object this handle references.
     * @param iRenderDataIndex Index into the render data array.
     */
    TextRenderingHandle(UiNodeManager* pManager, UiLayer uiLayer, unsigned short iRenderDataIndex)
        : pManager(pManager), uiLayer(uiLayer), iRenderDataIndex(iRenderDataIndex) {}

    /** Object that created this handle. */
    UiNodeManager* const pManager = nullptr;

    /** Layer of the UI object this handle references. */
    const UiLayer uiLayer = UiLayer::LAYER1;

    /**
     * Index into the render data array.
     * This index can be changed by the manager.
     */
    unsigned short iRenderDataIndex = 0;
};

// ------------------------------------------------------------------------------------------------

/** RAII-style type that keeps render data locked while exists. */
class TextRenderDataGuard {
    // Only renderer is allowed create objects of this type.
    friend class UiNodeManager;

public:
    TextRenderDataGuard() = delete;
    ~TextRenderDataGuard();

    TextRenderDataGuard(const TextRenderDataGuard&) = delete;
    TextRenderDataGuard& operator=(const TextRenderDataGuard&) = delete;

    TextRenderDataGuard(TextRenderDataGuard&) noexcept = delete;
    TextRenderDataGuard& operator=(TextRenderDataGuard&) noexcept = delete;

    /**
     * Returns mesh data to modify.
     *
     * @return Data.
     */
    TextRenderData& getData() { return *pData; }

private:
    /**
     * Creates a new object.
     *
     * @remark Expects that the render data mutex (in the manager) is already locked.
     *
     * @param pManager Manager.
     * @param pData Data to modify.
     */
    TextRenderDataGuard(UiNodeManager* pManager, TextRenderData* pData) : pData(pData), pManager(pManager) {};

    /** Data to modify. */
    TextRenderData* const pData = nullptr;

    /** Manager. */
    UiNodeManager* const pManager = nullptr;
};
