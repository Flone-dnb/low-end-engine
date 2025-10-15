#include "game/script/ScriptManager.h"

// Custom.
#include "io/Log.h"
#include "game/script/Script.h"
#include "misc/ProjectPaths.h"
#include "math/GLMath.hpp"
#include "render/DebugDrawer.h"

// External.
#include "scriptstdstring/scriptstdstring.h"
#include "scriptbuilder/scriptbuilder.h"
#include "scriptmath/scriptmath.h"
#include "autowrapper/aswrappedcall.h"

static void messageCallback(const asSMessageInfo* msg, void* param) {
    if (msg->type == asMSGTYPE_INFORMATION) {
        Log::info(std::format("[script]: {} ({}, {}) {}", msg->section, msg->row, msg->col, msg->message));
    } else if (msg->type == asMSGTYPE_WARNING) {
        Log::warn(std::format("[script]: {} ({}, {}) {}", msg->section, msg->row, msg->col, msg->message));
    } else {
        Error::showErrorAndThrowException(
            std::format("script error: {} ({}, {}) {}", msg->section, msg->row, msg->col, msg->message));
    }
}

ScriptManager::ScriptManager() {
    // Create engine.
    pScriptEngine = asCreateScriptEngine();
    if (pScriptEngine == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("failed to create the script engine");
    }

    // Register message callback.
    pScriptEngine->SetMessageCallback(asFUNCTION(messageCallback), 0, asCALL_CDECL);

    {
        std::scoped_lock guard(mtxUnusedContexts.first);

        // Create 1 unused context.
        const auto pContext = pScriptEngine->CreateContext();
        if (pContext == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("failed to create a script context");
        }
        mtxUnusedContexts.second.push_back(pContext);
    }

    // Register addons.
    {
        pScriptEngine->SetDefaultNamespace("std");
        RegisterStdString(pScriptEngine);
        RegisterScriptMath(pScriptEngine);
        pScriptEngine->SetDefaultNamespace("");
    }

    registerLogger();
    registerGlmTypes();
    registerDebugDrawer();
}

ScriptManager::~ScriptManager() {
    {
        std::scoped_lock guard(mtxUnusedContexts.first);
        for (const auto& pContext : mtxUnusedContexts.second) {
            pContext->Release();
        }
    }

    pScriptEngine->ShutDownAndRelease();
}

std::variant<std::unique_ptr<Script>, Error>
ScriptManager::compileScript(const std::string& sRelativePathToScript, bool bForceRecompile) {
    // Construct full path.
    const auto pathToScriptFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToScript;
    if (!std::filesystem::exists(pathToScriptFile)) [[unlikely]] {
        return Error(std::format("script file does not exist (\"{}\")", sRelativePathToScript));
    }

    // Check if module already exists.
    auto pModule = pScriptEngine->GetModule(sRelativePathToScript.c_str(), asGM_ONLY_IF_EXISTS);
    if (pModule != nullptr && bForceRecompile) {
        pScriptEngine->DiscardModule(sRelativePathToScript.c_str());
        pModule = nullptr;
    }
    if (pModule == nullptr) {
        // Create a new module.

        CScriptBuilder builder;
        auto result = builder.StartNewModule(pScriptEngine, sRelativePathToScript.c_str());
        if (result < 0) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to create a new module for the script \"{}\"", sRelativePathToScript));
        }

        result = builder.AddSectionFromFile(pathToScriptFile.string().c_str());
        if (result < 0) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to load the script \"{}\"", sRelativePathToScript));
        }

        result = builder.BuildModule();
        if (result < 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to compile the script \"{}\", see log for compilation errors",
                sRelativePathToScript));
        }

        pModule = pScriptEngine->GetModule(sRelativePathToScript.c_str(), asGM_ONLY_IF_EXISTS);
        if (pModule == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("failed to prepare a module for the script \"{}\"", sRelativePathToScript));
        }
    }

    return std::unique_ptr<Script>(new Script(sRelativePathToScript, pModule, this));
}

