#pragma once

#if defined(ENGINE_DEBUG_TOOLS)

// Standard.
#include <unordered_map>
#include <string>
#include <functional>

// Custom.
#include "input/KeyboardButton.hpp"

class GameInstance;
class Renderer;

/**
 * Can be displayed in game using the tilde (~) button.
 * Allows to create and run custom commands in the game (like cheat commands for developers).
 */
class DebugConsole {
    // Transfers input when the console is opened.
    friend class GameManager;

public:
    /** Various counters and statistics. */
    struct Stats {
        /** Total number of currently active simulated bodies. */
        size_t iActiveSimulatedBodyCount = 0;

        /** Total number of currently active moving bodies. */
        size_t iActiveMovingBodyCount = 0;

        /** Total number of currently active simulated character bodies. */
        size_t iActiveCharacterBodyCount = 0;

        /** Total number of light sources considered in the renderer. */
        size_t iRenderedLightSourceCount = 0;

        /** Total number of meshes rendered last frame. */
        size_t iRenderedMeshCount = 0;

        /** Time in milliseconds that the CPU spent doing the last tick. */
        float cpuTickTimeMs = 0.0F;

        /** Time in milliseconds that the CPU spent submitting the last frame. */
        float cpuTimeToSubmitFrameMs = 0.0F;

        /** Time in milliseconds (part of the @ref cpuTimeToSubmitFrameMs) to submit meshes. */
        float cpuTimeToSubmitMeshesMs = 0.0F;

        /** Time in milliseconds (part of the @ref cpuTimeToSubmitFrameMs) to submit UI. */
        float cpuTimeToSubmitUiMs = 0.0F;

        /** Time in milliseconds (part of the @ref cpuTimeToSubmitFrameMs) to submit debug drawer things. */
        float cpuTimeToSubmitDebugDrawMs = 0.0F;

        /** Time in milliseconds spent on the GPU drawing meshes. */
        float gpuTimeDrawMeshesMs = -1.0F;

        /** Time in milliseconds spent on the GPU drawing skybox. */
        float gpuTimeDrawSkyboxMs = -1.0F;

        /** Time in milliseconds spent on the GPU drawing UI. */
        float gpuTimeDrawUiMs = -1.0F;

        /** Time in milliseconds spent on the GPU drawing debug objects. */
        float gpuTimeDrawDebug = -1.0F;
    };

    DebugConsole(const DebugConsole&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;
    ~DebugConsole() = default;

    /**
     * Registers a new command with the specified name and a callback that will be triggered once
     * the command is called.
     *
     * @remark If a command with the specified name is already registered an error will be shown.
     *
     * @param sCommandName Name of the new command.
     * @param callback     Callback to trigger on command.
     */
    static void
    registerCommand(const std::string& sCommandName, const std::function<void(GameInstance*)>& callback);

    /**
     * Registers a new command with the specified name that accepts a single argument.
     *
     * @remark If a command with the specified name is already registered an error will be shown.
     *
     * @param sCommandName Name of the new command.
     * @param callback     Callback to trigger on command.
     */
    static void
    registerCommand(const std::string& sCommandName, const std::function<void(GameInstance*, int)>& callback);

    /** Shows debug statistics (this function can be called using a debug command). */
    static void toggleStats();

    /**
     * Returns debug statistics for various systems to update.
     *
     * @return Stats.
     */
    static Stats& getStats() { return DebugConsole::get().stats; }

private:
    /** Groups info about a registered command. */
    struct RegisteredCommand {
        /** If not empty means the command does not accept any arguments. */
        std::function<void(GameInstance*)> noArgs{};

        /** If not empty means the command requires 1 argument. */
        std::function<void(GameInstance*, int)> intArg{};
    };

    DebugConsole() = default;

    /** Registers a command to display @ref stats. */
    void registerStatsCommand();

    /**
     * Returns a reference to the debug console instance.
     * If no instance was created yet, this function will create it
     * and return a reference to it.
     *
     * @return Reference to the instance.
     */
    static DebugConsole& get();

    /**
     * Displays a message on top of the console.
     *
     * @param sText Text to display.
     */
    void displayMessage(const std::string& sText);

    /**
     * Called every frame by game manager regardless if the state (shown/hidden).
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     * @param pRenderer Renderer.
     */
    void onBeforeNewFrame(float timeSincePrevFrameInSec, Renderer* pRenderer);

    /** Shows the console. */
    void show();

    /** Hides the console. */
    void hide();

    /**
     * Called by game manager when the console is shown and the user input is received.
     *
     * @param key            Keyboard key.
     * @param modifiers      Keyboard modifier keys.
     * @param pGameInstance  Game instance.
     */
    void onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, GameInstance* pGameInstance);

    /**
     * Called by game manager when the console is shown and receives user input.
     *
     * @param sTextCharacter Character that was typed.
     */
    void onKeyboardInputTextCharacter(const std::string& sTextCharacter);

    /**
     * Returns the current state of the console.
     *
     * @return Visibility.
     */
    bool isShown() const { return bIsShown; }

    /** Pairs of "command name" - "callback to trigger on command". */
    std::unordered_map<std::string, RegisteredCommand> registeredCommands;

    /** Debug statistics. */
    Stats stats;

    /** Input by the user. */
    std::string sCurrentInput;

    /** Time in seconds since stats were updated the last time. */
    float timeSinceLastStatsUpdateSec = 0.0F;

    /** Current state. */
    bool bIsShown = false;

    /** `true` if @ref stats needs to be displayed. */
    bool bShowStats = false;
};

#endif
