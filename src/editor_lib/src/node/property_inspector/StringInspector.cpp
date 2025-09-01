#include "node/property_inspector/StringInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"
#include "EditorGameInstance.h"
#include "node/node_tree_inspector/NodeTreeInspector.h"

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

    const auto bChangingNodeName = sVariableName == "sNodeName";

    setChildNodeSpacing(EditorTheme::getTypePropertyNameValueSpacing());
    setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    setSize(glm::vec2(getSize().x, 0.045F));
    {
        const auto pTitle = addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorTheme::getTextHeight());
        pTitle->setText(utf::as_u16(EditorTheme::formatVariableName(sVariableName)));
        pTitle->setSize(glm::vec2(pTitle->getSize().x, EditorTheme::getSmallTextHeight() * 1.25F));

        auto pBackground = addChildNode(std::make_unique<RectUiNode>());
        pBackground->setPadding(EditorTheme::getPadding());
        pBackground->setColor(EditorTheme::getButtonColor());
        pBackground->setSize(glm::vec2(pBackground->getSize().x, getSize().y));
        {
            const auto pTextEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
            pTextEdit->setTextHeight(EditorTheme::getSmallTextHeight());
            pTextEdit->setSize(glm::vec2(pTextEdit->getSize().x, EditorTheme::getSmallTextHeight() * 1.25F));
            pTextEdit->setText(utf::as_u16(variableIt->second.getter(pObject)));
            pTextEdit->setHandleNewLineChars(false);
            pTextEdit->setOnTextChanged([bChangingNodeName, this](std::u16string_view sNewText) {
                // Set new value.
                const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(this->pObject->getTypeGuid());
                const auto variableIt = typeInfo.reflectedVariables.strings.find(this->sVariableName);
                if (variableIt == typeInfo.reflectedVariables.strings.end()) [[unlikely]] {
                    Error::showErrorAndThrowException(
                        std::format("expected to find variable named \"{}\"", this->sVariableName));
                }
                variableIt->second.setter(this->pObject, utf::as_str8(sNewText));

                if (bChangingNodeName) {
                    const auto pGameInstance =
                        dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
                    if (pGameInstance == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("expected editor game instance");
                    }
                    pGameInstance->getNodeTreeInspector()->refreshGameNodeName(
                        dynamic_cast<Node*>(this->pObject));
                }
            });
        }
    }
}
