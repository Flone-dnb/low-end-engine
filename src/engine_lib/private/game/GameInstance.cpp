#include "game/GameInstance.h"

GameInstance::GameInstance(Window* pWindow) : pWindow(pWindow) {}

Window* GameInstance::getWindow() const { return pWindow; }
