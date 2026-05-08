#ifdef NUCLEAR_BACKEND_SDL3

#include "SDL3InputBackend.h"

#include "SDL3Backend.h"
#include "Application.h"
#include "Frame.h"

#include <algorithm>
#include <SDL3/SDL.h>

#ifdef _DEBUG
#include "DebugUI.h"
#include "imgui.h"
#endif

void SDL3InputBackend::GetKeyboardState(uint8_t* outBuffer)
{
#ifdef _DEBUG
	if (DEBUG_UI.IsEnabled() && ImGui::GetIO().WantCaptureKeyboard) return;
#endif

	//return the keyboard state in a new array which matches the Fusion key codes
	const bool* keyboardState = SDL_GetKeyboardState(nullptr);
	for (int i = 0; i < 256; i++)
	{
		outBuffer[i] = keyboardState[FusionToSDLKey(i)] ? 1 : 0;
	}
}

int SDL3InputBackend::GetMouseX()
{
	if (!backend->GetGraphics()) return 0;
	SDL_Window* window = backend->GetGraphics()->GetSDLWindow();
	if (!window) return 0;
	float x;
#ifndef PLATFORM_WEB
	int windowX;
	SDL_GetWindowPosition(window, &windowX, NULL);
	SDL_GetGlobalMouseState(&x, NULL);
	float mouseX = x - windowX;
#else
	float mouseX;
	SDL_GetMouseState(&mouseX, NULL);
#endif
	
	//get mouse position relative to render target
	SDL_FRect rect = backend->GetGraphics()->CalculateRenderTargetRect();
	int windowWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	float relativeX = (mouseX - rect.x) * (windowWidth / rect.w);
	return static_cast<int>(relativeX);
}

int SDL3InputBackend::GetMouseY()
{
	if (!backend->GetGraphics()) return 0;
	SDL_Window* window = backend->GetGraphics()->GetSDLWindow();
	if (!window) return 0;
	float y;
#ifndef PLATFORM_WEB
	int windowY;
	SDL_GetWindowPosition(window, NULL, &windowY);
	SDL_GetGlobalMouseState(NULL, &y);
	float mouseY = y - windowY;
#else
	float mouseY;
	SDL_GetMouseState(NULL, &mouseY);
#endif

	//get mouse position relative to render target
	SDL_FRect rect = backend->GetGraphics()->CalculateRenderTargetRect();
	int windowHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);
	float relativeY = (mouseY - rect.y) * (windowHeight / rect.h);
	return static_cast<int>(relativeY);
}

void SDL3InputBackend::SetMouseX(int x)
{
	if (!backend->GetGraphics()) return;
	SDL_Window* window = backend->GetGraphics()->GetSDLWindow();
	if (!window) return;
	SDL_FRect rect = backend->GetGraphics()->CalculateRenderTargetRect();
	int renderTargetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	
	float windowX = rect.x + (x * rect.w / renderTargetWidth);
	float windowY;
	
	SDL_GetMouseState(NULL, &windowY);
	SDL_WarpMouseInWindow(window, windowX, windowY);
}

void SDL3InputBackend::SetMouseY(int y)
{
	if (!backend->GetGraphics()) return;
	SDL_Window* window = backend->GetGraphics()->GetSDLWindow();
	if (!window) return;
	SDL_FRect rect = backend->GetGraphics()->CalculateRenderTargetRect();
	int renderTargetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);
	
	float windowX;
	float windowY = rect.y + (y * rect.h / renderTargetHeight);
	
	SDL_GetMouseState(&windowX, NULL);
	SDL_WarpMouseInWindow(window, windowX, windowY);
}

int SDL3InputBackend::GetMouseWheelMove()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_MOUSE_WHEEL) {
			return event.wheel.y;
		}
	}
	return 0;
}

uint32_t SDL3InputBackend::GetMouseState()
{
#ifdef _DEBUG
	if (DEBUG_UI.IsEnabled() && ImGui::GetIO().WantCaptureMouse) return 0;
#endif

	return SDL_GetMouseState(nullptr, nullptr);
}

void SDL3InputBackend::HideMouseCursor()
{
	SDL_HideCursor();
}

void SDL3InputBackend::ShowMouseCursor()
{
	SDL_ShowCursor();
}

