#pragma once

// Standard.
#include <mutex>
#include <vector>
#include <memory>

// Custom.
#include "math/GLMath.hpp"
#include "render/wrapper/VertexArrayObject.h"
#include "render/wrapper/Buffer.h"

class ParticleRenderer;
class ParticleRenderingHandle;
class ShaderProgram;
class Renderer;

/** Data needed to render a single particle. */
struct ParticleRenderData {
    /** RGBA color. */
    glm::vec4 color;

    /** Position in world space, size in W. */
    glm::vec4 positionAndSize;
};
static_assert(sizeof(ParticleRenderData) == 32, "must be same as in shaders");

/** Data needed to render multiple particles (a particle emitter). */
struct EmitterRenderData {
    /** Particles data (used to update particles). */
    std::vector<ParticleRenderData> vParticleData;

    /** OpenGL texture ID or 0 if not used. */
    unsigned int iTextureId = 0;

    /** Do not modify, renderer updates this automatically. VAO used for drawing particles. */
    std::unique_ptr<VertexArrayObject> pVao;

    /** Do not modify, renderer updates this automatically. Stores per-particle (instanced) data. */
    std::unique_ptr<Buffer> pInstancedArrayBuffer;

    /** Do not modify, renderer uses this pointer to update handle's index. */
    ParticleRenderingHandle* pHandle = nullptr;
};

/** RAII-style type that keeps particle renderer data locked while exists. */
class ParticleEmitterRenderDataGuard {
    // Only particle renderer is allowed create objects of this type.
    friend class ParticleRenderer;

public:
    ParticleEmitterRenderDataGuard() = delete;
    ~ParticleEmitterRenderDataGuard();

    ParticleEmitterRenderDataGuard(const ParticleEmitterRenderDataGuard&) = delete;
    ParticleEmitterRenderDataGuard& operator=(const ParticleEmitterRenderDataGuard&) = delete;

    ParticleEmitterRenderDataGuard(ParticleEmitterRenderDataGuard&) noexcept = delete;
    ParticleEmitterRenderDataGuard& operator=(ParticleEmitterRenderDataGuard&) noexcept = delete;

    /**
     * Returns mesh data to modify.
     *
     * @return Data.
     */
    EmitterRenderData& getData() { return *pData; }

private:
    /**
     * Creates a new object.
     *
     * @remark Expects that the render data mutex (in the particle renderer) is already locked.
     *
     * @param pParticleRenderer Particle renderer.
     * @param pData Data to modify.
     * @param iEmitterIndex Index of the data in the array of render data for all particles.
     */
    ParticleEmitterRenderDataGuard(
        ParticleRenderer* pParticleRenderer, EmitterRenderData* pData, size_t iEmitterIndex)
        : pData(pData), pParticleRenderer(pParticleRenderer) {};

    /** Data to modify. */
    EmitterRenderData* const pData = nullptr;

    /** Mesh renderer. */
    ParticleRenderer* const pParticleRenderer = nullptr;

    /** Index of @ref pData in the array of render data for all particles. */
    const size_t iEmitterIndex = 0;
};

/** Handles particle rendering. */
class ParticleRenderer {
    // Notifies the renderer in its destructor.
    friend class ParticleEmitterRenderDataGuard;
    friend class ParticleRenderingHandle;

    // Creates this renderer to render world's particles.
    friend class World;

public:
    ParticleRenderer() = delete;
    ~ParticleRenderer();

    /**
     * Draws particles on the currently active framebuffer.
     *
     * @param viewMatrix       Camera's view matrix.
     * @param projectionMatrix Camera's projection matrix.
     */
    void drawParticles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * Registers new particles to be rendered.
     * Set particle parameters using the returned handle.
     *
     * @param iMaxParticleCount The maximum number of particles, used to preallocate a GPU buffer.
     *
     * @return RAII-style handle for rendering.
     */
    std::unique_ptr<ParticleRenderingHandle> registerParticleEmitter(unsigned int iMaxParticleCount);

    /**
     * Returns render data of particles to initialize/modify.
     *
     * @param handle Handle of the particles from @ref addParticlesForRendering.
     *
     * @return Data guard used to modify the data.
     */
    ParticleEmitterRenderDataGuard getParticleEmitterRenderData(ParticleRenderingHandle& handle);

private:
    /** Groups data used for rendering. */
    struct RenderData {
        RenderData() = default;

        /** Registered emitters. */
        std::vector<EmitterRenderData> vActiveEmitters;

        /** Program for rendering particles. */
        std::shared_ptr<ShaderProgram> pShaderProgram;

        /** Location of the shader uniform for view matrix. */
        int iViewMatrixUniform = 0;

        /** Location of the shader uniform for projection matrix. */
        int iProjectionMatrixUniform = 0;

        /** Location of the shader uniform for a boolean that tells if the texture is used or not. */
        int iIsUsingTextureUniform = 0;

        /** Binding index of the uniform buffer. */
        unsigned int iInstancedDataUniformBlockBindingIndex = 0;
    };

    /**
     * Creates a new particle renderer.
     *
     * @param pRenderer Renderer.
     */
    ParticleRenderer(Renderer* pRenderer);

    /**
     * Called from data guard destructor.
     *
     * @param iEmitterIndex Index of the particle array that changed.
     */
    void onParticleRenderDataChanged(size_t iEmitterIndex);

    /**
     * Called from handle's destructor to remove a mesh from rendering.
     *
     * @param pHandle Handle.
     */
    void onBeforeHandleDestroyed(ParticleRenderingHandle* pHandle);

    /** Data used for rendering. */
    std::pair<std::mutex, RenderData> mtxRenderData;
};
