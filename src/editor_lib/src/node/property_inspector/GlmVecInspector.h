#pragma once

// Standard.
#include <string>
#include <string_view>

// Custom.
#include "game/node/ui/LayoutUiNode.h"

class Serializable;
class TextEditUiNode;

/** Determines how much components a `glm::vec` has. */
enum class GlmVecComponentCount : uint8_t { VEC2, VEC3, VEC4 };

/** Allows viewing and modifing a vector variable. */
class GlmVecInspector : public LayoutUiNode {
public:
    GlmVecInspector() = delete;

    /**
     * Creates a new node.
     *
     * @param sNodeName      Name of this node.
     * @param pObject        Object that owns the property.
     * @param sVariableName  Name of the variable to inspect.
     * @param componentCount Size of vector.
     */
    GlmVecInspector(
        const std::string& sNodeName,
        Serializable* pObject,
        const std::string& sVariableName,
        GlmVecComponentCount componentCount);

    virtual ~GlmVecInspector() override = default;

    /** Reads value from inspected property and displays the up to date value. */
    void refreshDisplayedValue();

private:
    /** Determines which component of the vector is edited. */
    enum class VectorComponent : uint8_t {
        X,
        Y,
        Z,
        W,
    };

    /**
     * Called after the value is changed.
     *
     * @param pTextEdit Text edit for vector component.
     * @param component Determines which component is edited.
     * @param sNewText  New input.
     */
    void onValueChanged(TextEditUiNode* pTextEdit, VectorComponent component, std::u16string_view sNewText);

    /**
     * Called after the value was manually inputted.
     *
     * @param value Value to set.
     */
    void setManuallyInputtedValueToObject(const glm::vec4& value);

    /** Object that owns the variable. */
    Serializable* const pObject = nullptr;

    /** UI element for a vec component. */
    TextEditUiNode* pXComponentText = nullptr;

    /** UI element for a vec component. */
    TextEditUiNode* pYComponentText = nullptr;

    /** UI element for a vec component. */
    TextEditUiNode* pZComponentText = nullptr;

    /** Name of the variable to inspect. */
    const std::string sVariableName;

    /** Size of vector. */
    const GlmVecComponentCount componentCount;
};
