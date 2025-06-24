#include "render/wrapper/Framebuffer.h"

// Custom.
#include "misc/Error.h"

// External.
#include "glad/glad.h"

Framebuffer::Framebuffer(
    unsigned int iFramebufferId,
    unsigned int iColorTextureId,
    unsigned int iDepthStencilBufferId,
    unsigned int iWidth,
    unsigned int iHeight)
    : iFramebufferId(iFramebufferId), iColorTextureId(iColorTextureId),
      iDepthStencilBufferId(iDepthStencilBufferId), size({iWidth, iHeight}) {}

Framebuffer::~Framebuffer() {
    GL_CHECK_ERROR(glDeleteFramebuffers(1, &iFramebufferId));
    GL_CHECK_ERROR(glDeleteTextures(1, &iColorTextureId));
    GL_CHECK_ERROR(glDeleteRenderbuffers(1, &iDepthStencilBufferId));
}
