#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/UiNode.h"
#include "math/GLMath.hpp"

/** Single-line or a multi-line text rendering. */
class TextUiNode : public UiNode {
public:
    TextUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    TextUiNode(const std::string& sNodeName);

    virtual ~TextUiNode() override = default;

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
     * Sets text to display.
     *
     * @param sText Text.
     */
    void setText(const std::string& sText);

    /**
     * Sets color of the text.
     *
     * @param color Color in the RGBA format.
     */
    void setTextColor(const glm::vec4& color);

    /**
     * Sets height of the text in range [0.0; 1.0] relative to screen height.
     *
     * @param height Text height.
     */
    void setTextHeight(float height);

    /**
     * Sets vertical space between horizontal lines of text.
     *
     * @param lineSpacing Spacing in range [0.0F; +inf] proportional to the height of the text.
     */
    void setTextLineSpacing(float lineSpacing);

    /**
     * Sets if the text will be automatically transferred to a new line if it does not fit in a single line.
     *
     * @param bIsEnabled `true` to enable word wrap.
     */
    void setIsWordWrapEnabled(bool bIsEnabled);

    /**
     * Sets whether `\n` characters in the text create new lines or not.
     *
     * @param bHandleNewLineChars `true` to allow `\n` characters in the text to create new lines.
     */
    void setHandleNewLineChars(bool bHandleNewLineChars);

    /**
     * Returns displayed text.
     *
     * @return Text.
     */
    std::string_view getText() const { return sText; }

    /**
     * Returns color of the text in the RGBA format.
     *
     * @return Color.
     */
    glm::vec4 getTextColor() const { return color; }

    /**
     * Height of the text in range [0.0; 1.0] relative to screen height.
     *
     * @return Text height.
     */
    float getTextHeight() const { return textHeight; }

    /**
     * Returns vertical space between horizontal lines of text, in range [0.0F; +inf]
     * proportional to the height of the text.
     *
     * @return Line spacing.
     */
    float getTextLineSpacing() const { return lineSpacing; }

    /**
     * Tells if the text will be automatically transferred to a new line if it does not fit in a single line.
     *
     * @return Word wrap state.
     */
    bool getIsWordWrapEnabled() const { return bIsWordWrapEnabled; }

    /**
     * Tells whether `\n` characters in the text create new lines or not.
     *
     * @return `true` to allow `\n` characters in the text to create new lines.
     */
    bool getHandleNewLineChars() const { return bHandleNewLineChars; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark This function is called before any of the child nodes are spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

    /**
     * Called after some child node was attached to this node.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

private:
    /** Color of the text in the RGBA format. */
    glm::vec4 color = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);

    /**
     * Vertical space between horizontal lines of text, in range [0.0F; +inf]
     * proportional to the height of the text.
     */
    float lineSpacing = 0.1F; // NOLINT

    /** Height of the text in range [0.0; 1.0] relative to screen height. */
    float textHeight = 0.035F; // NOLINT

    /** Text to display. */
    std::string sText = "text";

    /** `true` to automatically transfer text to a new line if it does not fit in a single line. */
    bool bIsWordWrapEnabled = false;

    /** `true` to allow `\n` characters in the text to create new lines. */
    bool bHandleNewLineChars = true;
};
