#pragma once

// Custom.
#include "game/node/Node.h"
#include "render/UiLayer.hpp"

/** Base class for UI nodes. Provides functionality for positioning on the screen. */
class UiNode : public Node {
public:
    UiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    UiNode(const std::string& sNodeName);

    virtual ~UiNode() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets position on the screen in range [0.0; 1.0].
     *
     * @param position New position.
     */
    void setPosition(const glm::vec2& position);

    /**
     * Sets width and height in range [0.0; 1.0].
     *
     * @param size New size.
     */
    void setSize(const glm::vec2& size);

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     *
     * @param iPortion Positive value (can be bigger than 1 to fill more space relative to other nodes).
     */
    void setExpandPortionInLayout(unsigned int iPortion);

    /**
     * Sets if this node (and all child nodes) should be included in the rendering or not.
     *
     * @param bIsVisible New visibility.
     */
    void setIsVisible(bool bIsVisible);

    /**
     * Sets UI layer to use.
     *
     * @param layer UI layer.
     */
    void setUiLayer(UiLayer layer);

    /**
     * Returns position on the screen in range [0.0; 1.0].
     *
     * @return Position.
     */
    glm::vec2 getPosition() const { return position; }

    /**
     * Returns width and height in range [0.0; 1.0].
     *
     * @return Width and height.
     */
    glm::vec2 getSize() const { return size; }

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     *
     * @return Fill portion.
     */
    unsigned int getExpandPortionInLayout() const { return iExpandPortionInLayout; }

    /**
     * Tells if this node is included in the rendering or not.
     *
     * @return Visibility.
     */
    bool isVisible() const { return bIsVisible; }

    /**
     * Returns UI layer that this node uses.
     *
     * @return UI layer.
     */
    UiLayer getUiLayer() const { return layer; }

protected:
    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() {}

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() {}

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

private:
    /** Width and height in range [0.0; 1.0]. */
    glm::vec2 size = glm::vec2(0.2F, 0.2F); // NOLINT

    /** Position on the screen in range [0.0; 1.0]. */
    glm::vec2 position = glm::vec2(0.2F, 0.5F); // NOLINT

    /**
     * When this node is a child node of a layout node with an "expand child nodes rule" this value defines a
     * portion of the remaining (free) space in the layout to fill (relative to other nodes).
     */
    unsigned int iExpandPortionInLayout = 1;

    /** UI layer. */
    UiLayer layer = UiLayer::LAYER1;

    /** Rendered or not. */
    bool bIsVisible = true;
};
