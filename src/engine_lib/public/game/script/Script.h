#pragma once

// Standard.
#include <string>
#include <functional>
#include <optional>

// Custom.
#include "game/script/ScriptFuncInterface.h"
#include "misc/Error.h"

class ScriptManager;

class asIScriptModule;

/** A single instance of an AngelScript script (file). */
class Script {
    // Only script manager is allowed to create objects of this type.
    friend class ScriptManager;

public:
    ~Script() = default;

    /**
     * Executes a function with the specified name from the script.
     *
     * Example:
     * @code
     * // Angel script:
     * uint myfunc(uint input) {
     *     return input * 2;
     * }
     *
     * // C++:
     * unsigned int iReturnValue = 0;
     * executeFunction(
     *     "myfunc",
     *     [](ScriptFuncInterface& func) {
     *         func.setArgUInt(2);
     *     },
     *     [&](ScriptFuncInterface& func) {
     *         iReturnValue = func.getReturnUInt();
     *     });
     * @endcode
     *
     * @param sFunctionName    Name of the function to execute.
     * @param onSetArgs        Optional callback to pass arguments to the script function.
     * @param onGetReturnValue Optional callback to get return value from the script function.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> executeFunction(
        const std::string& sFunctionName,
        const std::function<void(const ScriptFuncInterface&)>& onSetArgs = {},
        const std::function<void(const ScriptFuncInterface&)>& onGetReturnValue = {});

private:
    Script() = delete;

    /**
     * Constructor.
     *
     * @param sRelativePathToScript Path to the script file relative to the `res` file.
     * @param pScriptModule         Script module.
     * @param pScriptManager        Script manager.
     */
    Script(
        const std::string& sRelativePathToScript,
        asIScriptModule* pScriptModule,
        ScriptManager* pScriptManager);

    /**
     * Path to the script file relative to the `res` file.
     * This string also serves as the "module name" in the AngelScript engine.
     */
    const std::string sRelativePathToScript;

    /** Module that this script uses. */
    asIScriptModule* const pScriptModule = nullptr;

    /** Manager that created this script. */
    ScriptManager* const pScriptManager = nullptr;
};
