#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/UiNode.h"
#include "math/GLMath.hpp"

/** 2D text rendering. */
class TextNode : public UiNode {
public:
    TextNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    TextNode(const std::string& sNodeName);

    virtual ~TextNode() override = default;

    /**
     * Sets text to display.
     *
     * @param sText Text.
     */
    void setText(const std::string& sText);

    /**
     * Sets size of the text.
     *
     * @param size Size in range [0.0F; 1.0F].
     */
    void setTextSize(float size);

    /**
     * Sets color of the text.
     *
     * @param color Color in the RGBA format.
     */
    void setTextColor(const glm::vec4& color);

    /**
     * Sets vertical space between horizontal lines of text.
     *
     * @param lineSpacing Spacing in range [0.0F; +inf] proportional to the height of the text.
     */
    void setLineSpacing(float lineSpacing);

    /**
     * Returns displayed text.
     *
     * @return Text.
     */
    std::string_view getText() const { return sText; }

    /**
     * Returns size of the text in range [0.0F; 1.0F]
     *
     * @return Size.
     */
    float getTextSize() const { return size; }

    /**
     * Returns color of the text in the RGBA format.
     *
     * @return Color.
     */
    glm::vec4 getTextColor() const { return color; }

    /**
     * Returns vertical space between horizontal lines of text, in range [0.0F; +inf]
     * proportional to the height of the text.
     *
     * @return Line spacing.
     */
    float getLineSpacing() const { return lineSpacing; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your login) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

private:
    /** Color of the text in the RGBA format. */
    glm::vec4 color = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);

    /** Size in range [0.0F; 1.0F] proportional to screen height. */
    float size = 0.05F;

    /**
     * Vertical space between horizontal lines of text, in range [0.0F; +inf]
     * proportional to the height of the text.
     */
    float lineSpacing = 0.25F;

    /** Text to display. */
    std::string sText;
};
