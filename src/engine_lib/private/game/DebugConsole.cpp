#include "game/DebugConsole.h"

#if defined(ENGINE_DEBUG_TOOLS)

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

    constexpr glm::vec2 statsScreenPos = glm::vec2(0.0F, 0.5F);

    constexpr float textPadding = consoleScreenSize.y * 0.1F;
    constexpr float textHeight = 0.025F;
}

void DebugConsole::registerStatsCommand() {
    registerCommand("showStats", [this](GameInstance*) { bShowStats = true; });
    registerCommand("hideStats", [this](GameInstance*) { bShowStats = false; });
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

    auto& registeredCommands = get().registeredCommands;

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

    auto& registeredCommands = get().registeredCommands;

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
    if (bIsShown) {
        // Draw background.
        DebugDrawer::drawScreenRect(consoleScreenPos, consoleScreenSize, glm::vec3(0.25F), 0.0F);

        if (sCurrentInput.empty()) {
            DebugDrawer::drawText(
                "type a command...", 0.0F, glm::vec3(0.5F), consoleScreenPos + textPadding, textHeight);
        } else {
            DebugDrawer::drawText(
                sCurrentInput, 0.0F, glm::vec3(1.0F), consoleScreenPos + textPadding, textHeight);
        }
    }

    if (bShowStats) {
        glm::vec2 currentPos = statsScreenPos;

        const auto drawText = [&](const std::string& sText) {
            DebugDrawer::drawText(
                sText,
                0.0F,
                glm::vec3(1.0F),
                glm::vec2(currentPos.x + textPadding, currentPos.y),
                textHeight);
            currentPos.y += textHeight + textPadding;
        };

        const auto iRamTotalMb = MemoryUsage::getTotalMemorySize() / 1024 / 1024;
        const auto iRamUsedMb = MemoryUsage::getTotalMemorySizeUsed() / 1024 / 1024;
        const auto iAppRamMb = MemoryUsage::getMemorySizeUsedByProcess() / 1024 / 1024;

        std::string sRamStats = std::format("RAM used (MB): {} ({}/{})", iAppRamMb, iRamUsedMb, iRamTotalMb);
#if defined(ENGINE_ASAN_ENABLED)
        sRamStats += " (big RAM usage due to ASan)";
#endif

        drawText(std::format(
            "FPS: {} (limit: {})",
            pRenderer->getRenderStatistics().getFramesPerSecond(),
            pRenderer->getFpsLimit()));
        drawText(sRamStats);
        drawText(std::format("active moving bodies: {}", stats.iActiveMovingBodyCount));
        drawText(std::format("active simulated bodies: {}", stats.iActiveSimulatedBodyCount));
        drawText(std::format("active character bodies: {}", stats.iActiveCharacterBodyCount));
        drawText(std::format("rendered light sources: {}", stats.iRenderedLightSourceCount));
        drawText(std::format("rendered opaque meshes: {}", stats.iRenderedOpaqueMeshCount));
        drawText(std::format("rendered transparent meshes: {}", stats.iRenderedTransparentMeshCount));
        drawText(std::format("CPU time (ms) for game tick: {:.1F}", stats.cpuTickTimeMs));
        drawText(std::format("CPU time (ms) to submit a frame: {:.1F}", stats.cpuTimeToSubmitFrameMs));
        if (stats.gpuTimeDrawMeshesMs < 0.0F) {
            drawText("GPU time metrics are not supported on this GPU");
        } else {
            drawText(std::format("GPU time (ms) draw meshes: {:.1F}", stats.gpuTimeDrawMeshesMs));
            drawText(std::format("GPU time (ms) post processing: {:.1F}", stats.gpuTimePostProcessingMs));
            drawText(std::format("GPU time (ms) draw ui: {:.1F}", stats.gpuTimeDrawUiMs));
        }
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

    DebugDrawer::drawScreenRect( // <- message background
        glm::vec2(consoleScreenPos.x, consoleScreenPos.y - consoleScreenSize.y),
        glm::vec2(consoleScreenSize.x, consoleScreenSize.y),
        glm::vec3(0.25F),
        messageTimeSec);

    DebugDrawer::drawText( // <- message
        sText,
        messageTimeSec,
        glm::vec3(1.0F),
        glm::vec2(consoleScreenPos.x + textPadding, consoleScreenPos.y - consoleScreenSize.y + textPadding),
        textHeight);
}

#endif
