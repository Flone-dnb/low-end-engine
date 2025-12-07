#include "render/RenderingHandle.h"

// Custom.
#include "render/MeshRenderer.h"
#include "render/ParticleRenderer.h"

MeshRenderingHandle::~MeshRenderingHandle() { pMeshRenderer->onBeforeHandleDestroyed(this); }

ParticleRenderingHandle::~ParticleRenderingHandle() { pRenderer->onBeforeHandleDestroyed(this); }
