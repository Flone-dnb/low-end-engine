#include "game/script/Script.h"

// Standard.
#include <format>

// Custom.
#include "game/script/ScriptManager.h"
#include "misc/Error.h"

// External.
#include "angelscript.h"
#include "scripthelper/scripthelper.h"

Script::Script(
    const std::string& sRelativePathToScript, asIScriptModule* pScriptModule, ScriptManager* pScriptManager)
    : sRelativePathToScript(sRelativePathToScript), pScriptModule(pScriptModule),
      pScriptManager(pScriptManager) {}

void Script::executeFunction(
    const std::string& sFunctionName,
    const std::function<void(const ScriptFuncInterface&)>& onSetArgs,
    const std::function<void(const ScriptFuncInterface&)>& onGetReturnValue) {
    // Get function.
    const auto pFunc = pScriptModule->GetFunctionByName(sFunctionName.c_str());
    if (pFunc == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to find the function \"{}\" to execute in the script \"{}\"",
            sFunctionName,
            sRelativePathToScript));
    }

    auto pContextGuard = pScriptManager->reserveContextForExecution();

    // Prepare context.
    auto result = pContextGuard->getContext()->Prepare(pFunc);
    if (result < 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to prepare context for the function \"{}\" for the script \"{}\"",
            sFunctionName,
            sRelativePathToScript));
    }

    onSetArgs(ScriptFuncInterface(pContextGuard->getContext()));

    // Execute.
    result = pContextGuard->getContext()->Execute();
    if (result != asEXECUTION_FINISHED && result == asEXECUTION_EXCEPTION) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "execution of the function \"{}\" for the script \"{}\" failed, exception: \"{}\", in function "
            "\"{}\", on line {}, detailed info:\n{}",
            sFunctionName,
            sRelativePathToScript,
            pContextGuard->getContext()->GetExceptionString(),
            pContextGuard->getContext()->GetExceptionFunction()->GetDeclaration(),
            pContextGuard->getContext()->GetExceptionLineNumber(),
            GetExceptionInfo(pContextGuard->getContext(), true)));
        return;
    }

    onGetReturnValue(ScriptFuncInterface(pContextGuard->getContext()));
}
