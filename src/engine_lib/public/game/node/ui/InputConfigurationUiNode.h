#pragma once

// Standard.
#include <unordered_map>
#include <string>
#include <variant>

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "input/KeyboardButton.hpp"
#include "input/MouseButton.hpp"
#include "input/GamepadButton.hpp"

/** The most easiest way to add input configuration menu to your game. */
class InputConfigurationUiNode : public LayoutUiNode {
public:
    InputConfigurationUiNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    InputConfigurationUiNode(const std::string& sNodeName);

    virtual ~InputConfigurationUiNode() override = default;

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
     * Called after some input event was changed.
     *
     * @param onChanged Callback.
     */
    void setOnInputChanged(const std::function<void()>& onChanged);

    /**
     * Sets action events to allow configuring.
     *
     * @param actionEvents Pairs of "managed action event ID" - "displayed name".
     */
    void setActionEvents(const std::unordered_map<unsigned int, std::string>& actionEvents);

    /**
     * Sets size of the text.
     *
     * @param newSize Size.
     */
    void setTextHeight(float newSize) { textHeight = newSize; }

    /**
     * Size of the text.
     *
     * @return Size.
     */
    float getTextHeight() const { return textHeight; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     */
    virtual void onSpawning() override;

private:
    /**
     * Returns button name.
     *
     * @param button Button.
     *
     * @return Button name.
     */
    static std::string getButtonName(std::variant<KeyboardButton, MouseButton, GamepadButton> button);

    /** Refreshes displayed options. */
    void refreshDisplayedEvents();

    /** Pairs of "managed action event ID" - "displayed name". */
    std::unordered_map<unsigned int, std::string> actionEventIdsToName;

    /** Called after some input event was changed. */
    std::function<void()> onInputChanged;

    /** Size of the text. */
    float textHeight = 0.03F;
};
