#include "node/property_inspector/PropertyInspector.h"

// Custom.
#include "misc/ReflectedTypeDatabase.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "EditorColorTheme.h"

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
    pGroupBackground->setPadding(EditorColorTheme::getPadding());
    pGroupBackground->setColor(EditorColorTheme::getContainerBackgroundColor());

    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);

    const auto pLayout = pGroupBackground->addChildNode(std::make_unique<LayoutUiNode>());
    pLayout->setChildNodeSpacing(EditorColorTheme::getSpacing());
    {
        const auto pGroupTitle = pLayout->addChildNode(std::make_unique<TextUiNode>());
        pGroupTitle->setTextHeight(EditorColorTheme::getTextHeight() * 0.9F);
        pGroupTitle->setText(utf::as_u16(typeInfo.sTypeName));
        pGroupTitle->setTextColor(glm::vec4(glm::vec3(pGroupTitle->getTextColor()), 0.5F));

        // Display fields (ignore inherited).
        // TODO;
    }

    if (!typeInfo.sParentTypeGuid.empty()) {
        displayPropertiesForTypeRecursive(typeInfo.sParentTypeGuid, pObject);
    }
}
