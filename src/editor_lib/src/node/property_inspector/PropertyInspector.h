#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;
class SkeletonNode;
class CollisionNode;

/** Displays reflected fields of an object. */
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

    /** Called after the inspected node changed its location. */
    void onAfterInspectedNodeMoved();

    /** Clears and displays all inspected properties again (if inspected node was previously set). */
    void refreshInspectedProperties();

    /**
     * Returns currently inspected node (if exists).
     *
     * @return `nullptr` if not inspecting.
     */
    Node* getInspectedNode() const { return pInspectedNode; }

private:
    /**
     * Displays reflected fields of the specified type by taking the current
     * values from the specified object.
     *
     * @param sTypeGuid  GUID of the type to display.
     * @param pObject    Object to read/write fields.
     * @param bRecursive `true` to display parent variables.
     */
    void
    displayPropertiesForType(const std::string& sTypeGuid, Serializable* pObject, bool bRecursive = true);

    /**
     * Adds special UI options to @ref pPropertyLayout for skeleton node.
     *
     * @param pSkeletonNode Node.
     */
    void addSkeletonNodeSpecialOptions(SkeletonNode* pSkeletonNode);

    /**
     * Adds special UI options to @ref pPropertyLayout for collision node.
     *
     * @param pCollisionNode Node.
     */
    void addCollisionNodeSpecialOptions(CollisionNode* pCollisionNode);

    /** Layout to add properties. */
    LayoutUiNode* pPropertyLayout = nullptr;

    /** `nullptr` if nothing displayed. */
    Node* pInspectedNode = nullptr;
};
