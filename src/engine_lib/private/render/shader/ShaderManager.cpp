#include "render/shader/ShaderManager.h"

// Standard.
#include <format>
#include <array>

// Custom.
#include "misc/Error.h"
#include "render/shader/Shader.h"
#include "misc/ProjectPaths.h"
#include "render/wrapper/ShaderProgram.h"

// External.
#include "glad/glad.h"
#include "GLSL-Include/src/glsl_include.hpp"

std::shared_ptr<Shader> ShaderManager::compileShader(const std::string& sPathToShaderRelativeRes) {
    // Construct full path.
    const auto pathToShader =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToShaderRelativeRes;

    // Make sure the specified path exists.
    if (!std::filesystem::exists(pathToShader)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("path \"{}\" does not exist", pathToShader.string()));
    }

    // Parse includes.
    auto loader = glsl_include::ShaderLoader("#include");
    const auto sSourceCode = loader.load_shader(pathToShader.string());

    // Prepare GL shader type.
    int shaderType = 0;
    if (pathToShader.string().ends_with(".vert.glsl")) {
        shaderType = GL_VERTEX_SHADER;
    } else if (pathToShader.string().ends_with(".frag.glsl")) {
        shaderType = GL_FRAGMENT_SHADER;
    } else [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to determine shader type (vertex, fragment, etc) from shader file name, shader: {}",
            sPathToShaderRelativeRes));
    }

    const auto iShaderId = glCreateShader(shaderType);

    // Attach source code and compile.
    std::array<const char*, 1> vCodesToAttach = {sSourceCode.c_str()};
    glShaderSource(iShaderId, static_cast<int>(vCodesToAttach.size()), vCodesToAttach.data(), nullptr);
    glCompileShader(iShaderId);

    // See if there were any warnings/errors.
    int iSuccess = 0;
    std::array<char, 1024> infoLog = {0}; // NOLINT
    glGetShaderiv(iShaderId, GL_COMPILE_STATUS, &iSuccess);
    if (iSuccess == 0) [[unlikely]] {
        glGetShaderInfoLog(iShaderId, static_cast<int>(infoLog.size()), nullptr, infoLog.data());
        Error error(std::format(
            "failed to compile shader from \"{}\", error: {}", sPathToShaderRelativeRes, infoLog.data()));
    }

    return std::shared_ptr<Shader>(new Shader(this, sPathToShaderRelativeRes, iShaderId));
}

std::shared_ptr<ShaderProgram> ShaderManager::compileShaderProgram(
    const std::string& sProgramName, const std::vector<std::shared_ptr<Shader>>& vLinkedShaders) {
    const auto iShaderProgramId = GL_CHECK_ERROR(glCreateProgram());

    // Link shaders.
    for (const auto& pShader : vLinkedShaders) {
        GL_CHECK_ERROR(glAttachShader(iShaderProgramId, pShader->getShaderId()));
    }
    GL_CHECK_ERROR(glLinkProgram(iShaderProgramId));

    // See if there were any linking errors.
    int iSuccess = 0;
    glGetProgramiv(iShaderProgramId, GL_LINK_STATUS, &iSuccess);
    if (iSuccess == 0) {
        std::string sShaderNames;
        for (const auto& pShader : vLinkedShaders) {
            sShaderNames += pShader->getPathToShaderRelativeRes() + ", ";
        }
        sShaderNames.pop_back();
        sShaderNames.pop_back();

        GLint iLogLength = 0;
        glGetProgramiv(iShaderProgramId, GL_INFO_LOG_LENGTH, &iLogLength);

        std::vector<char> vInfoLog;
        vInfoLog.resize(iLogLength);
        glGetProgramInfoLog(iShaderProgramId, static_cast<int>(vInfoLog.size()), nullptr, vInfoLog.data());

        Error::showErrorAndThrowException(
            std::format("failed to link shader(s) {} together, error: {}", sShaderNames, vInfoLog.data()));
    }

    return std::shared_ptr<ShaderProgram>(
        new ShaderProgram(this, vLinkedShaders, iShaderProgramId, sProgramName));
}

ShaderManager::~ShaderManager() {
    std::scoped_lock guard(mtxPathsToShaders.first);

    const auto iShaderCount = mtxPathsToShaders.second.size();
    if (iShaderCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "shader manager is being destroyed but there are still {} shader(s) not deleted", iShaderCount));
    }
}

std::shared_ptr<ShaderProgram> ShaderManager::getShaderProgram(
    const std::string& sPathToVertexShaderRelativeRes, const std::string& sPathToFragmentShaderRelativeRes) {
    // Get shaders.
    auto pVertexShader = getShader(sPathToVertexShaderRelativeRes);
    auto pFragmentShader = getShader(sPathToFragmentShaderRelativeRes);

    std::scoped_lock guard(mtxShaderPrograms.first);

    // Find program.
    const auto sCombinedName =
        pVertexShader->getPathToShaderRelativeRes() + pFragmentShader->getPathToShaderRelativeRes();
    const auto it = mtxShaderPrograms.second.find(sCombinedName);
    if (it == mtxShaderPrograms.second.end()) {
        // Load and compile.
        const auto pShaderProgram = compileShaderProgram(sCombinedName, {pVertexShader, pFragmentShader});
        auto& [pWeak, pRaw] = mtxShaderPrograms.second[sCombinedName];
        pWeak = pShaderProgram;
        pRaw = pShaderProgram.get();
        return pShaderProgram;
    } else {
        return it->second.first.lock();
    }
}

std::shared_ptr<Shader> ShaderManager::getShader(const std::string& sPathToShaderRelativeRes) {
    std::scoped_lock guard(mtxPathsToShaders.first);

    const auto it = mtxPathsToShaders.second.find(sPathToShaderRelativeRes);
    if (it == mtxPathsToShaders.second.end()) {
        // Load and compile.
        const auto pShader = compileShader(sPathToShaderRelativeRes);
        mtxPathsToShaders.second[sPathToShaderRelativeRes] = pShader;
        return pShader;
    } else {
        return it->second.lock();
    }
}

void ShaderManager::onShaderBeingDestroyed(const std::string& sPathToShaderRelativeRes) {
    std::scoped_lock guard(mtxPathsToShaders.first);

    // Erase.
    const auto it = mtxPathsToShaders.second.find(sPathToShaderRelativeRes);
    if (it == mtxPathsToShaders.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find shader \"{}\" previously loaded", sPathToShaderRelativeRes));
    }
    mtxPathsToShaders.second.erase(it);
}

void ShaderManager::onShaderProgramBeingDestroyed(const std::string& sShaderProgramId) {
    std::scoped_lock guard(mtxShaderPrograms.first);

    // Erase.
    const auto it = mtxShaderPrograms.second.find(sShaderProgramId);
    if (it == mtxShaderPrograms.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find shader program \"{}\" previously loaded", sShaderProgramId));
    }
    mtxShaderPrograms.second.erase(it);
}
