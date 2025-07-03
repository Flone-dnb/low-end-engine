#include "EditorColorTheme.h"

float EditorColorTheme::getTextHeight() { return 0.020F; }

float EditorColorTheme::getButtonSizeY() { return 0.025F; }

float EditorColorTheme::getPadding() { return 0.025F; }

float EditorColorTheme::getSpacing() { return 0.0025F; }

glm::vec4 EditorColorTheme::getEditorBackgroundColor() { return glm::vec4(glm::vec3(0.12F), 1.0F); }

glm::vec4 EditorColorTheme::getContainerBackgroundColor() { return glm::vec4(glm::vec3(0.15F), 1.0F); }

glm::vec4 EditorColorTheme::getButtonColor() { return getEditorBackgroundColor(); }

glm::vec4 EditorColorTheme::getButtonHoverColor() { return getEditorBackgroundColor() + 0.2F; }

glm::vec4 EditorColorTheme::getButtonPressedColor() { return getEditorBackgroundColor() + 0.1F; }
