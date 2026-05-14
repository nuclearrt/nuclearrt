#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "PlatformBackend.h"
#include <vector>

class SDL3Backend;

class SDL3PlatformBackend : public PlatformBackend {
public:
	void SetBackend(SDL3Backend* b) { backend = b; }
	
	std::string GetName() const override { return "SDL3"; }

	void Initialize() override;

	bool ShouldQuit() override;

	unsigned int GetTicks() override;
	float GetTimeDelta() override;
	void Delay(unsigned int ms) override;

	std::string GetPlatformName() override;
	std::string GetAssetsDirectory() override;

	bool IsWindowFocused() const { return windowFocused; }

	void Log(std::string text) override;
private:
	SDL3Backend* backend = nullptr;

	bool windowFocused = true;
}; 
#endif