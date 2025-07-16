#include "node/property_inspector/GlmVecInspector.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorColorTheme.h"
#include "misc/ReflectedTypeDatabase.h"

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

    void setNewValue(
        const glm::vec4& value,
        Serializable* pObject,
        const std::string& sVariableName,
        GlmVecComponentCount componentCount) {
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

}

GlmVecInspector::GlmVecInspector(
    const std::string& sNodeName,
    Serializable* pObject,
    const std::string& sVariableName,
    GlmVecComponentCount componentCount)
    : LayoutUiNode(sNodeName), pObject(pObject), sVariableName(sVariableName),
      componentCount(componentCount) {
    const glm::vec4 currentValue = getCurrentValue(pObject, sVariableName, componentCount);

    setChildNodeSpacing(EditorColorTheme::getTypePropertyNameValueSpacing());
    setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
    setSize(glm::vec2(getSize().x, 0.05F));
    {
        const auto pTitle = addChildNode(std::make_unique<TextUiNode>());
        pTitle->setTextHeight(EditorColorTheme::getTextHeight());
        pTitle->setText(utf::as_u16(EditorColorTheme::formatVariableName(sVariableName)));

        const auto pHorizontalLayout = addChildNode(std::make_unique<LayoutUiNode>());
        pHorizontalLayout->setIsHorizontal(true);
        pHorizontalLayout->setChildNodeSpacing(EditorColorTheme::getSpacing() * 10.0F);
        pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_MAIN_AXIS);
        {
            auto pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
            pBackground->setPadding(EditorColorTheme::getPadding());
            pBackground->setColor(EditorColorTheme::getButtonColor());
            {
                const auto pComponentXEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                pComponentXEdit->setTextHeight(EditorColorTheme::getSmallTextHeight());
                pComponentXEdit->setText(utf::as_u16(EditorColorTheme::floatToString(currentValue.x)));
                pComponentXEdit->setOnTextChanged([this, pComponentXEdit](std::u16string_view sNewText) {
                    onValueChanged(pComponentXEdit, VectorComponent::X, sNewText);
                });
            }

            pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
            pBackground->setPadding(EditorColorTheme::getPadding());
            pBackground->setColor(EditorColorTheme::getButtonColor());
            {
                const auto pComponentYEdit = pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                pComponentYEdit->setTextHeight(EditorColorTheme::getSmallTextHeight());
                pComponentYEdit->setText(utf::as_u16(EditorColorTheme::floatToString(currentValue.y)));
                pComponentYEdit->setOnTextChanged([this, pComponentYEdit](std::u16string_view sNewText) {
                    onValueChanged(pComponentYEdit, VectorComponent::Y, sNewText);
                });
            }

            if (componentCount == GlmVecComponentCount::VEC3 ||
                componentCount == GlmVecComponentCount::VEC4) {
                pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
                pBackground->setPadding(EditorColorTheme::getPadding());
                pBackground->setColor(EditorColorTheme::getButtonColor());
                {
                    const auto pComponentZEdit =
                        pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                    pComponentZEdit->setTextHeight(EditorColorTheme::getSmallTextHeight());
                    pComponentZEdit->setText(utf::as_u16(EditorColorTheme::floatToString(currentValue.z)));
                    pComponentZEdit->setOnTextChanged([this, pComponentZEdit](std::u16string_view sNewText) {
                        onValueChanged(pComponentZEdit, VectorComponent::Z, sNewText);
                    });
                }
            }

            if (componentCount == GlmVecComponentCount::VEC4) {
                pBackground = pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
                pBackground->setPadding(EditorColorTheme::getPadding());
                pBackground->setColor(EditorColorTheme::getButtonColor());
                {
                    const auto pComponentWEdit =
                        pBackground->addChildNode(std::make_unique<TextEditUiNode>());
                    pComponentWEdit->setTextHeight(EditorColorTheme::getSmallTextHeight());
                    pComponentWEdit->setText(utf::as_u16(EditorColorTheme::floatToString(currentValue.w)));
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
    auto sText = utf::as_str8(sNewText);
    for (auto it = sText.begin(); it != sText.end();) {
        char& item = *it;
        if (!std::isdigit(item) && item != '.' && item != ',') {
            it = sText.erase(it);
        } else {
            it++;
        }
    }

    float newComponentValue = 0.0F;
    try {
        newComponentValue = std::stof(sText);
    } catch (...) {
        Logger::get().error("unable to convert string to float");
        return;
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
    setNewValue(value, pObject, sVariableName, componentCount);

    // Display updated value.
    pTextEdit->setText(utf::as_u16(EditorColorTheme::floatToString(newComponentValue)));
}
