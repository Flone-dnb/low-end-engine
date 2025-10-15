#include "game/script/ScriptFuncInterface.h"

// External.
#include "angelscript.h"

void ScriptFuncInterface::setArgUInt(unsigned int iArgIndex, unsigned int value) const {
    pContext->SetArgDWord(iArgIndex, value);
}

void ScriptFuncInterface::setArgBool(unsigned int iArgIndex, bool value) const {
    pContext->SetArgByte(iArgIndex, static_cast<unsigned char>(value));
}

void ScriptFuncInterface::setArgFloat(unsigned int iArgIndex, float value) const {
    pContext->SetArgFloat(iArgIndex, value);
}

void ScriptFuncInterface::setArgValueType(unsigned int iArgIndex, void* pObjectToCopy) const {
    pContext->SetArgObject(iArgIndex, pObjectToCopy);
}

void ScriptFuncInterface::setPointerValue(unsigned int iArgIndex, void* pPointerValue) const {
    pContext->SetArgObject(iArgIndex, pPointerValue);
}

unsigned int ScriptFuncInterface::getReturnUInt() const { return pContext->GetReturnDWord(); }

bool ScriptFuncInterface::getReturnBool() const { return static_cast<bool>(pContext->GetReturnByte()); }

float ScriptFuncInterface::getReturnFloat() const { return pContext->GetReturnFloat(); }

void* ScriptFuncInterface::getReturnValueType() const { return pContext->GetReturnObject(); }

void* ScriptFuncInterface::getReturnPointerType() const { return pContext->GetReturnObject(); }
