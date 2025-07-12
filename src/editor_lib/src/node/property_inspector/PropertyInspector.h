#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;

class PropertyInspector : public RectUiNode {
public:
    PropertyInspector();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    PropertyInspector(const std::string& sNodeName);

    virtual ~PropertyInspector() override = default;

    /**
     * Sets node which properties (reflected properties) to display.
     * Specify `nullptr` to clear inspected item.
     *
     * @param pNode Node to inspect.
     */
    void setNodeToInspect(Node* pNode);

private:
    /**
     * Displays reflected fields of the specified type (ignoring inherited fields) by taking the current
     * values from the specified object.
     *
     * @param sTypeGuid GUID of the type to display.
     * @param pObject   Object to read/write fields.
     */
    void displayPropertiesForTypeRecursive(const std::string& sTypeGuid, Node* pObject);

    /** Layout to add properties. */
    LayoutUiNode* pPropertyLayout = nullptr;

    /** `nullptr` if nothing displayed. */
    Node* pInspectedNode = nullptr;
};
