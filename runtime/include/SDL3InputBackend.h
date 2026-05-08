#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "InputBackend.h"

class SDL3Backend;

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

	int FusionToSDLKey(short key);
private:
	SDL3Backend* backend = nullptr;
}; 
#endif