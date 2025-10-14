#pragma once

// Standard.
#include <variant>
#include <memory>
#include <string>
#include <mutex>
#include <vector>
#include <functional>
#include <optional>
#include <format>

// Custom.
#include "misc/Error.h"
#include "game/script/ScriptTypeRegistration.hpp"

// External.
#include "angelscript.h"

class Script;
class ScriptManager;
class ScriptTypeInfo;

/** Small RAII-style type that reserves a context and returns it in the destructor. */
class ReservedContextGuard {
    // Only script manager is allowed to create objects of this class.
    friend class ScriptManager;

public:
    ~ReservedContextGuard();

    ReservedContextGuard() = delete;
    ReservedContextGuard(const ReservedContextGuard&) = delete;
    ReservedContextGuard& operator=(const ReservedContextGuard&) = delete;

    /**
     * Returns reserved context.
     *
     * @return Context.
     */
    asIScriptContext* getContext() const { return pContext; }

private:
    /**
     * Reserves a new context.
     *
     * @param pContext Reserved context.
     * @param pScriptManager Script manager.
     */
    ReservedContextGuard(asIScriptContext* pContext, ScriptManager* pScriptManager)
        : pContext(pContext), pScriptManager(pScriptManager) {}

    /** Reserved context. */
    asIScriptContext* const pContext = nullptr;

    /** Script manager. */
    ScriptManager* const pScriptManager = nullptr;
};

//
// ------------------------------------------------------------------------------------------------
//

/** Manages all AngelScript scripts and handles their compilation. */
class ScriptManager {
    // Only game manager is expected to create objects of this type.
    friend class GameManager;

    // Returns reserved context.
    friend class ReservedContextGuard;

public:
    ~ScriptManager();

    /**
     * Compiles the specified script (or retrieves previously compiled version from cache).
     *
     * @param sRelativePathToScript Path relative to the `res` directory to the script file.
     * @param bForceRecompile       `true` to still recompile even if previously compiled version exist.
     *
     * @return Error if compilation failed.
     */
    std::variant<std::unique_ptr<Script>, Error>
    compileScript(const std::string& sRelativePathToScript, bool bForceRecompile = false);

    /**
     * Registers a new global function to be accessed in the scripts.
     *
     * Example:
     * @code
     * static void loggerInfo(const std::string& sText) { Log::info("[script]: " + sText); }
     * registerGlobalFunction(
     *     "Log",
     *     "void info(std::string)",
     *     asFUNCTION(loggerInfo)
     * );
     * @endcode
     *
     * @param sNamespace Optional namespace of the type, specify "" to don't use a namespace.
     * @param sDeclaration Function declaration.
     * @param funcPointer  Function pointer.
     */
    void registerGlobalFunction(
        const std::string& sNamespace, const std::string& sDeclaration, const asSFuncPtr& funcPointer);

    /**
     * Registers a new value type to be accessed in the scripts.
     *
     * Example:
     * @code
     * static void glmVec2Constructor(float x, float y, glm::vec2* self) { new (self) glm::vec2(x, y); }
     * registerValueType<glm::vec2>(
     *     "glm",
     *     "vec2",
     *     [&]() -> std::vector<ScriptMemberInfo> {
     *         return {
     *             ScriptMemberInfo("float x", asOFFSET(glm::vec2, x)),
     *             ScriptMemberInfo("float y", asOFFSET(glm::vec2, y)),
     *         };
     *     },
     *     // optional constructor:
     *     ScriptTypeConstructor("void f(float, float)", asFUNCTION(glmVec2Constructor))
     * );
     * @endcode
     *
     * @param sNamespace Optional namespace of the type, specify "" to don't use a namespace.
     * @param sTypeName Type name.
     * @param iTypeSizeInBytes Size of the type in bytes.
     * @param onSetMembers Function to set public member variables/functions of the type.
     * @param optCustomConstructor Optionally you can specify a custom constructor (note that default
     * constructor will still exist).
     */
    template <typename T>
    void registerValueType(
        const std::string& sNamespace,
        const std::string& sTypeName,
        const std::function<std::vector<ScriptMemberInfo>()>& onSetPublicMembers,
        const std::optional<ScriptTypeConstructor>& optCustomConstructor = {});

    /**
     * Returns RAII-style object that returns reserved context back to the manager.
     *
     * @return Context.
     */
    std::unique_ptr<ReservedContextGuard> reserveContextForExecution();

private:
    ScriptManager();

    /** Register the Logger class to be available in the scripts. */
    void registerLogger();

    /** Registers some GLM types such as glm::vec. */
    void registerGlmTypes();

    /** Angel script engine. */
    asIScriptEngine* pScriptEngine = nullptr;

    /**
     * Used to avoid allocating the context objects all the time. The context objects are quite heavy weight
     * and should be shared between function calls.
     */
    std::pair<std::mutex, std::vector<asIScriptContext*>> mtxUnusedContexts;
};

template <typename T>
inline void ScriptManager::registerValueType(
    const std::string& sNamespace,
    const std::string& sTypeName,
    const std::function<std::vector<ScriptMemberInfo>()>& onSetPublicMembers,
    const std::optional<ScriptTypeConstructor>& optCustomConstructor) {
    if (!sNamespace.empty()) {
        pScriptEngine->SetDefaultNamespace(sNamespace.c_str());
    }

    // Register type.
    auto result = pScriptEngine->RegisterObjectType(
        sTypeName.c_str(), sizeof(T), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<T>());
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to register the object \"{}\", see logs", sTypeName));
    }

    if (optCustomConstructor.has_value()) {
        // Register constructors.
        result = pScriptEngine->RegisterObjectBehaviour(
            sTypeName.c_str(),
            asBEHAVE_CONSTRUCT,
            optCustomConstructor->sDeclaration.c_str(),
            optCustomConstructor->functionPtr,
            asCALL_CDECL_OBJLAST);
        if (result < 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to register constructor \"{}\" for type \"{}\", see logs",
                optCustomConstructor->sDeclaration,
                sTypeName));
        }
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
