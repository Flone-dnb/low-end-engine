#pragma once

// Standard.
#include <new>
#include <string>

// Custom.
#include "math/GLMath.hpp"
#include "render/UiLayer.hpp"

/// @cond UNDOCUMENTED

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
#else
constexpr std::size_t hardware_constructive_interference_size = 64;
#endif

// ------------------------------------------------------------------------------------------------

class TextRenderingHandle;

/** Groups data needed to submit text for drawing. */
struct alignas(hardware_constructive_interference_size) TextRenderData {
    /** Color of the text in the RGBA format. */
    glm::vec4 textColor;

    /** Used by the manager to update handle's render data index. */
    TextRenderingHandle* pHandle = nullptr;

    /** Pointer to text to display. */
    std::u16string* pText = nullptr;

    /** Top-left corner in range [0.0; 1.0] relative to screen. */
    glm::vec2 pos;

    /** Size in range [0.0; 1.0] relative to screen size. */
    glm::vec2 size;

    /** Height of the text in range [0.0; 1.0] relative to screen height. */
    float textHeight = 0.035F;

    /**
     * Vertical space between horizontal lines of text, in range [0.0F; +inf]
     * proportional to the height of the text.
     */
    float lineSpacing = 0.1F;

    /** `true` to automatically transfer text to a new line if it does not fit in a single line. */
    bool bIsWordWrapEnabled = false;

    /** `true` to allow `\n` characters in the text to create new lines. */
    bool bHandleNewLineChars = true;
};
static_assert(sizeof(TextRenderData) == hardware_constructive_interference_size);

/// @endcond

class UiNodeManager;

// ------------------------------------------------------------------------------------------------

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
