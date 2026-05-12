#include "Input.h"

#include <cstring>

#include "Application.h"
#include "InputBackend.h"

void Input::Update()
{
	m_currIndex ^= 1;
	Application::Instance().GetBackend()->input->GetKeyboardState(m_keyboardState[m_currIndex]);

	previousMouseState = currentMouseState;
	currentMouseState = Application::Instance().GetBackend()->input->GetMouseState();

	for (int i = 0; i < 4; i++)
	{
		m_gamepadState[m_currIndex][i] = Application::Instance().GetBackend()->input->GetGamepadButtonState(i);
	}
}

void Input::Reset()
{
	memset((void*)m_keyboardState, 0, sizeof(m_keyboardState));

	previousMouseState = 0;
	currentMouseState = 0;

	for (int i = 0; i < 4; i++)
	{
		m_gamepadState[0][i] = 0;
		m_gamepadState[1][i] = 0;
	}
}

bool Input::IsKeyDown(short key)
{
	return m_keyboardState[m_currIndex][key] == 1;
}

bool Input::IsKeyPressed(short key)
{
	return m_keyboardState[m_currIndex][key] == 1 && m_keyboardState[m_currIndex ^ 1][key] == 0;
}

bool Input::IsKeyReleased(short key)
{
	return m_keyboardState[m_currIndex][key] == 0 && m_keyboardState[m_currIndex ^ 1][key] == 1;
}

bool Input::IsAnyKeyPressed()
{
	for (int i = 0; i < 256; i++)
	{
		if (IsKeyPressed(i))
			return true;
	}
	return false;
}

int Input::GetControlType(int player)
{
	return Application::Instance().GetAppData()->GetControlTypes()[player];
}

void Input::SetControlType(int player, int type)
{
	if (type >= 1 && type <= 4) //is gamepad
	{
		if (!Application::Instance().GetBackend()->input->IsGamepadConnected(type-1))
			type = 0; //reset back to keyboard
	}
	
	Application::Instance().GetAppData()->GetControlTypes()[player] = type;
}

void Input::SetControlKey(int player, short control, unsigned short key)
{
	Application::Instance().GetAppData()->GetControlKeys()[player][control] = key;
}

bool Input::IsControlsDown(int player, short control)
{
	int controlType = Application::Instance().GetAppData()->GetControlTypes()[player];
	if (controlType != 0)
	{
		if (!Application::Instance().GetBackend()->input->IsGamepadConnected(controlType-1))
			return false;

		for (int i = 0; i < 8; i++)
		{
			if ((control & (1 << i)) != 0 && (m_gamepadState[m_currIndex][player] & (1 << i)) == 0)
				return false;
		}
		return true;
	}

	for (int i = 0; i < 8; i++)
	{
		if ((control & (1 << i)) != 0 && !IsKeyDown(Application::Instance().GetAppData()->GetControlKeys()[player][i])) return false;
	}

	return true;
}

bool Input::IsControlsPressed(int player, short control)
{
	int controlType = Application::Instance().GetAppData()->GetControlTypes()[player];
	if (controlType != 0)
	{
		if (!Application::Instance().GetBackend()->input->IsGamepadConnected(controlType-1))
			return false;

		for (int i = 0; i < 8; i++)
		{
			if ((control & (1 << i)) == 0)
				continue;
			if ((m_gamepadState[m_currIndex][player] & (1 << i)) != 0 && (m_gamepadState[m_currIndex ^ 1][player] & (1 << i)) == 0)
				return true;
		}
		return false;
	}

	for (int i = 0; i < 8; i++)
	{
		if ((control & (1 << i)) != 0 && IsKeyPressed(Application::Instance().GetAppData()->GetControlKeys()[player][i])) return true;
	}

	return false;
}

int Input::GetMouseX()
{
	return Application::Instance().GetBackend()->input->GetMouseX();
}

int Input::GetMouseY()
{
	return Application::Instance().GetBackend()->input->GetMouseY();
}

int Input::GetMouseWheelMove()
{
	return Application::Instance().GetBackend()->input->GetMouseWheelMove();
}

bool Input::IsMouseButtonDown(int button)
{
	if (button == 1) button = 0;
	else if (button == 4) button = 1;
	return currentMouseState & (1 << button);
}

bool Input::IsMouseButtonPressed(int button, bool doubleClick)
{
	if (doubleClick) return false; // TODO: implement double click
	
	if (button == 1) button = 0;
	else if (button == 4) button = 1;
	return (currentMouseState & (1 << button)) && !(previousMouseState & (1 << button));
}