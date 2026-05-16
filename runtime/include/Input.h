#pragma once

#include <cstdint>
#include <vector>

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

	void RestoreControl(int player);
	void IgnoreControl(int player);

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

	std::vector<bool> m_playerEnabled;
};

