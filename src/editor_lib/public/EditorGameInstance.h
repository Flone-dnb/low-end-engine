#pragma once

// Custom.
#include "game/GameInstance.h"

class Window;
class GameManager;
class EditorCameraNode;

/** Editor's game instance. */
class EditorGameInstance : public GameInstance {
public:
    /**
     * Creates a new game instance.
     *
     * @param pWindow Valid pointer to the game's window.
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
};
