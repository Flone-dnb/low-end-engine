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
     * Sets an argument to pass to the script function.
     *
     * Used for custom registered value types.
     *
     * Example:
     * @code
     * glm::vec2 vec(2.0F, 3.5F);
     * setArgValueType(0, &vec);
     * @endcode
     *
     * @param iArgIndex     Index of the argument, starts with 0.
     * @param pObjectToCopy Object to copy.
     */
    void setArgValueType(unsigned int iArgIndex, void* pObjectToCopy) const;

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

    /**
     * Gets a return value as a pointer to the custom value type.
     *
     * Used for custom registered value types.
     *
     * Example:
     * @code
     * glm::vec2 vec = *reinterpret_cast<glm::vec2*>(getReturnValueType());
     * @endcode
     *
     * @return Value.
     */
    void* getReturnValueType() const;

private:
    /** Prepared script context. */
    asIScriptContext* const pContext = nullptr;
};
