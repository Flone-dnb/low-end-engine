#pragma once

// Standard.
#include <variant>
#include <memory>
#include <string>
#include <mutex>
#include <vector>

// Custom.
#include "misc/Error.h"

// External.
#include "angelscript.h"

class Script;
class ScriptManager;

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
     *
     * @return Error if compilation failed.
     */
    std::variant<std::unique_ptr<Script>, Error> compileScript(const std::string& sRelativePathToScript);

    /**
     * Registers a new global function to be accessed in the scripts.
     *
     * Example:
     * @code
     * static void myfunc(const std::string& sText) {
     *     Logger::get().info(sText);
     * }
     * registerGlobalFunction("void myfunc(string)", asFUNCTION(myfunc));
     * @endcode
     *
     * @param sDeclaration Function declaration.
     * @param funcPointer  Function pointer.
     */
    void registerGlobalFunction(const std::string& sDeclaration, const asSFuncPtr& funcPointer);

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

    /** Angel script engine. */
    asIScriptEngine* pScriptEngine = nullptr;

    /**
     * Used to avoid allocating the context objects all the time. The context objects are quite heavy weight
     * and should be shared between function calls.
     */
    std::pair<std::mutex, std::vector<asIScriptContext*>> mtxUnusedContexts;
};
