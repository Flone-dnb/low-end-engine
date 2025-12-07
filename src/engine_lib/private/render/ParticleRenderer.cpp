#include "render/ParticleRenderer.h"

// Standard.
#include <limits>
#include <format>

// Custom.
#include "misc/Error.h"
#include "io/Log.h"
#include "render/RenderingHandle.h"
#include "render/GpuResourceManager.h"
#include "misc/Profiler.hpp"
#include "render/ShaderManager.h"
#include "render/Renderer.h"
#include "game/DebugConsole.h"
#include "render/wrapper/ShaderProgram.h"

// External.
#include "SDL3/SDL.h"
#include "glad/glad.h"

ParticleEmitterRenderDataGuard::~ParticleEmitterRenderDataGuard() {
    pParticleRenderer->onParticleRenderDataChanged(iEmitterIndex);
}

ParticleRenderer::ParticleRenderer(Renderer* pRenderer) {
    auto& data = mtxRenderData.second;

    data.pShaderProgram = pRenderer->getShaderManager().getShaderProgram(
        "engine/shaders/node/ParticleEmitterNode.vert.glsl",
        "engine/shaders/node/ParticleEmitterNode.frag.glsl");

    data.iViewMatrixUniform = data.pShaderProgram->getShaderUniformLocation("viewMatrix");
    data.iProjectionMatrixUniform = data.pShaderProgram->getShaderUniformLocation("projectionMatrix");
    data.iIsUsingTextureUniform = data.pShaderProgram->getShaderUniformLocation("bIsUsingTexture");

    data.iInstancedDataUniformBlockBindingIndex =
        data.pShaderProgram->getShaderUniformBlockBindingIndex("ParticleInstanceData");
}

ParticleRenderer::~ParticleRenderer() {
    std::scoped_lock guard(mtxRenderData.first);

    if (!mtxRenderData.second.vActiveEmitters.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "particle renderer is being destroyed but there are still some active emitters");
    }
}

std::unique_ptr<ParticleRenderingHandle>
ParticleRenderer::registerParticleEmitter(unsigned int iMaxParticleCount) {
    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    const auto iOldSize = data.vActiveEmitters.size();
    if (iOldSize > std::numeric_limits<unsigned short>::max()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("reached maximum particle emitter count of {}", iOldSize));
    }
    const auto iRenderDataIndex = static_cast<unsigned short>(iOldSize);

    auto& newEmitterData = data.vActiveEmitters.emplace_back();

    // Create handle.
    auto pNewHandle =
        std::unique_ptr<ParticleRenderingHandle>(new ParticleRenderingHandle(this, iRenderDataIndex));
    newEmitterData.pHandle = pNewHandle.get();

    // Create vertex buffer.
    {
        std::scoped_lock guard(GpuResourceManager::mtx);

        // Create 4 quad vertices (that just store UVs) for particles.
        const std::array<glm::vec2, 4> vVertices = {
            glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)};
        const std::array<unsigned short, 6> vIndices = {0, 2, 1, 0, 3, 2};

        unsigned int iVao = 0;
        unsigned int iVbo = 0;
        unsigned int iEbo = 0;
        glGenVertexArrays(1, &iVao);
        glGenBuffers(1, &iVbo);
        glGenBuffers(1, &iEbo);

        glBindVertexArray(iVao);
        {
            // Allocate indices.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iEbo);
            GL_CHECK_ERROR(glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                vIndices.size() * sizeof(vIndices[0]),
                vIndices.data(),
                GL_STATIC_DRAW));

            // Allocate vertices.
            glBindBuffer(GL_ARRAY_BUFFER, iVbo);
            GL_CHECK_ERROR(glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<long long>(vVertices.size() * sizeof(vVertices[0])),
                vVertices.data(),
                GL_STATIC_DRAW));

            // Describe vertex layout.

            // Quad UV attribute.
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0,                 // attribute index (layout location)
                2,                 // number of components
                GL_FLOAT,          // type of component
                GL_FALSE,          // whether data should be normalized or not
                sizeof(glm::vec2), // stride (size in bytes between elements)
                0);                // beginning offset
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        newEmitterData.pVao = std::unique_ptr<VertexArrayObject>(new VertexArrayObject(
            iVao,
            iVbo,
            static_cast<unsigned int>(vVertices.size()),
            iEbo,
            static_cast<unsigned int>(vIndices.size())));

        // Prepare instanced array size.
        constexpr unsigned int iHardcodedParticleLimit = 512; // <- hardcoded array size from the shader
        if (iMaxParticleCount > iHardcodedParticleLimit) {
            const auto iOldParticleCount = iMaxParticleCount;
            iMaxParticleCount = iHardcodedParticleLimit;
#if defined(DEBUG)
            Log::warn(std::format(
                "emitter requested a GPU buffer for {} particles but the hardcoded limit is {}, particle "
                "count will be clamped to {}",
                iOldParticleCount,
                iMaxParticleCount,
                iMaxParticleCount));
#endif
        }
        size_t iTestInstancedBufferSize = sizeof(ParticleRenderData) * static_cast<size_t>(iMaxParticleCount);
        if (iTestInstancedBufferSize > std::numeric_limits<unsigned int>::max()) {
            const auto iMaxCount = std::numeric_limits<unsigned int>::max() / sizeof(ParticleRenderData);
            iTestInstancedBufferSize = iMaxCount * sizeof(ParticleRenderData);
            Log::warn(std::format(
                "emitter requested a GPU buffer for {} particles (which is too much) so the GPU buffer "
                "will be created only for {} particles",
                iMaxParticleCount,
                iMaxCount));
        }

        // Create instanced array.
        const auto iInstancedBufferSizeInBytes = static_cast<unsigned int>(iTestInstancedBufferSize);

        newEmitterData.pInstancedArrayBuffer =
            GpuResourceManager::createUniformBuffer(iInstancedBufferSizeInBytes, true);
    }

    return pNewHandle;
}

