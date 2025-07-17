#include "node/property_inspector/StringInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

// External.
#include "utf/utf.hpp"

StringInspector::StringInspector(
    const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName)
    : LayoutUiNode(sNodeName), pObject(pObject), sVariableName(sVariableName) {
    // Get current value.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pObject->getTypeGuid());
    const auto variableIt = typeInfo.reflectedVariables.strings.find(sVariableName);
    if (variableIt == typeInfo.reflectedVariables.strings.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected to find variable named \"{}\"", sVariableName));
    }

    setChildNodeSpacing(EditorTheme::getTypePropertyNameValueSpacing());
    setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    setSize(glm::vec2(getSize().x, 0.05F));
    {
        const auto pTitle = addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorTheme::getTextHeight());
        pTitle->setText(utf::as_u16(EditorTheme::formatVariableName(sVariableName)));

        auto pBackground = addChildNode(std::make_unique<RectUiNode>());
        pBackground->setPadding(EditorTheme::getPadding());
        pBackground->setColor(EditorTheme::getButtonColor());
        {
            const auto pTextEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
            pTextEdit->setTextHeight(EditorTheme::getSmallTextHeight());
            pTextEdit->setText(utf::as_u16(variableIt->second.getter(pObject)));
            pTextEdit->setOnTextChanged([this](std::u16string_view sNewText) {
                // Set new value.
                const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(this->pObject->getTypeGuid());
                const auto variableIt = typeInfo.reflectedVariables.strings.find(this->sVariableName);
                if (variableIt == typeInfo.reflectedVariables.strings.end()) [[unlikely]] {
                    Error::showErrorAndThrowException(
                        std::format("expected to find variable named \"{}\"", this->sVariableName));
                }
                variableIt->second.setter(this->pObject, utf::as_str8(sNewText));
            });
        }
    }
}
