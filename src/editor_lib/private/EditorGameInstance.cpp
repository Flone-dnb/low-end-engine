#include "EditorGameInstance.h"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
    // Create world.
    createWorld([](const std::optional<Error>& optionalWorldError) {
        if (optionalWorldError.has_value()) [[unlikely]] {
            auto error = optionalWorldError.value();
            error.addCurrentLocationToErrorStack();
            error.showError();
            throw std::runtime_error(error.getFullErrorMessage());
        }
    });
}
