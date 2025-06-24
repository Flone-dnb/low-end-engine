#pragma once

// Standard.
#include <utility>

/**
 * Groups OpenGL-related resources (such as framebuffer and textures) to draw on.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class Framebuffer {
    // Only GPU resource manager is expected to create objects of this type.
    friend class GpuResourceManager;

public:
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) noexcept = delete;
    Framebuffer& operator=(Framebuffer&&) noexcept = delete;

    ~Framebuffer();

    /**
     * Returns OpenGL ID of the framebuffer.
     *
     * @return OpenGL ID.
     */
    unsigned int getFramebufferId() const { return iFramebufferId; }

    /**
     * Returns OpenGL ID of the framebuffer's color texture.
     *
     * @return OpenGL ID.
     */
    unsigned int getColorTextureId() const { return iColorTextureId; }

    /**
     * Returns OpenGL ID of the framebuffer's depth/stencil texture.
     *
     * @return OpenGL ID.
     */
    unsigned int getDepthStencilTextureId() const { return iDepthStencilBufferId; }

    /**
     * Returns size of the framebuffer in pixels.
     *
     * @return Width and height in pixels.
     */
    std::pair<unsigned int, unsigned int> getSize() const { return size; }

private:
    /**
     * Initializes the object.
     *
     * @param iFramebufferId        ID of the framebuffer.
     * @param iColorTextureId       ID of the color texture used as a color attachment in @ref iFramebufferId.
     * @param iDepthStencilBufferId ID of the depth/stencil buffer in @ref iFramebufferId.
     * @param iWidth                Width of the framebuffer in pixels.
     * @param iHeight               Height of the framebuffer in pixels.
     */
    Framebuffer(
        unsigned int iFramebufferId,
        unsigned int iColorTextureId,
        unsigned int iDepthStencilBufferId,
        unsigned int iWidth,
        unsigned int iHeight);

    /** ID of the framebuffer. */
    const unsigned int iFramebufferId = 0;

    /** ID of the color texture used as a color attachment in @ref iFramebufferId. */
    const unsigned int iColorTextureId = 0;

    /** ID of the depth/stencil buffer in @ref iFramebufferId. */
    const unsigned int iDepthStencilBufferId = 0;

    /** Size (in pixels) of the framebuffer. */
    const std::pair<unsigned int, unsigned int> size = {0, 0};
};
