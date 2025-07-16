#include "node/property_inspector/PropertyInspector.h"

// Custom.
#include "misc/ReflectedTypeDatabase.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorColorTheme.h"
#include "node/property_inspector/GlmVecInspector.h"

// External.
#include "utf/utf.hpp"

PropertyInspector::PropertyInspector() : PropertyInspector("Property Inspector") {}
PropertyInspector::PropertyInspector(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(EditorColorTheme::getEditorBackgroundColor());

    pPropertyLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pPropertyLayout->setPadding(EditorColorTheme::getPadding());
    pPropertyLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pPropertyLayout->setIsScrollBarEnabled(true);
    pPropertyLayout->setChildNodeSpacing(EditorColorTheme::getSpacing() * 2.0F);
}

void PropertyInspector::setNodeToInspect(Node* pNode) {
    pInspectedNode = pNode;

    // Clear currently displayed properties.
    {
        const auto mtxChildNodes = pPropertyLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            pNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    if (pInspectedNode == nullptr) {
        return;
    }

    displayPropertiesForTypeRecursive(pNode->getTypeGuid(), pNode);
}

void PropertyInspector::displayPropertiesForTypeRecursive(const std::string& sTypeGuid, Node* pObject) {
    const auto pGroupBackground = pPropertyLayout->addChildNode(std::make_unique<RectUiNode>());
    pGroupBackground->setPadding(EditorColorTheme::getPadding() / 2.0F);
    pGroupBackground->setColor(EditorColorTheme::getContainerBackgroundColor());

    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

    const auto pTypeVariablesLayout =
        pGroupBackground->addChildNode(std::make_unique<LayoutUiNode>("type group"));
    pTypeVariablesLayout->setChildNodeSpacing(EditorColorTheme::getSpacing());
    pTypeVariablesLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    {
        const auto pGroupTitle = pTypeVariablesLayout->addChildNode(std::make_unique<TextUiNode>());
        pGroupTitle->setTextHeight(EditorColorTheme::getSmallTextHeight());
        pGroupTitle->setSize(glm::vec2(pGroupTitle->getSize().x, pGroupTitle->getTextHeight() * 1.4F));
        pGroupTitle->setText(utf::as_u16(typeInfo.sTypeName));
        pGroupTitle->setTextColor(glm::vec4(glm::vec3(pGroupTitle->getTextColor()), 0.5F));

        const auto pTypePropertiesLayout =
            pTypeVariablesLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pTypePropertiesLayout->setChildNodeSpacing(EditorColorTheme::getTypePropertySpacing());
        pTypePropertiesLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);

#define CONTINUE_IF_PARENT_VAR(array)                                                                        \
    if (!typeInfo.sParentTypeGuid.empty()) {                                                                 \
        const auto& parentTypeInfo = ReflectedTypeDatabase::getTypeInfo(typeInfo.sParentTypeGuid);           \
        const auto it = parentTypeInfo.reflectedVariables.array.find(sVariableName);                         \
        if (it != parentTypeInfo.reflectedVariables.array.end()) {                                           \
            continue;                                                                                        \
        }                                                                                                    \
    }

        {
            // Display fields (ignore inherited).
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec4s) {
                CONTINUE_IF_PARENT_VAR(vec4s);
                pTypePropertiesLayout->addChildNode(std::make_unique<GlmVecInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName),
                    pObject,
                    sVariableName,
                    GlmVecComponentCount::VEC4));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec3s) {
                CONTINUE_IF_PARENT_VAR(vec3s);
                pTypePropertiesLayout->addChildNode(std::make_unique<GlmVecInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName),
                    pObject,
                    sVariableName,
                    GlmVecComponentCount::VEC3));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec2s) {
                CONTINUE_IF_PARENT_VAR(vec2s);
                pTypePropertiesLayout->addChildNode(std::make_unique<GlmVecInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName),
                    pObject,
                    sVariableName,
                    GlmVecComponentCount::VEC2));
            }

#if defined(WIN32) && defined(DEBUG)
            static_assert(sizeof(ReflectedVariables) == 896, "add new variables here");
#elif defined(DEBUG)
            static_assert(sizeof(ReflectedVariables) == 784, "add new variables here");
#endif
        }
    }

    if (!typeInfo.sParentTypeGuid.empty()) {
        displayPropertiesForTypeRecursive(typeInfo.sParentTypeGuid, pObject);
    }
}