ReservedContextGuard::~ReservedContextGuard() {
    std::scoped_lock guard(pScriptManager->mtxUnusedContexts.first);

    // Free any objects that might be held.
    pContext->Unprepare();

    pScriptManager->mtxUnusedContexts.second.push_back(pContext);
}

void ScriptManager::registerPointerType(
    const std::string& sNamespace,
    const std::string& sTypeName,
    const std::function<std::vector<ScriptMemberInfo>()>& onSetPublicMembers) {
    if (!sNamespace.empty()) {
        pScriptEngine->SetDefaultNamespace(sNamespace.c_str());
    } else {
        pScriptEngine->SetDefaultNamespace("");
    }

    // Register type.
    auto result = pScriptEngine->RegisterObjectType(sTypeName.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to register the object type \"{}\", see logs", sTypeName));
    }

    // Register members.
    const auto vMembers = onSetPublicMembers();
    for (const auto& memberInfo : vMembers) {
        if (memberInfo.iVariableOffset >= 0) {
            result = pScriptEngine->RegisterObjectProperty(
                sTypeName.c_str(), memberInfo.sDeclaration.c_str(), memberInfo.iVariableOffset);
        } else {
            result = pScriptEngine->RegisterObjectMethod(
                sTypeName.c_str(), memberInfo.sDeclaration.c_str(), memberInfo.functionPtr, asCALL_THISCALL);
        }

        if (result < 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to register the member function \"{}\" for type \"{}\", see logs",
                memberInfo.sDeclaration,
                sTypeName));
        }
    }

    if (!sNamespace.empty()) {
        pScriptEngine->SetDefaultNamespace("");
    }
}

std::unique_ptr<ReservedContextGuard> ScriptManager::reserveContextForExecution() {
    std::scoped_lock guard(mtxUnusedContexts.first);

    asIScriptContext* pContext = nullptr;

    if (!mtxUnusedContexts.second.empty()) {
        pContext = *mtxUnusedContexts.second.rbegin();
        mtxUnusedContexts.second.pop_back();
    } else {
        pContext = pScriptEngine->CreateContext();
        if (pContext == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("failed to create a script context");
        }
    }

    return std::unique_ptr<ReservedContextGuard>(new ReservedContextGuard(pContext, this));
}

void ScriptManager::registerGlobalFunction(
    const std::string& sNamespace, const std::string& sDeclaration, const asSFuncPtr& funcPointer) {
    if (!sNamespace.empty()) {
        pScriptEngine->SetDefaultNamespace(sNamespace.c_str());
    }

#if defined(WIN32)
    const auto result =
        pScriptEngine->RegisterGlobalFunction(sDeclaration.c_str(), funcPointer, asCALL_CDECL);
#else
    // Most likely running Linux or other weird platform, AngelScript most likely does not support
    // native calling convention thus we have to use the other way.
    const auto result =
        pScriptEngine->RegisterGlobalFunction(sDeclaration.c_str(), funcPointer, asCALL_GENERIC);
#endif
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to register the function \"{}\", see logs", sDeclaration));
    }

    if (!sNamespace.empty()) {
        pScriptEngine->SetDefaultNamespace("");
    }
}

static void loggerInfo(std::string sText) { Log::info("[script]: " + sText); }
static void loggerWarn(std::string sText) { Log::warn("[script]: " + sText); }
static void loggerError(std::string sText) { Log::error("[script]: " + sText); }
void ScriptManager::registerLogger() {
    registerGlobalFunction("Log", "void info(std::string)", asGLOBAL_FUNC(loggerInfo));
    registerGlobalFunction("Log", "void warn(std::string)", asGLOBAL_FUNC(loggerWarn));
    registerGlobalFunction("Log", "void error(std::string)", asGLOBAL_FUNC(loggerError));
}

