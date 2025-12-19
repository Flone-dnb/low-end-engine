#include "EditorTheme.h"

// Standard.
#include <format>
#include <sstream>

std::string EditorTheme::floatToString(float value) {
    std::string sValue = std::format("{:.3F}", value);

    auto dotPos = sValue.find('.');
    if (dotPos == std::string::npos) {
        dotPos = sValue.find(',');
    }
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

float EditorTheme::stringToFloat(const std::string& sText) {
    std::istringstream stream(sText);
    float value = 0.0f;
    stream >> value;
    return value;
}

std::string EditorTheme::formatVariableName(const std::string& sName) {
    if (sName.empty()) {
        return sName;
    }

    // Skip if variable name is already pretty.
    if (sName.find(' ') != std::string::npos) {
        return sName;
    }

    std::string sOutputName;
    sOutputName.reserve(sName.size());

    size_t iStartPos = 1;

    if (sName.size() > 2 && std::islower(sName[0]) && std::isupper(sName[1])) {
        // The name probably starts with a prefix such as "sText".
        sOutputName = sName[1];
        iStartPos = 2;
    } else {
        sOutputName += static_cast<char>(std::toupper(sName[0]));
    }
    for (size_t i = iStartPos; i < sName.size(); i++) {
        if (std::isupper(sName[i])) {
            sOutputName += ' ';
            sOutputName += static_cast<char>(std::tolower(sName[i]));
            continue;
        }
        sOutputName += sName[i];
    }

    return sOutputName;
}

float EditorTheme::getTextHeight() { return 0.0195f; }

float EditorTheme::getBigTextHeight() { return getTextHeight() * 1.1f; }

float EditorTheme::getSmallTextHeight() { return getTextHeight() * 0.925f; }

float EditorTheme::getButtonSizeY() { return 0.025f; }

float EditorTheme::getPadding() { return 0.0125f; }

float EditorTheme::getSpacing() { return 0.017f; }

float EditorTheme::getTypePropertyNameValueSpacing() { return getSpacing() * 2.0f; }

float EditorTheme::getTypePropertySpacing() { return getTypePropertyNameValueSpacing() * 2.0f; }

float EditorTheme::getTypePropertyGroupSpacing() { return getSpacing() * 4.0f; }

glm::vec4 EditorTheme::getEditorBackgroundColor() { return glm::vec4(glm::vec3(0.12f), 1.0f); }

glm::vec4 EditorTheme::getContainerBackgroundColor() { return glm::vec4(glm::vec3(0.15f), 1.0f); }

glm::vec4 EditorTheme::getAccentColor() { return glm::vec4(0.85f, 0.35f, 0.2f, 1.0f); }

glm::vec4 EditorTheme::getButtonColor() { return getEditorBackgroundColor(); }

glm::vec4 EditorTheme::getButtonHoverColor() { return getEditorBackgroundColor() + 0.2f; }

glm::vec4 EditorTheme::getButtonPressedColor() { return getEditorBackgroundColor() + 0.1f; }
