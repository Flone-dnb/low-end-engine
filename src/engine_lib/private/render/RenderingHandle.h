#pragma once

class MeshRenderer;

/**
 * While you hold an object of this type the mesh will be rendered,
 * if you destroy this handle the mesh will be removed from the rendering.
 */
class MeshRenderingHandle {
    // Mesh renderer provides a handle when you register a mesh to be rendered.
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
