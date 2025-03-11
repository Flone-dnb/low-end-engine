#pragma once

// Standard.
#include <vector>
#include <functional>

class ShaderProgram;

/** Used to group functions that set values to shader `uniform` variables. */
class ShaderConstantManager {
public:
    /**
     * Adds a function that will set shader constants once called.
     *
     * @param setter Function.
     */
    void addSetterFunction(const std::function<void(ShaderProgram*)>& setter) {
        vSetterFunctions.push_back(setter);
    }

    /**
     * Calls all setter functions for the specified shader program.
     *
     * @param pShaderProgram Program to set `uniforms` to.
     */
    void setConstantsToShader(ShaderProgram* pShaderProgram) const {
        for (const auto& setter : vSetterFunctions) {
            setter(pShaderProgram);
        }
    }

private:
    /** Functions that will set constants. */
    std::vector<std::function<void(ShaderProgram*)>> vSetterFunctions;
};
