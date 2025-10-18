#pragma once

// Standard.
#include <string>

// External.
#include "angelscript.h"

/** Groups info about custom type's constructor. */
class ScriptTypeConstructor {
public:
    ScriptTypeConstructor() = delete;

    /**
     * Creates destructor info.
     *
     * Example:
     * @code
     * static void glmVec2Constructor(float x, float y, glm::vec2* self) { new (self) glm::vec2(x, y); }
     * ScriptTypeConstructor("void f(float, float)", SCRIPT_CONSTRUCTOR(glmVec2Constructor));
     * @endcode
     *
     * @param sDeclaration Function declaration.
     * @param funcPtr  Pointer to the constructor function.
     */
    ScriptTypeConstructor(const std::string& sDeclaration, asSFuncPtr funcPtr)
        : sDeclaration(sDeclaration), functionPtr(funcPtr) {}

    /** Member declaration. */
    const std::string sDeclaration;

    /** Function pointer. */
    asSFuncPtr functionPtr{};
};

/** Groups info about custom type's member variable/function. */
class ScriptMemberInfo {
public:
    ScriptMemberInfo() = delete;

    /**
     * Register a member variable that can be accessed directly (without a getter function).
     *
     * Example:
     * @code
     * struct MyType {
     *     float x;
     * };
     * ScriptMemberInfo("float x", SCRIPT_MEMBER_VARIABLE(MyType, x));
     * @endcode
     *
     * @param sDeclaration Member declaration.
     * @param iOffset      Offset of the variable.
     */
    ScriptMemberInfo(const std::string& sDeclaration, int iOffset)
        : sDeclaration(sDeclaration), iVariableOffset(iOffset) {}

    /**
     * Register a member variable that can be accessed directly (without a getter function).
     *
     * Example:
     * @code
     * struct MyType {
     *     void func();
     * };
     * ScriptMemberInfo("void func()", SCRIPT_MEMBER_FUNC(MyType, func));
     * @endcode
     *
     * @param sDeclaration Member declaration.
     * @param funcPtr      Function pointer.
     */
    ScriptMemberInfo(const std::string& sDeclaration, asSFuncPtr funcPtr)
        : sDeclaration(sDeclaration), functionPtr(funcPtr) {}

    /** Member declaration. */
    const std::string sDeclaration;

    /** Non-negative if variable. */
    const int iVariableOffset = -1;

    /** Non zero if function. */
    asSFuncPtr functionPtr{};
};
