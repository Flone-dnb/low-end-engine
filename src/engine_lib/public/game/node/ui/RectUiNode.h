#pragma once

// Standard.
#include <string>

// Custom.
#include "game/node/ui/UiNode.h"
#include "math/GLMath.hpp"
#include "material/TextureHandle.h"

/** Screen quad with a custom fragment shader. */
class RectUiNode : public UiNode {
    // Needs access to texture handle.
    friend class UiNodeManager;

public:
    RectUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    RectUiNode(const std::string& sNodeName);

    virtual ~RectUiNode() override = default;

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
     * Set color to fill the UI node area.
     *
     * @param color RGBA color.
     */
    void setColor(const glm::vec4& color);

    /**
     * Sets path to texture to load (if it was not loaded in the memory yet) when spawning and use.
     *
     * @param sPathToTextureRelativeRes Path to the texture file relative to the `res` directory.
     */
    void setPathToTexture(std::string sPathToTextureRelativeRes);

    /**
     * Sets padding for child node in range [0.0; 0.5] where 1.0 is full size of the node.
     *
     * @param padding Padding.
     */
    void setPadding(float padding);

    /**
     * Returns color to fill the UI node area.
     *
     * @return RGBA color.
     */
    glm::vec4 getColor() const { return color; }

    /**
     * Returns path to the texture (relative to the `res` directory) to display.
     *
     * @return Empty if not used.
     */
    std::string getPathToTexture() const { return sPathToTextureRelativeRes; }

    /**
     * Returns padding for child node in range [0.0; 0.5] where 1.0 is full size of the node.
     *
     * @return Padding.
     */
    float getPadding() const { return padding; }

protected:
    /** Called after color was changed while the node was spawned. */
    virtual void onColorChangedWhileSpawned() {}

    /** Called after texture was changed while the node was spawned. */
    virtual void onTextureChangedWhileSpawned() {}

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

    /** Called after position of this UI node was changed. */
    virtual void onAfterPositionChanged() override;

    /** Called after size of this UI node was changed. */
    virtual void onAfterSizeChanged() override;

private:
    /** Updates position and size of the child node to the current position and size of the node. */
    void updateChildNodePosAndSize();

    /** Not `nullptr` if texture from @ref sPathToTextureRelativeRes is loaded. */
    std::unique_ptr<TextureHandle> pTexture;

    /** Padding for child node in range [0.0; 0.5] where 1.0 is full size of the node. */
    float padding = 0.0F;

    /** Fill color. */
    glm::vec4 color = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);

    /** Empty if not used, otherwise path to the texture (relative the `res` directory) to load. */
    std::string sPathToTextureRelativeRes;
};