ParticleEmitterRenderDataGuard
ParticleRenderer::getParticleEmitterRenderData(ParticleRenderingHandle& handle) {
    mtxRenderData.first.lock(); // we will unlock it when render data guard is destroyed
    auto& data = mtxRenderData.second;

    return ParticleEmitterRenderDataGuard(
        this, &data.vActiveEmitters[handle.iRenderDataIndex], handle.iRenderDataIndex);
}

void ParticleRenderer::onParticleRenderDataChanged(size_t iEmitterIndex) {
    auto& emitterData = mtxRenderData.second.vActiveEmitters[iEmitterIndex];

    const auto iMaxElementCount =
        emitterData.pInstancedArrayBuffer->getSizeInBytes() / sizeof(ParticleRenderData);

    size_t iElementCount = std::min(emitterData.vParticleData.size(), iMaxElementCount);

    emitterData.pInstancedArrayBuffer->copyDataToBuffer(
        0,
        static_cast<unsigned int>(iElementCount * sizeof(ParticleRenderData)),
        emitterData.vParticleData.data());

    mtxRenderData.first.unlock();
}

void ParticleRenderer::onBeforeHandleDestroyed(ParticleRenderingHandle* pHandle) {
    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    data.vActiveEmitters.erase(data.vActiveEmitters.begin() + pHandle->iRenderDataIndex);

    // Update handles.
    for (auto i = static_cast<size_t>(pHandle->iRenderDataIndex); i < data.vActiveEmitters.size(); i++) {
        data.vActiveEmitters[i].pHandle->iRenderDataIndex -= 1;
    }
}

void ParticleRenderer::drawParticles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    PROFILE_FUNC;

    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    glEnable(GL_BLEND);
    {
        glUseProgram(data.pShaderProgram->getShaderProgramId());

        // Set camera uniforms.
        glUniformMatrix4fv(data.iViewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(data.iProjectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        for (const auto& emitterData : data.vActiveEmitters) {
            glBindVertexArray(emitterData.pVao->getVertexArrayObjectId());

            glBindTexture(GL_TEXTURE_2D, emitterData.iTextureId);
            glUniform1i(data.iIsUsingTextureUniform, emitterData.iTextureId);

            // Bind instanced array.
            glBindBufferBase(
                GL_UNIFORM_BUFFER,
                data.iInstancedDataUniformBlockBindingIndex,
                emitterData.pInstancedArrayBuffer->getBufferId());

            glDrawElementsInstanced(
                GL_TRIANGLES,
                6, // <- 6 indices (2 triangles) of a quad
                GL_UNSIGNED_SHORT,
                nullptr,
                static_cast<GLsizei>(emitterData.vParticleData.size()));
        }
    }
    glDisable(GL_BLEND);
}
