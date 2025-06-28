#pragma once

// Custom.
#include "math/GLMath.hpp"

/** Provides colors to use for UI nodes. */
class EditorColorTheme {
public:
    /**
     * Text height.
     *
     * @return Text height.
     */
    static float getTextHeight();

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
