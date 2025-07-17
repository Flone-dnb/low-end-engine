#pragma once

// Standard.
#include <string>
#include <string_view>

// Custom.
#include "game/node/ui/LayoutUiNode.h"

class Serializable;
class TextEditUiNode;

/** Allows viewing and modifing a string variable. */
class StringInspector : public LayoutUiNode {
public:
    StringInspector() = delete;

    /**
     * Creates a new node.
     *
     * @param sNodeName      Name of this node.
     * @param pObject        Object that owns the property.
     * @param sVariableName  Name of the variable to inspect.
     */
    StringInspector(const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName);

    virtual ~StringInspector() override = default;

private:
    /** Object that owns the variable. */
    Serializable* const pObject = nullptr;

    /** Name of the variable to inspect. */
    const std::string sVariableName;
};
