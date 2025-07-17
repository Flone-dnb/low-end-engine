#pragma once

// Standard.
#include <string>
#include <string_view>

// Custom.
#include "game/node/ui/LayoutUiNode.h"

class Serializable;

/** Allows viewing and modifing an integer variable. */
class UnsignedLongLongInspector : public LayoutUiNode {
public:
    UnsignedLongLongInspector() = delete;

    /**
     * Creates a new node.
     *
     * @param sNodeName      Name of this node.
     * @param pObject        Object that owns the property.
     * @param sVariableName  Name of the variable to inspect.
     */
    UnsignedLongLongInspector(
        const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName);

    virtual ~UnsignedLongLongInspector() override = default;

private:
    /** Object that owns the variable. */
    Serializable* const pObject = nullptr;

    /** Name of the variable to inspect. */
    const std::string sVariableName;
};
