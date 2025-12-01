#pragma once

// Custom.
#include "game/node/ui/RectUiNode.h"

/** Displays 2 colors/images one is background and the other is foreground (shows percent of the progress). */
class ProgressBarUiNode : public RectUiNode {
    // Needs access to texture handle.
    friend class UiNodeManager;

public:
    ProgressBarUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    ProgressBarUiNode(const std::string& sNodeName);

    virtual ~ProgressBarUiNode() override = default;

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
     * Set foreground color.
     *
     * @param color RGBA color.
     */
    void setForegroundColor(const glm::vec4& color);

    /**
     * Sets value in range [0.0; 1.0].
     *
     * @param progress New progress value.
     */
    void setProgressFactor(float progress);

    /**
     * Sets path to foreground texture to load (if it was not loaded in the memory yet) when spawning and use.
     *
     * @param sNewPathToForegroundTextureRelativeRes Path to the texture file relative to the `res` directory.
     */
    void setPathToForegroundTexture(std::string sNewPathToForegroundTextureRelativeRes);

    /**
     * Returns foreground color.
     *
     * @return RGBA color.
     */
    glm::vec4 getForegroundColor() const { return foregroundColor; }

    /**
     * Returns value in range [0.0; 1.0].
     *
     * @return Progress.
     */
    float getProgressFactor() const { return progressFactor; }

    /**
     * Returns path to the texture (relative to the `res` directory) to display.
     *
     * @return Empty if not used.
     */
    std::string getPathToForegroundTexture() const { return sPathToForegroundTextureRelativeRes; }

    /**
     * Returns the maximum number of child nodes this type allows. This is generally 0, 1, or +inf.
     *
     * @return Number of child nodes.
     */
    virtual size_t getMaxChildCount() const override { return 1; }

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

private:
    /** Not `nullptr` if texture from @ref sPathToForegroundTextureRelativeRes is loaded. */
    std::unique_ptr<TextureHandle> pForegroundTexture;

    /** Foreground color. */
    glm::vec4 foregroundColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    /** Empty if not used, otherwise path to the texture (relative the `res` directory) to load. */
    std::string sPathToForegroundTextureRelativeRes;

    /** Value in range [0.0; 1.0]. */
    float progressFactor = 0.5f;
};