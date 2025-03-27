#include "render/wrapper/Texture.h"

// Custom.
#include "misc/Error.h"

// External.
#include "glad/glad.h"

Texture::~Texture() { GL_CHECK_ERROR(glDeleteTextures(1, &iTextureId)); }

Texture::Texture(unsigned int iTextureId) : iTextureId(iTextureId) {}
