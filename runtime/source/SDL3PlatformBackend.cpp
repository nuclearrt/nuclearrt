#ifdef NUCLEAR_BACKEND_SDL3

#include "SDL3PlatformBackend.h"

#include <string>
#include <iostream>
#include <SDL3/SDL.h>
#ifdef _DEBUG
#include "DebugUI.h"
#include "imgui.h"
#include <imgui_impl_sdl3.h>
#endif

void SDL3PlatformBackend::Initialize()
{
	if (!pakFile.Load(GetAssetsFileName())) {
		std::cerr << "PakFile::Load Error: Failed to load assets file" << std::endl;
		return;
	}
}

bool SDL3PlatformBackend::ShouldQuit()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
#ifdef _DEBUG
		// Process ImGui events
		if (DEBUG_UI.IsEnabled()) {
			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		
		// Toggle debug UI with F1 key
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1 && event.key.repeat == 0) {
			DEBUG_UI.ToggleEnabled();
		}
#endif
		if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) windowFocused = true;

		if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) windowFocused = false;

		if (event.type == SDL_EVENT_QUIT) {
			return true;
		}
	}
	return false;
}

unsigned int SDL3PlatformBackend::GetTicks()
{
	return SDL_GetTicks();
}

float SDL3PlatformBackend::GetTimeDelta()
{
	static Uint32 previousTicks = SDL_GetTicks();
	Uint32 currentTicks = SDL_GetTicks();
	float delta = (currentTicks - previousTicks) / 1000.0f;
	previousTicks = currentTicks;
	return delta;
}

void SDL3PlatformBackend::Delay(unsigned int ms)
{
	SDL_Delay(ms);
}

std::string SDL3PlatformBackend::GetPlatformName()
{
#if defined(PLATFORM_WINDOWS)
	return "Windows";
#elif defined(PLATFORM_MACOS)
	return "macOS";
#elif defined(PLATFORM_LINUX)
	return "Linux";
#else
	return "Unknown";
#endif
}

std::string SDL3PlatformBackend::GetAssetsFileName()
{
	const char* basePath = SDL_GetBasePath();
	return std::string(basePath) + "assets.pak";
}
#endif