int SDL3InputBackend::FusionToSDLKey(short key)
{
	switch (key)
	{
		default:
			return SDL_SCANCODE_UNKNOWN;
		case 0x08:
			return SDL_SCANCODE_BACKSPACE;
		case 0x09:
			return SDL_SCANCODE_TAB;
		case 0x0D:
			return SDL_SCANCODE_RETURN;
		case 0x10:
			return SDL_SCANCODE_LSHIFT;
		case 0x11:
			return SDL_SCANCODE_LCTRL;
		case 0x13:
			return SDL_SCANCODE_PAUSE;
		case 0x14:
			return SDL_SCANCODE_CAPSLOCK;
		case 0x1B:
			return SDL_SCANCODE_ESCAPE;
		case 0x20:
			return SDL_SCANCODE_SPACE;
		case 0x21:
			return SDL_SCANCODE_PAGEUP;
		case 0x22:
			return SDL_SCANCODE_PAGEDOWN;
		case 0x23:
			return SDL_SCANCODE_END;
		case 0x24:
			return SDL_SCANCODE_HOME;
		case 0x25:
			return SDL_SCANCODE_LEFT;
		case 0x26:
			return SDL_SCANCODE_UP;
		case 0x27:
			return SDL_SCANCODE_RIGHT;
		case 0x28:
			return SDL_SCANCODE_DOWN;
		case 0x2D:
			return SDL_SCANCODE_INSERT;
		case 0x2E:
			return SDL_SCANCODE_DELETE;
		case 0x30:
			return SDL_SCANCODE_0;
		case 0x31:
			return SDL_SCANCODE_1;
		case 0x32:
			return SDL_SCANCODE_2;
		case 0x33:
			return SDL_SCANCODE_3;
		case 0x34:
			return SDL_SCANCODE_4;
		case 0x35:
			return SDL_SCANCODE_5;
		case 0x36:
			return SDL_SCANCODE_6;
		case 0x37:
			return SDL_SCANCODE_7;
		case 0x38:
			return SDL_SCANCODE_8;
		case 0x39:
			return SDL_SCANCODE_9;
		case 0x41:
			return SDL_SCANCODE_A;
		case 0x42:
			return SDL_SCANCODE_B;
		case 0x43:
			return SDL_SCANCODE_C;
		case 0x44:
			return SDL_SCANCODE_D;
		case 0x45:
			return SDL_SCANCODE_E;
		case 0x46:
			return SDL_SCANCODE_F;
		case 0x47:
			return SDL_SCANCODE_G;
		case 0x48:
			return SDL_SCANCODE_H;
		case 0x49:
			return SDL_SCANCODE_I;
		case 0x4A:
			return SDL_SCANCODE_J;
		case 0x4B:
			return SDL_SCANCODE_K;
		case 0x4C:
			return SDL_SCANCODE_L;
		case 0x4D:
			return SDL_SCANCODE_M;
		case 0x4E:
			return SDL_SCANCODE_N;
		case 0x4F:
			return SDL_SCANCODE_O;
		case 0x50:
			return SDL_SCANCODE_P;
		case 0x51:
			return SDL_SCANCODE_Q;
		case 0x52:
			return SDL_SCANCODE_R;
		case 0x53:
			return SDL_SCANCODE_S;
		case 0x54:
			return SDL_SCANCODE_T;
		case 0x55:
			return SDL_SCANCODE_U;
		case 0x56:
			return SDL_SCANCODE_V;
		case 0x57:
			return SDL_SCANCODE_W;
		case 0x58:
			return SDL_SCANCODE_X;
		case 0x59:
			return SDL_SCANCODE_Y;
		case 0x5A:
			return SDL_SCANCODE_Z;
		case 0x60:
			return SDL_SCANCODE_KP_0;
		case 0x61:
			return SDL_SCANCODE_KP_1;
		case 0x62:
			return SDL_SCANCODE_KP_2;
		case 0x63:
			return SDL_SCANCODE_KP_3;
		case 0x64:
			return SDL_SCANCODE_KP_4;
		case 0x65:
			return SDL_SCANCODE_KP_5;
		case 0x66:
			return SDL_SCANCODE_KP_6;
		case 0x67:
			return SDL_SCANCODE_KP_7;
		case 0x68:
			return SDL_SCANCODE_KP_8;
		case 0x69:
			return SDL_SCANCODE_KP_9;
		case 0x6A:
			return SDL_SCANCODE_KP_MULTIPLY;
		case 0x6B:
			return SDL_SCANCODE_KP_PLUS;
		case 0x6D:
			return SDL_SCANCODE_KP_MINUS;
		case 0x6E:
			return SDL_SCANCODE_KP_PERIOD;
		case 0x6F:
			return SDL_SCANCODE_KP_DIVIDE;
		case 0x70:
			return SDL_SCANCODE_F1;
		case 0x71:
			return SDL_SCANCODE_F2;
		case 0x72:
			return SDL_SCANCODE_F3;
		case 0x73:
			return SDL_SCANCODE_F4;
		case 0x74:
			return SDL_SCANCODE_F5;
		case 0x75:
			return SDL_SCANCODE_F6;
		case 0x76:
			return SDL_SCANCODE_F7;
		case 0x77:
			return SDL_SCANCODE_F8;
		case 0x78:
			return SDL_SCANCODE_F9;
		case 0x79:
			return SDL_SCANCODE_F10;
		case 0x7A:
			return SDL_SCANCODE_F11;
		case 0x7B:
			return SDL_SCANCODE_F12;
		case 0x90:
			return SDL_SCANCODE_NUMLOCKCLEAR;
		case 0xBA:
			return SDL_SCANCODE_SEMICOLON;
		case 0xBB:
			return SDL_SCANCODE_EQUALS;
		case 0xBC:
			return SDL_SCANCODE_COMMA;
		case 0xBD:
			return SDL_SCANCODE_MINUS;
		case 0xBE:
			return SDL_SCANCODE_PERIOD;
		case 0xBF:
			return SDL_SCANCODE_SLASH;
		case 0xC0:
			return SDL_SCANCODE_GRAVE;
		case 0xDB:
			return SDL_SCANCODE_LEFTBRACKET;
		case 0xDC:
			return SDL_SCANCODE_BACKSLASH;
		case 0xDD:
			return SDL_SCANCODE_RIGHTBRACKET;
		case 0xDE:
			return SDL_SCANCODE_APOSTROPHE;
	}
}
#endif