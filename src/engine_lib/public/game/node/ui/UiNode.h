#pragma once

// Custom.
#include "game/node/Node.h"

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
     * Returns position on the screen in range [0.0; 1.0].
     *
     * @return Position.
     */
    glm::vec2 getPosition() const;

    /**
     * Tells if this node is included in the rendering or not.
     *
     * @return Visibility.
     */
    bool isVisible() const;

protected:
    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() {}

private:
    /** Position on the screen in range [0.0; 1.0]. */
    glm::vec2 position;

    /** Rendered or not. */
    bool bIsVisible = true;
};
