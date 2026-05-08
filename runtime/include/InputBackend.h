#pragma once

#include <cstdint>

class InputBackend {
public:
	virtual void Initialize() {}
	virtual void Deinitialize() {}

	virtual void GetKeyboardState(uint8_t* outBuffer) {}
	virtual int GetMouseX() { return 0; }
	virtual int GetMouseY() { return 0; }
	virtual void SetMouseX(int x) {}
	virtual void SetMouseY(int y) {}
	virtual int GetMouseWheelMove() { return 0; }
	virtual uint32_t GetMouseState() { return 0; }
	virtual void HideMouseCursor() {}
	virtual void ShowMouseCursor() {}
}; 