#include "game/DebugConsole.h"

#if defined(DEBUG)

// Standard.
#include <format>

// Custom.
#include "render/Renderer.h"
#include "render/DebugDrawer.h"
#include "misc/Error.h"
#include "misc/MemoryUsage.hpp"

// External.
#include "hwinfo/hwinfo.h"

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
    if (it != registeredCommands.end()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the name \"{}\" for a new command is already being used by some other command", sCommandName));
    }

    registeredCommands[sCommandName] = callback;
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

    DebugDrawer::get().drawScreenRect(consoleScreenPos, consoleScreenSize);

    if (sCurrentInput.empty()) {
        // Prepare stats text.
        std::string sStatsText = std::format(
            "FPS: {} (limit: {}) ",
            pRenderer->getRenderStatistics().getFramesPerSecond(),
            pRenderer->getFpsLimit());

        hwinfo::Memory memory;
        const auto iRamTotalMb = memory.total_Bytes() / 1024 / 1024;
        const auto iRamFreeMb = memory.available_Bytes() / 1024 / 1024;
        const auto iRamUsedMb = iRamTotalMb - iRamFreeMb;
        const auto ratio = static_cast<float>(iRamUsedMb) / static_cast<float>(iRamTotalMb);
        const auto iAppRamMb = getCurrentRSS() / 1024 / 1024;

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
        const auto commandIt = registeredCommands.find(sCurrentInput);
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

            DebugDrawer::get().drawText(
                sErrorMessage,
                5.0F,
                glm::vec3(1.0F),
                glm::vec2(consoleScreenPos.x + textPadding, consoleScreenPos.y - textHeight - textPadding),
                textHeight);
            return;
        }

        // Execute the command.
        commandIt->second(pGameInstance);
        hide();

        return;
    }
}

#endif
