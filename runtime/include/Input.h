#pragma once

#include <cstdint>

class Input
{
public:
	Input() = default;
	~Input() = default;

	void Update();
	void Reset();

	bool IsKeyDown(short key);
	bool IsKeyPressed(short key);
	bool IsKeyReleased(short key);
	bool IsAnyKeyPressed();

	int GetControlType(int player);
	void SetControlType(int player, int type);

	void SetControlKey(int player, short control, unsigned short key);

	bool IsControlsDown(int player, short control);
	bool IsControlsPressed(int player, short control);

	int GetMouseX();
	int GetMouseY();
	int GetMouseWheelMove();
	bool IsMouseButtonDown(int button);
	bool IsMouseButtonPressed(int button, bool doubleClick = false);

private:
	uint8_t m_keyboardState[2][256];
	int m_currIndex = 0;

	uint32_t currentMouseState;
	uint32_t previousMouseState;

	uint8_t m_gamepadState[2][4];
};

