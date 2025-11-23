#include "render/ShaderManager.h"

// Standard.
#include <format>
#include <array>

// Custom.
#include "misc/Error.h"
#include "render/shader/Shader.h"
#include "misc/ProjectPaths.h"
#include "render/wrapper/ShaderProgram.h"
#include "misc/Profiler.hpp"
#include "io/Log.h"

// External.
#include "glad/glad.h"
#include "GLSL-Include/src/glsl_include.hpp"

std::shared_ptr<Shader> ShaderManager::compileShader(const std::string& sPathToShaderRelativeRes) {
    PROFILE_FUNC

    // Construct full path.
    const auto pathToShader =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToShaderRelativeRes;

    // Make sure the specified path exists.
    if (!std::filesystem::exists(pathToShader)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("path \"{}\" does not exist", pathToShader.string()));
    }

    // Parse includes.
    auto loader = glsl_include::ShaderLoader("#include");
    auto result = loader.load_shader(pathToShader.string());
    if (std::holds_alternative<glsl_include::Error>(result)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to parse `#include`s from shader \"{}\", error: {}",
            pathToShader.string(),
            std::get<glsl_include::Error>(std::move(result)).message));
    }
    auto sSourceCode = std::get<std::string>(std::move(result));

    // Prepare GL shader type.
    int shaderType = 0;
    std::string sFilename = pathToShader.filename().string();
    if (sFilename.ends_with(".vert.glsl")) {
        shaderType = GL_VERTEX_SHADER;
    } else if (sFilename.ends_with(".frag.glsl")) {
        shaderType = GL_FRAGMENT_SHADER;
    } else if (sFilename.ends_with(".comp.glsl")) {
        shaderType = GL_COMPUTE_SHADER;
    } else [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to determine shader type (vertex, fragment, etc) from shader file name, shader: {}",
            sPathToShaderRelativeRes));
    }

    // Insert base code for all shaders.
    sSourceCode.insert(
        0,
        "#version 310 es\n"
        "precision highp float;\n"
        "precision highp int;\n"
        "precision highp sampler2DArrayShadow;\n");

    // Define some macros.
    std::vector<std::string_view> vDefinedMacros;
#if defined(ENGINE_EDITOR)
    vDefinedMacros.push_back("ENGINE_EDITOR");
#endif

    if (!vDefinedMacros.empty()) {
        // Insert macros into the source code.
        const auto iVersionPos = sSourceCode.find("#version");
        if (iVersionPos == std::string::npos) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "unable to find `#version` in source \"{}\" or any included files", pathToShader.string()));
        }
        const auto iVersionLineEndPos = sSourceCode.find('\n', iVersionPos);
        if (iVersionLineEndPos == std::string::npos) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "expected to have a new line after `#version` in source \"{}\"", pathToShader.string()));
        }
        std::string sMacrosString;
        for (const auto& sMacro : vDefinedMacros) {
            sMacrosString += "#define " + std::string(sMacro) + "\n";
        }
        sSourceCode.insert(iVersionLineEndPos + 1, sMacrosString);
    }

    const auto iShaderId = glCreateShader(shaderType);

    // Attach source code and compile.
    std::array<const char*, 1> vCodesToAttach = {sSourceCode.c_str()};
    glShaderSource(iShaderId, static_cast<int>(vCodesToAttach.size()), vCodesToAttach.data(), nullptr);
    glCompileShader(iShaderId);

    // See if there were any warnings/errors.
    int iSuccess = 0;
    std::array<char, 8192> infoLog = {0};
    glGetShaderiv(iShaderId, GL_COMPILE_STATUS, &iSuccess);
    if (iSuccess == 0) [[unlikely]] {
        glGetShaderInfoLog(iShaderId, static_cast<int>(infoLog.size()), nullptr, infoLog.data());
        // Log source for debugging.
        std::string sFormattedSourceCode;
        size_t iLineNumber = 2; // start with 2 to match reporter lines
        sFormattedSourceCode = "2. ";
        for (const auto& ch : sSourceCode) {
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                sFormattedSourceCode += std::format("\n{}. ", iLineNumber);
                iLineNumber += 1;
                continue;
            }

            sFormattedSourceCode += ch;
        }
        Log::info(std::format("full source code of the shader:\n{}\n", sFormattedSourceCode));
        Error::showErrorAndThrowException(std::format(
            "failed to compile shader from \"{}\" (see log to view the full source code), error: {}",
            sPathToShaderRelativeRes,
            infoLog.data()));
    }

    return std::shared_ptr<Shader>(new Shader(this, sPathToShaderRelativeRes, iShaderId));
}

