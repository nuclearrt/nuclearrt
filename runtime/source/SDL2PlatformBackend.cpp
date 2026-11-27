#ifdef NUCLEAR_BACKEND_SDL2

#include "SDL2PlatformBackend.h"

#include <iostream>

#include "Application.h"
#include "FontBank.h"
#include "SDL2Backend.h"

#include <SDL.h>

#ifdef NUCLEAR_DEBUG_UI
#include "DebugUI.h"
#include <imgui_impl_sdl2.h>
#endif

#ifdef PLATFORM_SWITCH
#include <switch.h>
#endif

void SDL2PlatformBackend::Initialize() {
	#ifdef __SWITCH__
		romfsInit();
	#endif
	
	if (!pakFile.Load(GetAssetsDirectory())) {
		Log("PakFile::Load Error: Failed to load assets file");
		return;
	}
}

bool SDL2PlatformBackend::ShouldQuit()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
#ifdef NUCLEAR_DEBUG_UI
		// Process ImGui events
		if (DEBUG_UI.IsEnabled()) {
			ImGui_ImplSDL2_ProcessEvent(&event);
		}
		
		// Toggle debug UI with F1 key
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1 && event.key.repeat == 0) {
			DEBUG_UI.ToggleEnabled();
		}
#endif

#ifdef PLATFORM_SWITCH
		// handle touch events
		if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION) {
			int windowWidth, windowHeight;
			SDL_GetWindowSize(backend->GetGraphics()->GetSDLWindow(), &windowWidth, &windowHeight);
			backend->GetInput()->touchX = static_cast<int>(event.tfinger.x * windowWidth);
			backend->GetInput()->touchY = static_cast<int>(event.tfinger.y * windowHeight);
			backend->GetInput()->touchDown = true;
		}
		else if (event.type == SDL_FINGERUP) {
			backend->GetInput()->touchDown = false;
		}
#endif
		//game controller connected or disconnected
		if (event.type == SDL_CONTROLLERDEVICEADDED) {
			backend->GetInput()->gamepads.push_back(SDL_GameControllerOpen(event.jdevice.which));
		}
		else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
			SDL_GameControllerClose(backend->GetInput()->gamepads.back());
			backend->GetInput()->gamepads.pop_back();
		}

		if (event.type == SDL_WINDOWEVENT) {
			if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				windowFocused = true;
			}
			else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				windowFocused = false;
			}
		}

		if (event.type == SDL_QUIT) {
			return true;
		}
	}
	return false;
}

std::string SDL2PlatformBackend::GetPlatformName()
{
#if defined(PLATFORM_WINDOWS)
	return "Windows";
#elif defined(PLATFORM_MACOS)
	return "macOS";
#elif defined(PLATFORM_LINUX)
	return "Linux";
#elif defined(PLATFORM_WEB)
	return "Web";
#elif defined(PLATFORM_IOS)
	return "iOS";
#elif defined(PLATFORM_SWITCH)
	return "Switch";
#else
	return "Unknown";
#endif
}

std::string SDL2PlatformBackend::GetAssetsDirectory()
{
#if defined(PLATFORM_SWITCH)
	return "romfs:/";
#else
	return std::string(SDL_GetBasePath());
#endif
}

std::string SDL2PlatformBackend::GetAppDrive()
{
#if defined(PLATFORM_WINDOWS)
	std::string basePath = SDL_GetBasePath();
	if (basePath.length() >= 2 && basePath[1] == ':') {
		return basePath.substr(0, 2);
	}
	return "";
#elif defined(PLATFORM_WEB)
	return "/disk";
#else
	return "/";
#endif
}

std::string SDL2PlatformBackend::GetAppDirectory()
{
#if defined(PLATFORM_WEB)
	return "/";
#else
	std::string basePath = SDL_GetBasePath();
	size_t pos = basePath.find('/');
	if (pos != std::string::npos) {
		return basePath.substr(pos);
	}
	else {
		pos = basePath.find('\\');
		if (pos != std::string::npos) {
			return basePath.substr(pos);
		}
	}
	return basePath;
#endif
}

std::string SDL2PlatformBackend::GetAppPath()
{
#if defined(PLATFORM_WEB)
	return "/disk/";
#else
	return SDL_GetBasePath();
#endif
}


unsigned int SDL2PlatformBackend::GetTicks()
{
	return SDL_GetTicks();
}

float SDL2PlatformBackend::GetTimeDelta()
{
    static Uint32 previousTicks = SDL_GetTicks();
    Uint32 currentTicks = SDL_GetTicks();
    float delta = (currentTicks - previousTicks) / 1000.0f;
    previousTicks = currentTicks;
    return delta;
}

void SDL2PlatformBackend::Delay(unsigned int ms)
{
    SDL_Delay(ms);
}

void SDL2PlatformBackend::SetFullscreen(bool fullscreen)
{
	SDL_SetWindowFullscreen(backend->GetGraphics()->GetSDLWindow(), fullscreen);
}

void SDL2PlatformBackend::Log(std::string text) {
	#ifdef __EMSCRIPTEN__
	emscripten_console_log(text.c_str());
	#else
	SDL_Log(text.c_str());
	#endif
}
#endif