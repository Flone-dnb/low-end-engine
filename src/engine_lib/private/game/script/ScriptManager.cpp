#include "game/script/ScriptManager.h"

// Custom.
#include "io/Logger.h"
#include "game/script/Script.h"
#include "misc/ProjectPaths.h"

// External.
#include "scriptstdstring/scriptstdstring.h"
#include "scriptbuilder/scriptbuilder.h"
#include "scriptmath/scriptmath.h"

static void messageCallback(const asSMessageInfo* msg, void* param) {
    if (msg->type == asMSGTYPE_INFORMATION) {
        Logger::get().info(
            std::format("[script]: {} ({}, {}) {}", msg->section, msg->row, msg->col, msg->message));
    } else if (msg->type == asMSGTYPE_WARNING) {
        Logger::get().warn(
            std::format("[script]: {} ({}, {}) {}", msg->section, msg->row, msg->col, msg->message));
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
    RegisterStdString(pScriptEngine);
    RegisterScriptMath(pScriptEngine);

    registerLogger();
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
ScriptManager::compileScript(const std::string& sRelativePathToScript) {
    // Construct full path.
    const auto pathToScriptFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToScript;
    if (!std::filesystem::exists(pathToScriptFile)) [[unlikely]] {
        return Error(std::format("script file does not exist (\"{}\")", sRelativePathToScript));
    }

    // Check if module already exists.
    auto pModule = pScriptEngine->GetModule(sRelativePathToScript.c_str(), asGM_ONLY_IF_EXISTS);
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

void ScriptManager::registerGlobalFunction(const std::string& sDeclaration, const asSFuncPtr& funcPointer) {
    const auto result =
        pScriptEngine->RegisterGlobalFunction(sDeclaration.c_str(), funcPointer, asCALL_CDECL);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to register the function \"{}\", see logs", sDeclaration));
    }
}

static void loggerInfo(const std::string& sText) { Logger::get().info("[script]: " + sText); }
static void loggerWarn(const std::string& sText) { Logger::get().warn("[script]: " + sText); }
static void loggerError(const std::string& sText) { Logger::get().error("[script]: " + sText); }
void ScriptManager::registerLogger() {
    // Static functions must be registered as global functions so we can't just register Logger::get() as
    // object and a static method, instead:

    // Create namespace.
    pScriptEngine->SetDefaultNamespace("Logger");

    // Register Logger::info().
    auto result = pScriptEngine->RegisterGlobalFunction(
        "void Logger::info(string)", asFUNCTION(loggerInfo), asCALL_CDECL);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException("failed to register logger function");
    }

    // Register Logger::warn().
    result = pScriptEngine->RegisterGlobalFunction(
        "void Logger::warn(string)", asFUNCTION(loggerWarn), asCALL_CDECL);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException("failed to register logger function");
    }

    // Register Logger::error().
    result = pScriptEngine->RegisterGlobalFunction(
        "void Logger::error(string)", asFUNCTION(loggerError), asCALL_CDECL);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException("failed to register logger function");
    }

    pScriptEngine->SetDefaultNamespace("");
}