static void glmVec2Constructor(float x, float y, glm::vec2* self) { new (self) glm::vec2(x, y); }
static void glmVec3Constructor(float x, float y, float z, glm::vec3* self) { new (self) glm::vec3(x, y, z); }
static void glmVec4Constructor(float x, float y, float z, float w, glm::vec4* self) {
    new (self) glm::vec4(x, y, z, w);
}
static float glmVec2Dot(glm::vec2 a, glm::vec2 b) { return glm::dot(a, b); }
static float glmVec3Dot(glm::vec3 a, glm::vec3 b) { return glm::dot(a, b); }
static glm::vec3 glmVec3Cross(glm::vec3 a, glm::vec3 b) { return glm::cross(a, b); }
static float glmDegrees(float radians) { return glm::degrees(radians); }
static float glmRadians(float degrees) { return glm::radians(degrees); }
void ScriptManager::registerGlmTypes() {
    registerValueType<glm::vec2>(
        "glm",
        "vec2",
        [&]() -> std::vector<ScriptMemberInfo> {
            return {
                ScriptMemberInfo("float x", asOFFSET(glm::vec2, x)),
                ScriptMemberInfo("float y", asOFFSET(glm::vec2, y)),
            };
        },
        ScriptTypeConstructor("void f(float, float)", asFUNCTION(glmVec2Constructor)));

    registerValueType<glm::vec3>(
        "glm",
        "vec3",
        [&]() -> std::vector<ScriptMemberInfo> {
            return {
                ScriptMemberInfo("float x", asOFFSET(glm::vec3, x)),
                ScriptMemberInfo("float y", asOFFSET(glm::vec3, y)),
                ScriptMemberInfo("float z", asOFFSET(glm::vec3, z)),
            };
        },
        ScriptTypeConstructor("void f(float, float, float)", asFUNCTION(glmVec3Constructor)));

    registerValueType<glm::vec4>(
        "glm",
        "vec4",
        [&]() -> std::vector<ScriptMemberInfo> {
            return {
                ScriptMemberInfo("float x", asOFFSET(glm::vec4, x)),
                ScriptMemberInfo("float y", asOFFSET(glm::vec4, y)),
                ScriptMemberInfo("float z", asOFFSET(glm::vec4, z)),
                ScriptMemberInfo("float w", asOFFSET(glm::vec4, w)),
            };
        },
        ScriptTypeConstructor("void f(float, float, float, float)", asFUNCTION(glmVec4Constructor)));

    registerValueType<glm::mat3>("glm", "mat3", [&]() -> std::vector<ScriptMemberInfo> { return {}; });
    registerValueType<glm::mat4>("glm", "mat4", [&]() -> std::vector<ScriptMemberInfo> { return {}; });

    registerGlobalFunction("glm", "float dot(glm::vec2, glm::vec2)", asGLOBAL_FUNC(glmVec2Dot));
    registerGlobalFunction("glm", "float dot(glm::vec3, glm::vec3)", asGLOBAL_FUNC(glmVec3Dot));

    registerGlobalFunction("glm", "vec3 cross(glm::vec3, glm::vec3)", asGLOBAL_FUNC(glmVec3Cross));

    registerGlobalFunction("glm", "float degrees(float)", asGLOBAL_FUNC(glmDegrees));
    registerGlobalFunction("glm", "float radians(float)", asGLOBAL_FUNC(glmRadians));
}

#if defined(DEBUG)
static void scriptDrawText(std::string sText, float timeInSec) { DebugDrawer::drawText(sText, timeInSec); }
static void scriptDrawSphere(float radius, glm::vec3 worldPosition, float timeInSec) {
    DebugDrawer::drawSphere(radius, worldPosition, timeInSec);
}
#endif
void ScriptManager::registerDebugDrawer() {
#if defined(DEBUG)
    registerGlobalFunction(
        "DebugDrawer", "void drawText(std::string sText, float)", asGLOBAL_FUNC(scriptDrawText));
    registerGlobalFunction(
        "DebugDrawer", "void drawSphere(float, glm::vec3, float)", asGLOBAL_FUNC(scriptDrawSphere));
#endif
}
