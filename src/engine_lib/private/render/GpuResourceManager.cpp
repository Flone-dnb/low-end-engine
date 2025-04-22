#include "GpuResourceManager.h"

// Standard.
#include <format>
#include <mutex>
#include <array>

// Custom.
#include "misc/Error.h"

// External.
#include "glad/glad.h"

std::mutex GpuResourceManager::mtx{};

std::unique_ptr<VertexArrayObject> GpuResourceManager::createVertexArrayObject(const MeshGeometry& geometry) {
    std::scoped_lock guard(mtx);

    // Create VAO.
    unsigned int iVertexArrayObjectId = 0;
    glGenVertexArrays(1, &iVertexArrayObjectId);
    glBindVertexArray(iVertexArrayObjectId);

    // Create VBO.
    unsigned int iVertexBufferObjectId = 0;
    glGenBuffers(1, &iVertexBufferObjectId);
    glBindBuffer(GL_ARRAY_BUFFER, iVertexBufferObjectId);

    // Copy vertices to the vertex buffer.
    GL_CHECK_ERROR(glBufferData(
        GL_ARRAY_BUFFER,
        geometry.getVertices().size() * sizeof(geometry.getVertices()[0]),
        geometry.getVertices().data(),
        GL_STATIC_DRAW));
    MeshVertex::setVertexAttributes();

    // Before converting index count to int (for OpenGL) make sure the conversion will be safe.
    constexpr size_t iTypeLimit = std::numeric_limits<int>::max();
    if (geometry.getIndices().size() > iTypeLimit) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("index count {} exceeds type limit of {}", geometry.getIndices().size(), iTypeLimit));
    }
    int iIndexCount = static_cast<int>(geometry.getIndices().size());

    // Create EBO.
    unsigned int iIndexBufferObjectId = 0;
    glGenBuffers(1, &iIndexBufferObjectId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iIndexBufferObjectId);

    // Copy indices to the buffer.
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        geometry.getIndices().size() * sizeof(geometry.getIndices()[0]),
        geometry.getIndices().data(),
        GL_STATIC_DRAW);

    // Done.
    glBindVertexArray(0);

    return std::unique_ptr<VertexArrayObject>(new VertexArrayObject(
        iVertexArrayObjectId, iVertexBufferObjectId, iIndexBufferObjectId, iIndexCount));
}

std::unique_ptr<Framebuffer> GpuResourceManager::createFramebuffer(
    unsigned int iWidth, unsigned int iHeight, int iColorGlFormat, int iDepthGlFormat) {
    std::scoped_lock guard(mtx);

    unsigned int iFramebufferId = 0;
    unsigned int iColorTextureId = 0;
    unsigned int iDepthStencilBufferId = 0;

    // Create a framebuffer, a color texture and a depth/stencil buffer.
    glGenFramebuffers(1, &iFramebufferId);
    glGenTextures(1, &iColorTextureId);
    glGenRenderbuffers(1, &iDepthStencilBufferId);

    // Bind them to the target to update them.
    glBindFramebuffer(GL_FRAMEBUFFER, iFramebufferId);
    glBindTexture(GL_TEXTURE_2D, iColorTextureId);
    glBindRenderbuffer(GL_RENDERBUFFER, iDepthStencilBufferId);
    {
        // Configure and attach color texture to the framebuffer.
        GL_CHECK_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, iColorGlFormat, iWidth, iHeight));
        GL_CHECK_ERROR(glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iColorTextureId, 0));

        auto attachment = GL_DEPTH_ATTACHMENT;
        if (iDepthGlFormat == GL_DEPTH24_STENCIL8 || iDepthGlFormat == GL_DEPTH32F_STENCIL8) {
            attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        }

        // Configure and attach depth/stencil buffer to the framebuffer.
        GL_CHECK_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, iDepthGlFormat, iWidth, iHeight));
        GL_CHECK_ERROR(
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, iDepthStencilBufferId));

        // Specify color texture to draw into.
        const std::array<GLenum, 1> vAttachments = {GL_COLOR_ATTACHMENT0};
        GL_CHECK_ERROR(glDrawBuffers(static_cast<int>(vAttachments.size()), vAttachments.data()));

        // Make sure framebuffer is complete.
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            Error::showErrorAndThrowException("render framebuffer is not complete");
        }
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::unique_ptr<Framebuffer>(
        new Framebuffer(iFramebufferId, iColorTextureId, iDepthStencilBufferId));
}

std::unique_ptr<Buffer> GpuResourceManager::createUniformBuffer(unsigned int iSizeInBytes, bool bIsDynamic) {
    std::scoped_lock guard(mtx);

    unsigned int iBufferId = 0;
    glGenBuffers(1, &iBufferId);

    // Allocate buffer.
    glBindBuffer(GL_UNIFORM_BUFFER, iBufferId);
    {
        GL_CHECK_ERROR(glBufferData(
            GL_UNIFORM_BUFFER, iSizeInBytes, nullptr, bIsDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return std::unique_ptr<Buffer>(new Buffer(iSizeInBytes, iBufferId, GL_UNIFORM_BUFFER, bIsDynamic));
}
