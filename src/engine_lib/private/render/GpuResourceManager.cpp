#include "GpuResourceManager.h"

// Standard.
#include <format>
#include <mutex>
#include <array>

// Custom.
#include "misc/Error.h"
#include "misc/Profiler.hpp"

// External.
#include "glad/glad.h"

std::mutex GpuResourceManager::mtx{};

std::unique_ptr<ScreenQuadGeometry> GpuResourceManager::createQuad(
    bool bIsVertexDataDynamic,
    std::optional<std::array<glm::vec3, ScreenQuadGeometry::iVertexCount>> vertexPositions) {
    PROFILE_FUNC

    // Prepare initial vertex buffer (full screen quad with positions in normalized device coordinates).
    std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount> vVertices = {
        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(-1.0F, -1.0F, 0.0F), .uv = glm::vec2(0.0F, 0.0F)},
        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(-1.0F, 1.0F, 0.0F), .uv = glm::vec2(0.0F, 1.0F)},
        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(1.0F, 1.0F, 0.0F), .uv = glm::vec2(1.0F, 1.0F)},

        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(-1.0F, -1.0F, 0.0F), .uv = glm::vec2(0.0F, 0.0F)},
        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(1.0F, 1.0F, 0.0F), .uv = glm::vec2(1.0F, 1.0F)},
        ScreenQuadGeometry::VertexLayout{
            .position = glm::vec3(1.0F, -1.0F, 0.0F), .uv = glm::vec2(1.0F, 0.0F)},
    };

    if (vertexPositions.has_value()) {
        for (size_t i = 0; i < vertexPositions->size(); i++) {
            vVertices[i].position = (*vertexPositions)[i];
        }
    }

    std::scoped_lock guard(mtx);

    unsigned int iVao = 0;
    unsigned int iVbo = 0;
    glGenVertexArrays(1, &iVao);
    glGenBuffers(1, &iVbo);

    glBindVertexArray(iVao);
    glBindBuffer(GL_ARRAY_BUFFER, iVbo);
    {
        // Allocate vertices.
        GL_CHECK_ERROR(glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<long long>(vVertices.size() * sizeof(ScreenQuadGeometry::VertexLayout)),
            vVertices.data(),
            bIsVertexDataDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

        // Position (XY) and UV (ZW).
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                 // attribute index (layout location)
            4,                 // number of components
            GL_FLOAT,          // type of component
            GL_FALSE,          // whether data should be normalized or not
            sizeof(glm::vec4), // stride (size in bytes between elements)
            0);                // beginning offset
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return std::unique_ptr<ScreenQuadGeometry>(new ScreenQuadGeometry(
        vVertices, std::unique_ptr<VertexArrayObject>(new VertexArrayObject(iVao, iVbo))));
}

std::unique_ptr<VertexArrayObject> GpuResourceManager::createVertexArrayObject(const MeshGeometry& geometry) {
    PROFILE_FUNC

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
    PROFILE_FUNC

    std::scoped_lock guard(mtx);

    unsigned int iFramebufferId = 0;
    unsigned int iColorTextureId = 0;
    unsigned int iDepthStencilBufferId = 0;

    // Create a framebuffer and textures.
    glGenFramebuffers(1, &iFramebufferId);
    glGenTextures(1, &iColorTextureId);
    if (iDepthGlFormat != 0) {
        glGenTextures(1, &iDepthStencilBufferId);
    }

    // Bind them to the target to update them.
    glBindFramebuffer(GL_FRAMEBUFFER, iFramebufferId);
    glBindTexture(GL_TEXTURE_2D, iColorTextureId);
    {
        // Configure and attach color texture to the framebuffer.
        GL_CHECK_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, iColorGlFormat, iWidth, iHeight));
        GL_CHECK_ERROR(glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iColorTextureId, 0));

        if (iDepthGlFormat != 0) {
            glBindTexture(GL_TEXTURE_2D, iDepthStencilBufferId);

            auto componentType = GL_UNSIGNED_INT;
            if (iDepthGlFormat == GL_DEPTH_COMPONENT16) {
                componentType = GL_UNSIGNED_SHORT;
            } else if (iDepthGlFormat == GL_DEPTH_COMPONENT32F) {
                componentType = GL_FLOAT;
            } else if (iDepthGlFormat == GL_DEPTH24_STENCIL8) {
                componentType = GL_UNSIGNED_INT_24_8;
            } else if (iDepthGlFormat == GL_DEPTH32F_STENCIL8) {
                componentType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
            }

            auto format = GL_DEPTH_COMPONENT;
            if (iDepthGlFormat == GL_DEPTH24_STENCIL8 || iDepthGlFormat == GL_DEPTH32F_STENCIL8) {
                format = GL_DEPTH_STENCIL;
            }

            // Configure depth texture.
            GL_CHECK_ERROR(glTexImage2D(
                GL_TEXTURE_2D, 0, iDepthGlFormat, iWidth, iHeight, 0, format, componentType, nullptr));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            auto attachment = GL_DEPTH_ATTACHMENT;
            if (iDepthGlFormat == GL_DEPTH24_STENCIL8 || iDepthGlFormat == GL_DEPTH32F_STENCIL8) {
                attachment = GL_DEPTH_STENCIL_ATTACHMENT;
            }

            // Attach to framebuffer.
            GL_CHECK_ERROR(
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, iDepthStencilBufferId, 0));
        }

        // Specify color texture to draw into.
        const std::array<GLenum, 1> vAttachments = {GL_COLOR_ATTACHMENT0};
        GL_CHECK_ERROR(glDrawBuffers(static_cast<int>(vAttachments.size()), vAttachments.data()));

        // Make sure framebuffer is complete.
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            Error::showErrorAndThrowException("render framebuffer is not complete");
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::unique_ptr<Framebuffer>(
        new Framebuffer(iFramebufferId, iColorTextureId, iDepthStencilBufferId));
}

std::unique_ptr<Buffer> GpuResourceManager::createUniformBuffer(unsigned int iSizeInBytes, bool bIsDynamic) {
    PROFILE_FUNC

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
