#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "Backend.h"

#include "SDL3PlatformBackend.h"
#include "SDL3GraphicsBackend.h"
#include "SDL3AudioBackend.h"
#include "SDL3InputBackend.h"

class SDL3Backend : public Backend {
public:
	SDL3Backend() {
		platform = new SDL3PlatformBackend();
		graphics = new SDL3GraphicsBackend();
		audio = new SDL3AudioBackend();
		input = new SDL3InputBackend();
		
		GetPlatform()->SetBackend(this);
		GetGraphics()->SetBackend(this);
		GetAudio()->SetBackend(this);
		GetInput()->SetBackend(this);

		platform->Initialize();
		graphics->Initialize();
		audio->Initialize();
		input->Initialize();
	}
	
	~SDL3Backend() {
		platform->Deinitialize();
		graphics->Deinitialize();
		audio->Deinitialize();
		input->Deinitialize();
		delete platform;
		delete graphics;
		delete audio;
		delete input;
	}

	SDL3GraphicsBackend* GetGraphics() const { return static_cast<SDL3GraphicsBackend*>(graphics); }
	SDL3AudioBackend* GetAudio() const { return static_cast<SDL3AudioBackend*>(audio); }
	SDL3InputBackend* GetInput() const { return static_cast<SDL3InputBackend*>(input); }
	SDL3PlatformBackend* GetPlatform() const { return static_cast<SDL3PlatformBackend*>(platform); }
}; 
#endif