std::shared_ptr<ShaderProgram> ShaderManager::compileShaderProgram(
    const std::string& sProgramName,
    const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
    const std::shared_ptr<Shader>& pVertexShader) {
    const auto showCompilationError = [](unsigned int iShaderProgramId,
                                         const std::vector<std::shared_ptr<Shader>>& vShaders) {
        std::string sShaderNames;
        for (const auto& pShader : vShaders) {
            sShaderNames += pShader->getPathToShaderRelativeRes() + ", ";
        }
        sShaderNames.pop_back();
        sShaderNames.pop_back();

        GLint iLogLength = 0;
        glGetProgramiv(iShaderProgramId, GL_INFO_LOG_LENGTH, &iLogLength);

        std::vector<char> vInfoLog;
        vInfoLog.resize(static_cast<size_t>(iLogLength));
        glGetProgramInfoLog(iShaderProgramId, static_cast<int>(vInfoLog.size()), nullptr, vInfoLog.data());

        Error::showErrorAndThrowException(
            std::format("failed to link shader(s) {} together, error: {}", sShaderNames, vInfoLog.data()));
    };

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
        showCompilationError(iShaderProgramId, vLinkedShaders);
    }

    unsigned int iVertexOnlyShaderProgramId = 0;
    if (pVertexShader != nullptr) {
        iVertexOnlyShaderProgramId = GL_CHECK_ERROR(glCreateProgram());
        GL_CHECK_ERROR(glAttachShader(iVertexOnlyShaderProgramId, pVertexShader->getShaderId()));
        GL_CHECK_ERROR(glAttachShader(iVertexOnlyShaderProgramId, pEmptyFragmentShader->getShaderId()));
        GL_CHECK_ERROR(glLinkProgram(iVertexOnlyShaderProgramId));
        iSuccess = 0;
        glGetProgramiv(iVertexOnlyShaderProgramId, GL_LINK_STATUS, &iSuccess);
        if (iSuccess == 0) {
            showCompilationError(iVertexOnlyShaderProgramId, {pVertexShader});
        }
    }

    return std::shared_ptr<ShaderProgram>(
        new ShaderProgram(this, vLinkedShaders, iShaderProgramId, sProgramName, iVertexOnlyShaderProgramId));
}

ShaderManager::~ShaderManager() {
    pEmptyFragmentShader = nullptr;

    std::scoped_lock guard(mtxPathsToShaders.first);

    const auto iShaderCount = mtxPathsToShaders.second.size();
    if (iShaderCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "shader manager is being destroyed but there are still {} shader(s) not deleted", iShaderCount));
    }
}

std::shared_ptr<ShaderProgram> ShaderManager::getShaderProgram(
    const std::string& sPathToVertexShaderRelativeRes, const std::string& sPathToFragmentShaderRelativeRes) {
    PROFILE_FUNC

    // Get shaders.
    auto pVertexShader = getShader(sPathToVertexShaderRelativeRes);
    auto pFragmentShader = getShader(sPathToFragmentShaderRelativeRes);

    std::scoped_lock guard(mtxDatabase.first);

    // Find program.
    const auto sCombinedName =
        pVertexShader->getPathToShaderRelativeRes() + pFragmentShader->getPathToShaderRelativeRes();

    const auto it = mtxDatabase.second.find(sCombinedName);
    if (it == mtxDatabase.second.end()) {
        // Load and compile.
        const auto pShaderProgram =
            compileShaderProgram(sCombinedName, {pVertexShader, pFragmentShader}, pVertexShader);
        auto& [pWeak, pRaw] = mtxDatabase.second[sCombinedName];
        pWeak = pShaderProgram;
        pRaw = pShaderProgram.get();

        return pShaderProgram;
    }

    return it->second.first.lock();
}

std::shared_ptr<ShaderProgram>
ShaderManager::getShaderProgram(const std::string& sPathToComputeShaderRelativeRes) {
    PROFILE_FUNC

    auto pComputeShader = getShader(sPathToComputeShaderRelativeRes);

    std::scoped_lock guard(mtxDatabase.first);

    // Find program.
    const auto sName = pComputeShader->getPathToShaderRelativeRes();

    const auto it = mtxDatabase.second.find(sName);
    if (it == mtxDatabase.second.end()) {
        // Load and compile.
        const auto pShaderProgram = compileShaderProgram(sName, {pComputeShader}, nullptr);
        auto& [pWeak, pRaw] = mtxDatabase.second[sName];
        pWeak = pShaderProgram;
        pRaw = pShaderProgram.get();

        return pShaderProgram;
    }

    return it->second.first.lock();
}

ShaderManager::ShaderManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    pEmptyFragmentShader = getShader("engine/shaders/Empty.frag.glsl");
}

std::shared_ptr<Shader> ShaderManager::getShader(const std::string& sPathToShaderRelativeRes) {
    std::scoped_lock guard(mtxPathsToShaders.first);

    const auto it = mtxPathsToShaders.second.find(sPathToShaderRelativeRes);
    if (it == mtxPathsToShaders.second.end()) {
        // Load and compile.
        const auto pShader = compileShader(sPathToShaderRelativeRes);
        mtxPathsToShaders.second[sPathToShaderRelativeRes] = pShader;

        return pShader;
    }

    return it->second.lock();
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
    std::scoped_lock guard(mtxDatabase.first);

    // Erase.
    const auto it = mtxDatabase.second.find(sShaderProgramId);
    if (it == mtxDatabase.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find shader program \"{}\" previously loaded", sShaderProgramId));
    }
    mtxDatabase.second.erase(it);
}
