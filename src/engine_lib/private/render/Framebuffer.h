#pragma once

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
     * Returns ID of the framebuffer.
     *
     * @return ID.
     */
    unsigned int getFramebufferId() const { return iFramebufferId; }

private:
    /**
     * Initializes the object.
     *
     * @param iFramebufferId        ID of the framebuffer.
     * @param iColorTextureId       ID of the color texture used as a color attachment in @ref iFramebufferId.
     * @param iDepthStencilBufferId ID of the depth/stencil buffer in @ref iFramebufferId.
     */
    Framebuffer(
        unsigned int iFramebufferId, unsigned int iColorTextureId, unsigned int iDepthStencilBufferId);

    /** ID of the framebuffer. */
    unsigned int iFramebufferId = 0;

    /** ID of the color texture used as a color attachment in @ref iFramebufferId. */
    unsigned int iColorTextureId = 0;

    /** ID of the depth/stencil buffer in @ref iFramebufferId. */
    unsigned int iDepthStencilBufferId = 0;
};
