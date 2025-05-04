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
     * Sets if this node should be included in the rendering or not.
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

private:
    /** Position on the screen in range [0.0; 1.0]. */
    glm::vec2 position;

    /** UI layer. */
    UiLayer layer = UiLayer::LAYER1;

    /** Rendered or not. */
    bool bIsVisible = true;
};
