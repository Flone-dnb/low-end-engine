#include "node/property_inspector/BoolInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/CheckboxUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

// External.
#include "utf/utf.hpp"

BoolInspector::BoolInspector(
    const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName)
    : LayoutUiNode(sNodeName), pObject(pObject), sVariableName(sVariableName) {
    // Get current value.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pObject->getTypeGuid());
    const auto variableIt = typeInfo.reflectedVariables.bools.find(sVariableName);
    if (variableIt == typeInfo.reflectedVariables.bools.end()) [[unlikely]] {
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

        const auto pCheckbox = addChildNode(std::make_unique<CheckboxUiNode>());
        pCheckbox->setIsChecked(variableIt->second.getter(pObject));
        pCheckbox->setOnStateChanged([this](bool bNewValue) {
            // Set new value.
            const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(this->pObject->getTypeGuid());
            const auto variableIt = typeInfo.reflectedVariables.bools.find(this->sVariableName);
            if (variableIt == typeInfo.reflectedVariables.bools.end()) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("expected to find variable named \"{}\"", this->sVariableName));
            }
            variableIt->second.setter(this->pObject, bNewValue);
        });
    }
}
