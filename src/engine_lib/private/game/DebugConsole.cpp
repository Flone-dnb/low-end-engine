#include "game/DebugConsole.h"

#if defined(DEBUG)

// Standard.
#include <format>

// Custom.
#include "render/Renderer.h"
#include "game/Window.h"
#include "game/physics/PhysicsManager.h"
#include "render/DebugDrawer.h"
#include "misc/Error.h"
#include "misc/MemoryUsage.hpp"

namespace {
    constexpr glm::vec2 consoleScreenPos = glm::vec2(0.0F, 0.96F);
    constexpr glm::vec2 consoleScreenSize = glm::vec2(1.0F, 1.0F - consoleScreenPos.y);
    constexpr float textPadding = consoleScreenSize.y * 0.1F;
    constexpr float textHeight = 0.025F;
}

DebugConsole& DebugConsole::get() {
    static DebugConsole debugConsole;
    return debugConsole;
}

void DebugConsole::registerCommand(
    const std::string& sCommandName, const std::function<void(GameInstance*)>& callback) {
    if (sCommandName.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("empty commands are not allowed");
    }

    const auto it = registeredCommands.find(sCommandName);
    if (it != registeredCommands.end()) {
        // Probably already registered since DebugConsole is a singleton.
        // This often happens in automated tests.
        return;
    }

    registeredCommands[sCommandName] = RegisteredCommand{.noArgs = callback};
}

void DebugConsole::registerCommand(
    const std::string& sCommandName, const std::function<void(GameInstance*, int)>& callback) {
    if (sCommandName.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("empty commands are not allowed");
    }

    const auto it = registeredCommands.find(sCommandName);
    if (it != registeredCommands.end()) {
        // Probably already registered since DebugConsole is a singleton.
        // This often happens in automated tests.
        return;
    }

    registeredCommands[sCommandName] = RegisteredCommand{.intArg = callback};
}

void DebugConsole::show() { bIsShown = true; }

void DebugConsole::hide() {
    bIsShown = false;
    sCurrentInput = "";
}

void DebugConsole::onBeforeNewFrame(Renderer* pRenderer) {
    if (!bIsShown) {
        return;
    }

    // Draw background.
    DebugDrawer::get().drawScreenRect(consoleScreenPos, consoleScreenSize, glm::vec3(0.25F), 0.0F);

    if (sCurrentInput.empty()) {
        // Prepare stats text.
        std::string sStatsText = std::format(
            "FPS: {} (limit: {}) ",
            pRenderer->getRenderStatistics().getFramesPerSecond(),
            pRenderer->getFpsLimit());

        const auto iRamTotalMb = MemoryUsage::getTotalMemorySize() / 1024 / 1024;
        const auto iRamUsedMb = MemoryUsage::getTotalMemorySizeUsed() / 1024 / 1024;
        const auto iAppRamMb = MemoryUsage::getMemorySizeUsedByProcess() / 1024 / 1024;

        sStatsText += std::format("RAM used (MB): {} ({}/{})", iAppRamMb, iRamUsedMb, iRamTotalMb);
#if defined(ENGINE_ASAN_ENABLED)
        sStatsText += " (big RAM usage due to ASan)";
#endif

        DebugDrawer::get().drawText(
            "type a command...               | " + sStatsText,
            0.0F,
            glm::vec3(0.5F),
            consoleScreenPos + textPadding,
            textHeight);
    } else {
        DebugDrawer::get().drawText(
            sCurrentInput, 0.0F, glm::vec3(1.0F), consoleScreenPos + textPadding, textHeight);
    }
}

void DebugConsole::onKeyboardInputTextCharacter(const std::string& sTextCharacter) {
    if (sTextCharacter == "`") {
        return;
    }
    sCurrentInput += sTextCharacter;
}

void DebugConsole::onKeyboardInput(
    KeyboardButton key, KeyboardModifiers modifiers, GameInstance* pGameInstance) {
    if (key == KeyboardButton::BACKSPACE && !sCurrentInput.empty()) {
        sCurrentInput.pop_back();
        return;
    }

    if (key == KeyboardButton::ENTER && !sCurrentInput.empty()) {
        // Split input into command and arguments.
        std::vector<std::string> vCommand;
        std::string sCurrentPart;
        for (const auto& ch : sCurrentInput) {
            if (ch == ' ') {
                if (!sCurrentPart.empty()) {
                    vCommand.push_back(sCurrentPart);
                    sCurrentPart = "";
                }
                continue;
            }
            sCurrentPart += ch;
        }
        if (!sCurrentPart.empty()) {
            vCommand.push_back(sCurrentPart);
        }
        if (vCommand.empty()) {
            vCommand.push_back("");
        }

        const auto commandIt = registeredCommands.find(vCommand[0]);
        if (commandIt == registeredCommands.end()) {
            sCurrentInput = "";

            // Prepare a message to display.
            std::string sErrorMessage = "unknown command";
            if (registeredCommands.empty()) {
                sErrorMessage += ", no commands registered";
            } else {
                sErrorMessage +=
                    std::format(", available {} command(s) such as: ", registeredCommands.size());
                constexpr size_t iCommandCountToDisplay = 5;
                size_t iAddedCommandCount = 0;
                for (const auto& [sCommandName, callback] : registeredCommands) {
                    sErrorMessage += "\"" + sCommandName + "\", ";
                    iAddedCommandCount += 1;

                    if (iAddedCommandCount == iCommandCountToDisplay) {
                        break;
                    }
                }
                sErrorMessage.pop_back();
                sErrorMessage.pop_back();
            }

            displayMessage(sErrorMessage);
            return;
        }

        // Execute the command.
        if (commandIt->second.noArgs != nullptr) {
            commandIt->second.noArgs(pGameInstance);
            hide();
        } else if (vCommand.size() == 2 && commandIt->second.intArg != nullptr) {
            int iArg1 = 0;
            try {
                iArg1 = std::stoi(vCommand[1]);
            } catch (std::exception&) {
                displayMessage("unable to convert 1st argument to int");
                return;
            }
            commandIt->second.intArg(pGameInstance, iArg1);
            hide();
        } else {
            displayMessage("incorrect number of arguments specified");
        }
    }
}

void DebugConsole::displayMessage(const std::string& sText) {
    constexpr float messageTimeSec = 2.5F;

    DebugDrawer::get().drawScreenRect( // <- message background
        glm::vec2(consoleScreenPos.x, consoleScreenPos.y - consoleScreenSize.y),
        glm::vec2(consoleScreenSize.x, consoleScreenSize.y),
        glm::vec3(0.25F),
        messageTimeSec);

    DebugDrawer::get().drawText( // <- message
        sText,
        messageTimeSec,
        glm::vec3(1.0F),
        glm::vec2(consoleScreenPos.x + textPadding, consoleScreenPos.y - consoleScreenSize.y + textPadding),
        textHeight);
}

#endif
