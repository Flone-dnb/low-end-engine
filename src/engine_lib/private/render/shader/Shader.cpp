#include "render/shader/Shader.h"

// Custom.
#include "render/shader/ShaderManager.h"

// External.
#include "misc/Error.h"
#include "glad/glad.h"

Shader::~Shader() {
    pShaderManager->onShaderBeingDestroyed(sPathToShaderRelativeRes);

    GL_CHECK_ERROR(glDeleteShader(iShaderId));
}

Shader::Shader(
    ShaderManager* pShaderManager, const std::string& sPathToShaderRelativeRes, unsigned int iShaderId)
    : iShaderId(iShaderId), sPathToShaderRelativeRes(sPathToShaderRelativeRes),
      pShaderManager(pShaderManager) {}
