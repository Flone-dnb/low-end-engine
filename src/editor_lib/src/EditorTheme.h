#pragma once

// Standard.
#include <string>
#include <string_view>

// Custom.
#include "math/GLMath.hpp"

/** Determines color and design of the editor's UI. */
class EditorTheme {
public:
    /**
     * Text height for default text.
     *
     * @return Text height.
     */
    static float getTextHeight();

    /**
     * Text height for big text.
     *
     * @return Text height.
     */
    static float getBigTextHeight();

    /**
     * Text height for small text.
     *
     * @return Text height.
     */
    static float getSmallTextHeight();

    /**
     * Used to format variable names such as "materialDiffuseColor" to more readable versions such as
     * "material diffuse color".
     *
     * @param sName Name of the variable.
     *
     * @return More readable name.
     */
    static std::string formatVariableName(const std::string& sName);

    /**
     * Converts float to a fixed text format.
     *
     * @param value Value to convert.
     *
     * @return Number in text.
     */
    static std::string floatToString(float value);

    /**
     * Color of the editor's background.
     *
     * @return RGBA color.
     */
    static glm::vec4 getEditorBackgroundColor();

    /**
     * Color of container UI nodes.
     *
     * @return RGBA color.
     */
    static glm::vec4 getContainerBackgroundColor();

    /**
     * Color of an activated (selected) button and similar items.
     *
     * @return RGBA color.
     */
    static glm::vec4 getSelectedItemColor();

    /**
     * Height for buttons.
     *
     * @return Value in range [0.0; 1.0].
     */
    static float getButtonSizeY();

    /**
     * Returns padding for UI nodes.
     *
     * @return Padding.
     */
    static float getPadding();

    /**
     * Returns spacing for UI containers.
     *
     * @return Spacing.
     */
    static float getSpacing();

    /**
     * Return spacing between property's name and value.
     *
     * @return Spacing.
     */
    static float getTypePropertyNameValueSpacing();

    /**
     * Returns spacing between type's properties.
     *
     * @return Spacing.
     */
    static float getTypePropertySpacing();

    /**
     * Returns spacing between groups of properties (1 group per node type).
     *
     * @return Spacing.
     */
    static float getTypePropertyGroupSpacing();

    /**
     * Default color for buttons.
     *
     * @return RGBA color.
     */
    static glm::vec4 getButtonColor();

    /**
     * Color for buttons while hovered.
     *
     * @return RGBA color.
     */
    static glm::vec4 getButtonHoverColor();

    /**
     * Color for buttons while pressed.
     *
     * @return RGBA color.
     */
    static glm::vec4 getButtonPressedColor();
};
