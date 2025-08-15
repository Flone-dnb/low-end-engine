#include "node/property_inspector/PropertyInspector.h"

// Custom.
#include "misc/ReflectedTypeDatabase.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorTheme.h"
#include "EditorGameInstance.h"
#include "node/GizmoNode.h"
#include "node/property_inspector/GlmVecInspector.h"
#include "node/property_inspector/StringInspector.h"
#include "node/property_inspector/FloatInspector.h"
#include "node/property_inspector/UnsignedLongLongInspector.h"
#include "node/property_inspector/LongLongInspector.h"
#include "node/property_inspector/UnsignedIntInspector.h"
#include "node/property_inspector/IntInspector.h"
#include "node/property_inspector/BoolInspector.h"

// External.
#include "utf/utf.hpp"

PropertyInspector::PropertyInspector() : PropertyInspector("Property Inspector") {}
PropertyInspector::PropertyInspector(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(EditorTheme::getEditorBackgroundColor());

    pPropertyLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pPropertyLayout->setPadding(EditorTheme::getPadding());
    pPropertyLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pPropertyLayout->setIsScrollBarEnabled(true);
    pPropertyLayout->setChildNodeSpacing(EditorTheme::getTypePropertyGroupSpacing());
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

void PropertyInspector::onAfterInspectedNodeMoved() {
    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance");
    }

    const auto pGizmoNode = pGameInstance->getGizmoNode();
    if (pGizmoNode == nullptr) {
        return;
    }

    const auto pSpatialNode = dynamic_cast<SpatialNode*>(pInspectedNode);
    if (pSpatialNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a spatial node");
    }

    pGizmoNode->setWorldLocation(pSpatialNode->getWorldLocation());
}

void PropertyInspector::refreshInspectedProperties() {
    if (pInspectedNode == nullptr) {
        return;
    }

    // Clear currently displayed properties.
    {
        const auto mtxChildNodes = pPropertyLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            pNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    displayPropertiesForTypeRecursive(pInspectedNode->getTypeGuid(), pInspectedNode);
}

void PropertyInspector::displayPropertiesForTypeRecursive(const std::string& sTypeGuid, Node* pObject) {
    auto pGroupBackground = std::make_unique<RectUiNode>();
    pGroupBackground->setPadding(EditorTheme::getPadding() / 2.0F);
    pGroupBackground->setColor(EditorTheme::getContainerBackgroundColor());

    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

    const auto pTypeGroupLayout = pGroupBackground->addChildNode(
        std::make_unique<LayoutUiNode>(std::format("type group {}", typeInfo.sTypeName)));
    pTypeGroupLayout->setChildNodeSpacing(EditorTheme::getSpacing());
    pTypeGroupLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    {
        const auto pGroupTitle = pTypeGroupLayout->addChildNode(std::make_unique<TextUiNode>());
        pGroupTitle->setTextHeight(EditorTheme::getSmallTextHeight());
        pGroupTitle->setSize(glm::vec2(pGroupTitle->getSize().x, pGroupTitle->getTextHeight() * 1.4F));
        pGroupTitle->setText(utf::as_u16(typeInfo.sTypeName));
        pGroupTitle->setTextColor(glm::vec4(glm::vec3(pGroupTitle->getTextColor()), 0.5F));

        const auto pTypePropertiesLayout = pTypeGroupLayout->addChildNode(std::make_unique<LayoutUiNode>());
        pTypePropertiesLayout->setChildNodeSpacing(EditorTheme::getTypePropertySpacing());
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
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.strings) {
                CONTINUE_IF_PARENT_VAR(strings);
                pTypePropertiesLayout->addChildNode(std::make_unique<StringInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.floats) {
                CONTINUE_IF_PARENT_VAR(floats);
                pTypePropertiesLayout->addChildNode(std::make_unique<FloatInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.unsignedLongLongs) {
                CONTINUE_IF_PARENT_VAR(unsignedLongLongs);
                pTypePropertiesLayout->addChildNode(std::make_unique<UnsignedLongLongInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.longLongs) {
                CONTINUE_IF_PARENT_VAR(longLongs);
                pTypePropertiesLayout->addChildNode(std::make_unique<LongLongInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.unsignedInts) {
                CONTINUE_IF_PARENT_VAR(unsignedInts);
                pTypePropertiesLayout->addChildNode(std::make_unique<UnsignedIntInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.ints) {
                CONTINUE_IF_PARENT_VAR(ints);
                pTypePropertiesLayout->addChildNode(std::make_unique<IntInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }
            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.bools) {
                CONTINUE_IF_PARENT_VAR(bools);
                pTypePropertiesLayout->addChildNode(std::make_unique<BoolInspector>(
                    std::format("inspector for variable \"{}\"", sVariableName), pObject, sVariableName));
            }

#if defined(WIN32) && defined(DEBUG)
            static_assert(sizeof(ReflectedVariables) == 896, "consider adding new variables here");
#elif defined(DEBUG)
            static_assert(sizeof(ReflectedVariables) == 784, "consider adding new variables here");
#endif
        }
    }

    pPropertyLayout->addChildNode(std::move(pGroupBackground));

    if (!typeInfo.sParentTypeGuid.empty()) {
        displayPropertiesForTypeRecursive(typeInfo.sParentTypeGuid, pObject);
    }
}
