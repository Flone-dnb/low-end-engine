#include "node/property_inspector/UnsignedIntInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

// External.
#include "utf/utf.hpp"

UnsignedIntInspector::UnsignedIntInspector(
    const std::string& sNodeName, Serializable* pObject, const std::string& sVariableName)
    : LayoutUiNode(sNodeName), pObject(pObject), sVariableName(sVariableName) {
    // Get current value.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pObject->getTypeGuid());
    const auto variableIt = typeInfo.reflectedVariables.unsignedInts.find(sVariableName);
    if (variableIt == typeInfo.reflectedVariables.unsignedInts.end()) [[unlikely]] {
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
            pTextEdit->setText(utf::as_u16(std::to_string(variableIt->second.getter(pObject))));
            pTextEdit->setOnTextChanged([this, pTextEdit](std::u16string_view sNewText) {
                // Filter text.
                bool bErasedSomeText = false;
                auto sText = utf::as_str8(sNewText);
                for (auto it = sText.begin(); it != sText.end();) {
                    char& item = *it;
                    if (!std::isdigit(item)) {
                        it = sText.erase(it);
                        bErasedSomeText = true;
                    } else {
                        it++;
                    }
                }

                unsigned int iNewValue = 0;
                if (!sText.empty()) {
                    try {
                        iNewValue = static_cast<unsigned int>(std::stoull(sText));
                    } catch (...) {
                        Logger::get().error("unable to convert string to float");
                        return;
                    }
                }

                // Set new value.
                const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(this->pObject->getTypeGuid());
                const auto variableIt = typeInfo.reflectedVariables.unsignedInts.find(this->sVariableName);
                if (variableIt == typeInfo.reflectedVariables.unsignedInts.end()) [[unlikely]] {
                    Error::showErrorAndThrowException(
                        std::format("expected to find variable named \"{}\"", this->sVariableName));
                }
                variableIt->second.setter(this->pObject, iNewValue);

                if (bErasedSomeText) {
                    // Overwrite invalid text.
                    pTextEdit->setText(utf::as_u16(std::to_string(iNewValue)));
                }
            });
        }
    }
}
