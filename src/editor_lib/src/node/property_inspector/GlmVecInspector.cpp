#include "node/property_inspector/GlmVecInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "misc/ReflectedTypeDatabase.h"
#include "node/property_inspector/PropertyInspector.h"

// External.
#include "utf/utf.hpp"

namespace {
    glm::vec4 getCurrentValue(
        Serializable* pObject, const std::string& sVariableName, GlmVecComponentCount componentCount) {
        const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pObject->getTypeGuid());

        glm::vec4 currentValue = glm::vec4(0.0F);

        switch (componentCount) {
        case (GlmVecComponentCount::VEC2): {
            const auto variableIt = typeInfo.reflectedVariables.vec2s.find(sVariableName);
            if (variableIt == typeInfo.reflectedVariables.vec2s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("expected to find reflected variable \"{}\"", sVariableName));
            }
            currentValue = glm::vec4(variableIt->second.getter(pObject), 0.0F, 0.0F);
            break;
        }
        case (GlmVecComponentCount::VEC3): {
            const auto variableIt = typeInfo.reflectedVariables.vec3s.find(sVariableName);
            if (variableIt == typeInfo.reflectedVariables.vec3s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("expected to find reflected variable \"{}\"", sVariableName));
            }
            currentValue = glm::vec4(variableIt->second.getter(pObject), 0.0F);
            break;
        }
        case (GlmVecComponentCount::VEC4): {
            const auto variableIt = typeInfo.reflectedVariables.vec4s.find(sVariableName);
            if (variableIt == typeInfo.reflectedVariables.vec4s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(
                    std::format("expected to find reflected variable \"{}\"", sVariableName));
            }
            currentValue = variableIt->second.getter(pObject);
            break;
        }
        default: {
            Error::showErrorAndThrowException("unhandled case");
            break;
        }
        };
        return currentValue;
    }
}

void GlmVecInspector::setNewValue(const glm::vec4& value) {
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(pObject->getTypeGuid());

    switch (componentCount) {
    case (GlmVecComponentCount::VEC2): {
        const auto variableIt = typeInfo.reflectedVariables.vec2s.find(sVariableName);
        if (variableIt == typeInfo.reflectedVariables.vec2s.end()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected to find reflected variable \"{}\"", sVariableName));
        }
        variableIt->second.setter(pObject, value);
        break;
    }
    case (GlmVecComponentCount::VEC3): {
        const auto variableIt = typeInfo.reflectedVariables.vec3s.find(sVariableName);
        if (variableIt == typeInfo.reflectedVariables.vec3s.end()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected to find reflected variable \"{}\"", sVariableName));
        }
        variableIt->second.setter(pObject, value);

        if (sVariableName == "worldLocation" || sVariableName == "relativeLocation") {
            const auto pInspector = getParentNodeOfType<PropertyInspector>();
            if (pInspector == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected a valid property inspector");
            }
            pInspector->onAfterInspectedNodeMoved();
        }

        break;
    }
    case (GlmVecComponentCount::VEC4): {
        const auto variableIt = typeInfo.reflectedVariables.vec4s.find(sVariableName);
        if (variableIt == typeInfo.reflectedVariables.vec4s.end()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected to find reflected variable \"{}\"", sVariableName));
        }
        variableIt->second.setter(pObject, value);
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
        break;
    }
    };
}

