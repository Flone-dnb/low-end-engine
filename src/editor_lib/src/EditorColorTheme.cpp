#include "EditorColorTheme.h"

// Standard.
#include <format>

std::string EditorColorTheme::floatToString(float value) {
    std::string sValue = std::format("{:.3F}", value);

    const auto dotPos = sValue.find('.');
    if (dotPos != std::string::npos) {
        const int iCharCount = static_cast<int>(sValue.size()) - static_cast<int>(dotPos + 2);
        if (iCharCount > 0) {
            // Remove trailing zeroes.
            for (int i = 0; i < iCharCount; i++) {
                if (sValue.back() == '0') {
                    sValue.pop_back();
                }
            }
        }
    }

    return sValue;
}

std::string EditorColorTheme::formatVariableName(const std::string& sName) {
    if (sName.empty()) {
        return sName;
    }

    // Skip if variable name is already pretty.
    if (sName.find(' ') != std::string::npos) {
        return sName;
    }

    std::string sOutputName;
    sOutputName.reserve(sName.size());

    sOutputName += static_cast<char>(std::toupper(static_cast<unsigned char>(sName[0])));
    for (size_t i = 1; i < sName.size(); i++) {
        if (std::isupper(sName[i])) {
            sOutputName += ' ';
            sOutputName += static_cast<char>(std::tolower(static_cast<unsigned char>(sName[i])));
            continue;
        }
        sOutputName += sName[i];
    }

    return sOutputName;
}

float EditorColorTheme::getTextHeight() { return 0.0195F; }

float EditorColorTheme::getBigTextHeight() { return getTextHeight() * 1.1F; }

float EditorColorTheme::getSmallTextHeight() { return getTextHeight() * 0.925F; }

float EditorColorTheme::getButtonSizeY() { return 0.025F; }

float EditorColorTheme::getPadding() { return 0.0275F; }

float EditorColorTheme::getSpacing() { return 0.0025F; }

float EditorColorTheme::getTypePropertyNameValueSpacing() { return getSpacing() * 4.0F; }

float EditorColorTheme::getTypePropertySpacing() { return getTypePropertyNameValueSpacing() * 4.0F; }

glm::vec4 EditorColorTheme::getEditorBackgroundColor() { return glm::vec4(glm::vec3(0.12F), 1.0F); }

glm::vec4 EditorColorTheme::getContainerBackgroundColor() { return glm::vec4(glm::vec3(0.15F), 1.0F); }

glm::vec4 EditorColorTheme::getSelectedItemColor() { return glm::vec4(0.85F, 0.35F, 0.2F, 1.0F); }

glm::vec4 EditorColorTheme::getButtonColor() { return getEditorBackgroundColor(); }

glm::vec4 EditorColorTheme::getButtonHoverColor() { return getEditorBackgroundColor() + 0.2F; }

glm::vec4 EditorColorTheme::getButtonPressedColor() { return getEditorBackgroundColor() + 0.1F; }
