#pragma once

class asIScriptContext;

/** Helper class to pass arguments to a script function and get return value from a script function. */
class ScriptFuncInterface {
public:
    ScriptFuncInterface() = delete;
    ScriptFuncInterface(asIScriptContext* pContext) : pContext(pContext) {}

    /**
     * Sets an argument to pass to the script function.
     *
     * @param iArgIndex Index of the argument, starts with 0.
     * @param value     Value to set.
     */
    void setArgUInt(unsigned int iArgIndex, unsigned int value) const;

    /**
     * Sets an argument to pass to the script function.
     *
     * @param iArgIndex Index of the argument, starts with 0.
     * @param value     Value to set.
     */
    void setArgBool(unsigned int iArgIndex, bool value) const;

    /**
     * Sets an argument to pass to the script function.
     *
     * @param iArgIndex Index of the argument, starts with 0.
     * @param value     Value to set.
     */
    void setArgFloat(unsigned int iArgIndex, float value) const;

    /**
     * Gets a return value from the script function.
     *
     * @return Value.
     */
    unsigned int getReturnUInt() const;

    /**
     * Gets a return value from the script function.
     *
     * @return Value.
     */
    bool getReturnBool() const;

    /**
     * Gets a return value from the script function.
     *
     * @return Value.
     */
    float getReturnFloat() const;

private:
    /** Prepared script context. */
    asIScriptContext* const pContext = nullptr;
};
