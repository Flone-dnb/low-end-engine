#include "EditorGameInstance.h"

// Custom.
#include "input/InputManager.h"
#include "game/Window.h"
#include "input/EditorInputEventIds.hpp"

const char* EditorGameInstance::getEditorWindowTitle() { return "Low End Editor"; }

EditorGameInstance::EditorGameInstance(Window* pWindow) : GameInstance(pWindow) {}

void EditorGameInstance::onGameStarted() {
    registerEditorInputEvents();

    // Create world.
    createWorld([]() {
        // TODO
    });
}

void EditorGameInstance::registerEditorInputEvents() {
    // Prepare a handy lambda.
    const auto showErrorIfNotEmpty = [](std::optional<Error>& optionalError) {
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            optionalError->showError();
            throw std::runtime_error(optionalError->getFullErrorMessage());
        }
    };

    // Register action events.
    {
        auto optionalError = getInputManager()->addActionEvent(
            static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE),
            {KeyboardButton::ESCAPE, GamepadButton::BACK});
        showErrorIfNotEmpty(optionalError);
    }

    // Bind to action events.
    {
        const auto pActionEvents = getActionEventBindings();
        std::scoped_lock guard(pActionEvents->first);

        pActionEvents->second[static_cast<unsigned int>(EditorInputEventIds::Action::CLOSE)] =
            [this](KeyboardModifiers modifiers, bool bIsPressedDown) { getWindow()->close(); };
    }
}
