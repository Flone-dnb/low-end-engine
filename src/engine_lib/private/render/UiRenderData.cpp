#include "render/UiRenderData.h"

// Custom.
#include "render/UiNodeManager.h"

TextRenderingHandle::~TextRenderingHandle() { pManager->onBeforeHandleDestroyed(this); }

TextRenderDataGuard::~TextRenderDataGuard() { pManager->mtxData.first.unlock(); }
