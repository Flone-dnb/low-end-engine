#pragma once

// Custom.
#include "game/GameInstance.h"

class Window;
class GameManager;
class EditorCameraNode;
class TextNode;

/** Editor's game instance. */
class EditorGameInstance : public GameInstance {
public:
    /**
     * Creates a new game instance.
     *
     * @param pWindow Do not delete (free). Valid pointer to the game's window.
     */
    EditorGameInstance(Window* pWindow);

    virtual ~EditorGameInstance() override = default;

    /**
     * Returns title of the editor's window.
     *
     * @return Window title.
     */
    static const char* getEditorWindowTitle();

protected:
    /**
     * Called after GameInstance's constructor is finished and created
     * GameInstance object was saved in GameManager (that owns GameInstance).
     *
     * At this point you can create and interact with the game world and etc.
     */
    virtual void onGameStarted() override;

    /**
     * Called after a gamepad controller was connected.
     *
     * @param sGamepadName Name of the connected gamepad.
     */
    virtual void onGamepadConnected(std::string_view sGamepadName) override;

    /** Called after a gamepad controller was disconnected. */
    virtual void onGamepadDisconnected() override;

    /**
     * Called before a new frame is rendered.
     *
     * @remark Called before nodes that should be called every frame.
     *
     * @param timeSincePrevCallInSec Time in seconds that has passed since the last call
     * to this function.
     */
    virtual void onBeforeNewFrame(float timeSincePrevCallInSec) override;

private:
    /** Registers action and axis input events in the input manager. */
    void registerEditorInputEvents();

    /**
     * Returns editor camera node.
     *
     * @return Editor camera node.
     */
    EditorCameraNode* getEditorCameraNode();

    /** Text node used to print statistics in the editor. */
    TextNode* pStatsTextNode = nullptr;
};
