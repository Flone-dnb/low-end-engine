#include "material/TextureHandle.h"

// Custom.
#include "material/TextureManager.h"

TextureHandle::TextureHandle(
    TextureManager* pTextureManager, unsigned int iTextureId, const std::string& sPathToTextureRelativeRes)
    : iTextureId(iTextureId), sPathToTextureRelativeRes(sPathToTextureRelativeRes),
      pTextureManager(pTextureManager) {}

TextureHandle::~TextureHandle() { pTextureManager->releaseTextureIfNotUsed(sPathToTextureRelativeRes); }
