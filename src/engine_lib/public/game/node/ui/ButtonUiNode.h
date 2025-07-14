#pragma once

// Standard.
#include <string>
#include <functional>

// Custom.
#include "game/node/ui/RectUiNode.h"
#include "math/GLMath.hpp"

class TextureHandle;

/** Interactable UI node that reacts to mouse clicks. */
class ButtonUiNode : public RectUiNode {
public:
    ButtonUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    ButtonUiNode(const std::string& sNodeName);

    virtual ~ButtonUiNode() override = default;

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
     * Sets color to be used while the mouse cursor is floating over the node.
     *
     * @param color RGBA color.
     */
    void setColorWhileHovered(const glm::vec4& color);

    /**
     * Sets color to be used while being pressed by cursor.
     *
     * @param color RGBA color.
     */
    void setColorWhilePressed(const glm::vec4& color);

    /**
     * Sets path to texture to display while the mouse cursor is hovering over the node.
     *
     * @param sPathToTextureRelativeRes Path to the texture file relative to the `res` directory.
     */
    void setPathToTextureWhileHovered(std::string sPathToTextureRelativeRes);

    /**
     * Sets path to texture to display while the node is being pressed.
     *
     * @param sPathToTextureRelativeRes Path to the texture file relative to the `res` directory.
     */
    void setPathToTextureWhilePressed(std::string sPathToTextureRelativeRes);

    /**
     * Sets callback to call after a click on this UI node.
     *
     * @param onClicked Callback.
     */
    void setOnClicked(const std::function<void()>& onClicked);

    /**
     * Returns color used while the mouse cursor is floating over the node.
     *
     * @return RGBA color.
     */
    glm::vec4 getColorWhileHovered() const { return colorWhileHovered; }

    /**
     * Returns color used while being pressed by cursor.
     *
     * @return RGBA color.
     */
    glm::vec4 getColorWhilePressed() const { return colorWhilePressed; }

    /**
     * Returns path to the texture (relative to the `res` directory) to display while hovered.
     *
     * @return Empty if not used.
     */
    std::string getPathToTextureWhileHovered() const { return sPathToTextureWhileHovered; }

    /**
     * Returns path to the texture (relative to the `res` directory) to display while pressed.
     *
     * @return Empty if not used.
     */
    std::string getPathToTextureWhilePressed() const { return sPathToTextureWhilePressed; }

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

    /**
     * Called when the window receives mouse button press event while floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     * @remark This function will only be called if the user clicked on this UI node.
     *
     * @param button    Mouse button.
     * @param modifiers Keyboard modifier keys.
     *
     * @return `true` if the event was handled.
     */
    virtual bool onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override;

    /**
     * Same as @ref onMouseButtonPressedOnUiNode but for mouse button released event.
     *
     * @param button    Mouse button.
     * @param modifiers Keyboard modifier keys.
     *
     * @return `true` if the event was handled.
     */
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override;

    /**
     * Called when the mouse cursor started floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseEntered() override;

    /**
     * Called when the mouse cursor stopped floating over this UI node.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     */
    virtual void onMouseLeft() override;

    /** Called after color was changed while the node was spawned. */
    virtual void onColorChangedWhileSpawned() override;

    /** Called after texture was changed while the node was spawned. */
    virtual void onTextureChangedWhileSpawned() override;

    /** Called after node's visibility was changed. */
    virtual void onVisibilityChanged() override;

private:
    /**
     * Should be used instead of the default setColor function from the parent.
     *
     * @param color Color to display.
     */
    void setButtonColor(const glm::vec4& color);

    /**
     * Should be used instead of the default setTexture function from the parent.
     *
     * @param sPathToTexture Texture to display (path relative to the `res` directory).
     */
    void setButtonTexture(const std::string& sPathToTexture);

    /**
     * "Shortcut" for loading texture handles.
     *
     * @param sPathToTexture Path to texture to load.
     *
     * @return Loaded handle.
     */
    std::unique_ptr<TextureHandle> getTextureHandle(const std::string& sPathToTexture);

    /** Callback to call after a click on this UI node. */
    std::function<void()> onClicked;

    /** Handle to @ref sTempPathToDefaultTexture. Used to keep the texture in cache. */
    std::unique_ptr<TextureHandle> pDefaultTexture;

    /** Handle to @ref sPathToTextureWhileHovered. Used to keep the texture in cache. */
    std::unique_ptr<TextureHandle> pHoveredTexture;

    /** Handle to @ref sPathToTextureWhilePressed. Used to keep the texture in cache. */
    std::unique_ptr<TextureHandle> pPressedTexture;

    /** Color while mouse cursor is floating over the node. */
    glm::vec4 colorWhileHovered = glm::vec4(0.4F, 0.4F, 0.4F, 1.0F); // NOLINT

    /** Color while pressed. */
    glm::vec4 colorWhilePressed = glm::vec4(0.2F, 0.2F, 0.2F, 1.0F); // NOLINT

    /** Temporary variable used while being hovered/clicked to restore the original color. */
    glm::vec4 tempDefaultColor = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);

    /** Texture to display while mouse cursor is floating over the node. */
    std::string sPathToTextureWhileHovered;

    /** Texture to display while being clicked. */
    std::string sPathToTextureWhilePressed;

    /** Temporary variable used while being hovered/clicked to restore the original texture. */
    std::string sTempPathToDefaultTexture;

    /** `true` if the mouse is currently floating over this node. */
    bool bIsCurrentlyHovered = false;

    /** Used to determine if this node is changing color/texture (calling parent function). */
    bool bIsChangingColorTexture = false;
};
