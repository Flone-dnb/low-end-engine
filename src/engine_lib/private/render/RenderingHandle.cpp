#include "render/RenderingHandle.h"

// Custom.
#include "render/MeshRenderer.h"

MeshRenderingHandle::~MeshRenderingHandle() { pMeshRenderer->onBeforeHandleDestroyed(this); }