GlmVecInspector::GlmVecInspector(
    const std::string& sNodeName,
    Serializable* pObject,
    const std::string& sVariableName,
    GlmVecComponentCount componentCount)
    : LayoutUiNode(sNodeName), pObject(pObject), sVariableName(sVariableName),
      componentCount(componentCount) {
    const glm::vec4 currentValue = getCurrentValue(pObject, sVariableName, componentCount);

    setChildNodeSpacing(EditorTheme::getTypePropertyNameValueSpacing());
    setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    setSize(glm::vec2(getSize().x, 0.05F));
    {
        const auto pTitle = addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorTheme::getTextHeight());
        pTitle->setText(utf::as_u16(EditorTheme::formatVariableName(sVariableName)));

        const auto pHorizontalLayout = addChildNode(std::make_unique<LayoutUiNode>());
        pHorizontalLayout->setIsHorizontal(true);
        pHorizontalLayout->setChildNodeSpacing(EditorTheme::getSpacing() * 10.0F);
        pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_MAIN_AXIS);
        {
            auto pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
            pBackground->setPadding(EditorTheme::getPadding());
            pBackground->setColor(EditorTheme::getButtonColor());
            {
                const auto pComponentXEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                pComponentXEdit->setTextHeight(EditorTheme::getSmallTextHeight());
                pComponentXEdit->setHandleNewLineChars(false);
                pComponentXEdit->setText(utf::as_u16(EditorTheme::floatToString(currentValue.x)));
                pComponentXEdit->setOnTextChanged([this, pComponentXEdit](std::u16string_view sNewText) {
                    onValueChanged(pComponentXEdit, VectorComponent::X, sNewText);
                });
            }

            pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
            pBackground->setPadding(EditorTheme::getPadding());
            pBackground->setColor(EditorTheme::getButtonColor());
            {
                const auto pComponentYEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                pComponentYEdit->setTextHeight(EditorTheme::getSmallTextHeight());
                pComponentYEdit->setHandleNewLineChars(false);
                pComponentYEdit->setText(utf::as_u16(EditorTheme::floatToString(currentValue.y)));
                pComponentYEdit->setOnTextChanged([this, pComponentYEdit](std::u16string_view sNewText) {
                    onValueChanged(pComponentYEdit, VectorComponent::Y, sNewText);
                });
            }

            if (componentCount == GlmVecComponentCount::VEC3 ||
                componentCount == GlmVecComponentCount::VEC4) {
                pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
                pBackground->setPadding(EditorTheme::getPadding());
                pBackground->setColor(EditorTheme::getButtonColor());
                {
                    const auto pComponentZEdit =
                        pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                    pComponentZEdit->setTextHeight(EditorTheme::getSmallTextHeight());
                    pComponentZEdit->setHandleNewLineChars(false);
                    pComponentZEdit->setText(utf::as_u16(EditorTheme::floatToString(currentValue.z)));
                    pComponentZEdit->setOnTextChanged([this, pComponentZEdit](std::u16string_view sNewText) {
                        onValueChanged(pComponentZEdit, VectorComponent::Z, sNewText);
                    });
                }
            }

            if (componentCount == GlmVecComponentCount::VEC4) {
                pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
                pBackground->setPadding(EditorTheme::getPadding());
                pBackground->setColor(EditorTheme::getButtonColor());
                {
                    const auto pComponentWEdit =
                        pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                    pComponentWEdit->setTextHeight(EditorTheme::getSmallTextHeight());
                    pComponentWEdit->setHandleNewLineChars(false);
                    pComponentWEdit->setText(utf::as_u16(EditorTheme::floatToString(currentValue.w)));
                    pComponentWEdit->setOnTextChanged([this, pComponentWEdit](std::u16string_view sNewText) {
                        onValueChanged(pComponentWEdit, VectorComponent::W, sNewText);
                    });
                }
            }
        }
    }
}

void GlmVecInspector::onValueChanged(
    TextEditUiNode* pTextEdit, VectorComponent component, std::u16string_view sNewText) {
    // Filter text.
    bool bErasedSomeText = false;
    auto sText = utf::as_str8(sNewText);
    for (auto it = sText.begin(); it != sText.end();) {
        char& item = *it;
        if (!std::isdigit(item) && item != '.' && item != ',' && item != '-') {
            it = sText.erase(it);
            bErasedSomeText = true;
        } else {
            it++;
        }
    }

    if (sText.size() == 1 && sText[0] == '-') {
        sText = "";
    }

    float newComponentValue = 0.0F;
    if (!sText.empty()) {
        try {
            newComponentValue = EditorTheme::stringToFloat(sText);
        } catch (...) {
            Log::error("unable to convert string to float");
            return;
        }
    }

    // Update value.
    glm::vec4 value = getCurrentValue(pObject, sVariableName, componentCount);
    switch (component) {
    case (VectorComponent::X): {
        value.x = newComponentValue;
        break;
    }
    case (VectorComponent::Y): {
        value.y = newComponentValue;
        break;
    }
    case (VectorComponent::Z): {
        value.z = newComponentValue;
        break;
    }
    case (VectorComponent::W): {
        value.w = newComponentValue;
        break;
    }
    }
    setNewValue(value);

    if (bErasedSomeText) {
        // Overwrite invalid text.
        pTextEdit->setText(utf::as_u16(EditorTheme::floatToString(newComponentValue)));
    }
}
