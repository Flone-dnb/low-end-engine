#pragma once

class MeshRenderer;

/**
 * While you hold an object of this type the mesh will be rendered,
 * if you destroy this handle the mesh will be removed from the rendering.
 */
class MeshRenderingHandle {
    // Provides the handle when you register.
    friend class MeshRenderer;

public:
    MeshRenderingHandle() = delete;
    ~MeshRenderingHandle();

    MeshRenderingHandle(const MeshRenderingHandle&) = delete;
    MeshRenderingHandle& operator=(const MeshRenderingHandle&) = delete;

private:
    /**
     * Creates a new handle.
     *
     * @param pMeshRenderer Mesh renderer.
     * @param iMeshIndex    Index into the render data array.
     */
    MeshRenderingHandle(MeshRenderer* pMeshRenderer, unsigned short iMeshIndex)
        : pMeshRenderer(pMeshRenderer), iMeshRenderDataIndex(iMeshIndex) {}

    /** Object that created this handle. */
    MeshRenderer* const pMeshRenderer = nullptr;

    /**
     * Index into the render data array.
     * This index can be changed by the mesh renderer.
     */
    unsigned short iMeshRenderDataIndex = 0;
};

class ParticleRenderer;

/**
 * While you hold an object of this type the particles will be rendered,
 * if you destroy this handle the particles will be removed from the rendering.
 */
class ParticleRenderingHandle {
    // Provides the handle when you register.
    friend class ParticleRenderer;

public:
    ParticleRenderingHandle() = delete;
    ~ParticleRenderingHandle();

    ParticleRenderingHandle(const ParticleRenderingHandle&) = delete;
    ParticleRenderingHandle& operator=(const ParticleRenderingHandle&) = delete;

private:
    /**
     * Creates a new handle.
     *
     * @param pRenderer Renderer.
     * @param iRenderDataIndex Index into the render data array.
     */
    ParticleRenderingHandle(ParticleRenderer* pRenderer, unsigned short iRenderDataIndex)
        : pRenderer(pRenderer), iRenderDataIndex(iRenderDataIndex) {}

    /** Object that created this handle. */
    ParticleRenderer* const pRenderer = nullptr;

    /**
     * Index into the render data array.
     * This index can be changed by the renderer.
     */
    unsigned short iRenderDataIndex = 0;
};
