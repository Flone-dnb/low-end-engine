#include "GpuResourceManager.h"

// Standard.
#include <format>
#include <mutex>
#include <array>

// Custom.
#include "misc/Error.h"
#include "misc/Profiler.hpp"
#include "render/wrapper/Texture.h"

// External.
#include "glad/glad.h"

std::recursive_mutex GpuResourceManager::mtx{};

std::unique_ptr<VertexArrayObject> GpuResourceManager::createVertexArrayObject(
    bool bIsVertexDataDynamic,
    const std::vector<glm::vec3>& vVertexPositions,
    const std::vector<unsigned short>& vIndices) {
    if (vVertexPositions.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("you must specify at least 1 position");
    }
    if (vVertexPositions.empty() && !bIsVertexDataDynamic) [[unlikely]] {
        Error::showErrorAndThrowException(
            "initial data must be specified because vertex data is not marked as dynamic");
    }

    std::scoped_lock guard(mtx);

    unsigned int iVao = 0;
    unsigned int iVbo = 0;
    std::optional<unsigned int> optionalEbo;
    std::optional<int> optionalIndexCount;
    glGenVertexArrays(1, &iVao);
    glGenBuffers(1, &iVbo);

    glBindVertexArray(iVao);
    {
        // Allocate vertices.
        glBindBuffer(GL_ARRAY_BUFFER, iVbo);
        GL_CHECK_ERROR(glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<long long>(vVertexPositions.size() * sizeof(vVertexPositions[0])),
            vVertexPositions.data(),
            bIsVertexDataDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

        // Describe vertex layout.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                 // attribute index (layout location)
            3,                 // number of components
            GL_FLOAT,          // type of component
            GL_FALSE,          // whether data should be normalized or not
            sizeof(glm::vec3), // stride (size in bytes between elements)
            0);                // beginning offset
    }
    if (!vIndices.empty()) {
        // Create EBO.
        unsigned int iEbo = 0;
        glGenBuffers(1, &iEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iEbo);
        optionalEbo = iEbo;
        optionalIndexCount = static_cast<int>(vIndices.size());

        // Allocate indices.
        GL_CHECK_ERROR(glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(vIndices[0]), vIndices.data(), GL_STATIC_DRAW));
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return std::unique_ptr<VertexArrayObject>(new VertexArrayObject(
        iVao, iVbo, static_cast<unsigned int>(vVertexPositions.size()), optionalEbo, optionalIndexCount));
}

std::unique_ptr<ScreenQuadGeometry> GpuResourceManager::createScreenQuad(
    std::optional<std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount>> vertexData,
    std::optional<std::array<unsigned short, ScreenQuadGeometry::iIndexCount>> indexData) {
    PROFILE_FUNC

    // Prepare initial vertex buffer (full screen quad with positions in normalized device coordinates).
    std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount> vVertices = {
        ScreenQuadGeometry::VertexLayout{.position = glm::vec2(1.0f, 1.0f), .uv = glm::vec2(1.0f, 1.0f)},
        ScreenQuadGeometry::VertexLayout{.position = glm::vec2(-1.0f, 1.0f), .uv = glm::vec2(0.0f, 1.0f)},
        ScreenQuadGeometry::VertexLayout{.position = glm::vec2(-1.0f, -1.0f), .uv = glm::vec2(0.0f, 0.0f)},

        ScreenQuadGeometry::VertexLayout{.position = glm::vec2(1.0f, -1.0f), .uv = glm::vec2(1.0f, 0.0f)}};

    std::array<unsigned short, ScreenQuadGeometry::iIndexCount> vIndices = {0, 1, 2, 3, 0, 2};

    if (vertexData.has_value()) {
        vVertices = *vertexData;
    }
    if (indexData.has_value()) {
        vIndices = *indexData;
    }

    std::scoped_lock guard(mtx);

    unsigned int iVao = 0;
    unsigned int iVbo = 0;
    unsigned int iEbo = 0;
    glGenVertexArrays(1, &iVao);
    glGenBuffers(1, &iVbo);
    glGenBuffers(1, &iEbo);

    glBindVertexArray(iVao);
    {
        {
            // Allocate vertices.
            glBindBuffer(GL_ARRAY_BUFFER, iVbo);
            GL_CHECK_ERROR(glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<long long>(vVertices.size() * sizeof(ScreenQuadGeometry::VertexLayout)),
                vVertices.data(),
                GL_STATIC_DRAW));

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

        {
            // Allocate indices.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iEbo);
            GL_CHECK_ERROR(glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                vIndices.size() * sizeof(vIndices[0]),
                vIndices.data(),
                GL_STATIC_DRAW));
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return std::unique_ptr<ScreenQuadGeometry>(new ScreenQuadGeometry(
        vVertices,
        std::unique_ptr<VertexArrayObject>(new VertexArrayObject(
            iVao,
            iVbo,
            static_cast<unsigned int>(vVertices.size()),
            iEbo,
            static_cast<int>(vIndices.size())))));
}

std::unique_ptr<VertexArrayObject>
GpuResourceManager::createVertexArrayObject(const MeshNodeGeometry& geometry) {
    PROFILE_FUNC

    if (geometry.getVertices().empty() || geometry.getIndices().empty()) [[unlikely]] {
        Error::showErrorAndThrowException("expected mesh geometry to be not empty");
    }

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
    MeshNodeVertex::setVertexAttributes();

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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return std::unique_ptr<VertexArrayObject>(new VertexArrayObject(
        iVertexArrayObjectId,
        iVertexBufferObjectId,
        static_cast<unsigned int>(geometry.getVertices().size()),
        iIndexBufferObjectId,
        iIndexCount));
}

std::unique_ptr<VertexArrayObject>
GpuResourceManager::createVertexArrayObject(const SkeletalMeshNodeGeometry& geometry) {
    PROFILE_FUNC

    if (geometry.getVertices().empty() || geometry.getIndices().empty()) [[unlikely]] {
        Error::showErrorAndThrowException("expected mesh geometry to be not empty");
    }

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
    SkeletalMeshNodeVertex::setVertexAttributes();

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
        iVertexArrayObjectId,
        iVertexBufferObjectId,
        static_cast<unsigned int>(geometry.getVertices().size()),
        iIndexBufferObjectId,
        iIndexCount));
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
    if (iColorGlFormat != 0) {
        glGenTextures(1, &iColorTextureId);
    }
    if (iDepthGlFormat != 0) {
        glGenTextures(1, &iDepthStencilBufferId);
    }

    // Bind them to the target to update them.
    glBindFramebuffer(GL_FRAMEBUFFER, iFramebufferId);
    {
        if (iColorGlFormat != 0) {
            glBindTexture(GL_TEXTURE_2D, iColorTextureId);

            GL_CHECK_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, iColorGlFormat, iWidth, iHeight));
            GL_CHECK_ERROR(glFramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iColorTextureId, 0));
        }

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

        if (iColorGlFormat != 0) {
            // Specify color texture to draw to.
            const std::array<GLenum, 1> vAttachments = {GL_COLOR_ATTACHMENT0};
            GL_CHECK_ERROR(glDrawBuffers(static_cast<int>(vAttachments.size()), vAttachments.data()));
        } else {
            GL_CHECK_ERROR(glDrawBuffers(GL_NONE, nullptr));
            GL_CHECK_ERROR(glReadBuffer(GL_NONE));
        }

        // Make sure the framebuffer is complete.
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            Error::showErrorAndThrowException("framebuffer is not complete");
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::unique_ptr<Framebuffer>(
        new Framebuffer(iFramebufferId, iColorTextureId, iDepthStencilBufferId, iWidth, iHeight));
}

