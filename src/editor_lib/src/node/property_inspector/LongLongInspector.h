#pragma once

// Standard.
#include <string>
#include <string_view>

// Custom.
#include "game/node/ui/LayoutUiNode.h"

class Serializable;

/** Allows viewing and modifing an integer variable. */
class LongLongInspector : public LayoutUiNode {
public:
    LongLongInspector() = delete;

    /**
     * Creates a new node.
     *
     * @param sNodeName      Name of this node.
     * @param pObject        Object that owns the property.
     * @param sVariableName  Name of the variable to inspect.
     */
    LongLongInspector(const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName);

    virtual ~LongLongInspector() override = default;

private:
    /** Object that owns the variable. */
    Serializable* const pObject = nullptr;

    /** Name of the variable to inspect. */
    const std::string sVariableName;
};
