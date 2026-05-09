#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "InputBackend.h"

#include <vector>

class SDL3Backend;
struct SDL_Gamepad;

class SDL3InputBackend : public InputBackend {
public:
	void SetBackend(SDL3Backend* b) { backend = b; }

	void GetKeyboardState(uint8_t* outBuffer) override;
	int GetMouseX() override;
	int GetMouseY() override;
	void SetMouseX(int x) override;
	void SetMouseY(int y) override;
	int GetMouseWheelMove() override;
	uint32_t GetMouseState() override;
	void HideMouseCursor() override;
	void ShowMouseCursor() override;

	bool IsGamepadConnected(int index) override;
	uint8_t GetGamepadButtonState(int index) override;

	int FusionToSDLKey(short key);

	std::vector<SDL_Gamepad*> gamepads = {};
private:
	SDL3Backend* backend = nullptr;
}; 
#endif