std::unique_ptr<Framebuffer>
GpuResourceManager::createShadowMapFramebuffer(Texture& shadowMapArray, unsigned int iTextureIndex) {
    unsigned int iFramebufferId = 0;
    glGenFramebuffers(1, &iFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, iFramebufferId);
    {
        GL_CHECK_ERROR(glFramebufferTextureLayer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapArray.getTextureId(), 0, iTextureIndex));

        GL_CHECK_ERROR(glDrawBuffers(GL_NONE, nullptr));
        GL_CHECK_ERROR(glReadBuffer(GL_NONE));

        // Make sure the framebuffer is complete.
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            Error::showErrorAndThrowException("framebuffer is not complete");
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::unique_ptr<Framebuffer>(new Framebuffer(
        iFramebufferId,
        0,
        shadowMapArray.getTextureId(),
        shadowMapArray.getSize().first,
        shadowMapArray.getSize().second));
}

std::unique_ptr<Texture> GpuResourceManager::createTextureArray(
    unsigned int iWidth, unsigned int iHeight, int iGlFormat, unsigned int iArraySize, bool bIsShadowMaps) {
    unsigned int iTexArrayId = 0;
    glGenTextures(1, &iTexArrayId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, iTexArrayId);

    GL_CHECK_ERROR(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, iGlFormat, iWidth, iHeight, iArraySize));

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (bIsShadowMaps) {
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        // for hardware PCF (does 4 samples):
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    return std::unique_ptr<Texture>(new Texture(iTexArrayId, iWidth, iHeight, iGlFormat));
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

std::unique_ptr<Buffer> GpuResourceManager::createStorageBuffer(unsigned int iSizeInBytes) {
    PROFILE_FUNC

    std::scoped_lock guard(mtx);

    unsigned int iBufferId = 0;
    glGenBuffers(1, &iBufferId);

    // Allocate buffer.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, iBufferId);
    {
        GL_CHECK_ERROR(glBufferData(GL_SHADER_STORAGE_BUFFER, iSizeInBytes, nullptr, GL_DYNAMIC_READ));
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return std::unique_ptr<Buffer>(new Buffer(iSizeInBytes, iBufferId, GL_SHADER_STORAGE_BUFFER, false));
}

std::unique_ptr<Texture>
GpuResourceManager::createStorageTexture(unsigned int iWidth, unsigned int iHeight, int iFormat) {
    PROFILE_FUNC

    std::scoped_lock guard(mtx);

    unsigned int iTextureId = 0;
    glGenTextures(1, &iTextureId);
    glBindTexture(GL_TEXTURE_2D, iTextureId);
    {
        GL_CHECK_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, iFormat, iWidth, iHeight));

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return std::unique_ptr<Texture>(new Texture(iTextureId, iWidth, iHeight, iFormat));
}
