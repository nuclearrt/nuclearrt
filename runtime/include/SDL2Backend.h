#pragma once

#ifdef NUCLEAR_BACKEND_SDL2

#include "Backend.h"
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef _DEBUG
#include "DebugUI.h"
#endif

#ifdef __SWITCH__
#define INPUT_TOUCH
#endif

class SDL2Backend : public Backend {
public:
	SDL2Backend();
	~SDL2Backend() override;

	std::string GetName() const override { return "SDL2"; }

	void Initialize() override;
	void Deinitialize() override;

	bool ShouldQuit() override;

	std::string GetPlatformName() override;
	std::string GetAssetsFileName() override;

	void BeginDrawing() override;
	void EndDrawing() override;
	void Clear(int color) override;

	void LoadTexture(int id) override;
	void UnloadTexture(int id) override;
	void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) override;
	void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape) override;
	
	void DrawRectangle(int x, int y, int width, int height, int color) override;
	void DrawRectangleLines(int x, int y, int width, int height, int color) override;
	void DrawLine(int x1, int y1, int x2, int y2, int color) override;
	void DrawPixel(int x, int y, int color) override;

	void LoadFont(int id) override;
	void UnloadFont(int id) override;
	void DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) override;
	// Sample Start
	bool LoadSample(int id, int channel) override {return false;}
	bool LoadSampleFile(std::string path) override {return false;}
	void PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) override {}
	void PlaySampleFile(std::string path, int channel, int loops) override {}
	void DiscardSampleFile(std::string path) override {}
	void StopSample(int id, bool channel) override {}
	int FindSample(std::string name) override {return -1;}
	void SetSampleVolume(float volume, int id, bool channel) override {}
	void UpdateSample() override {}
	int GetSampleVolume(int id, bool channel) override {return 0;}
	std::string GetChannelName(int channel) override {return "";}
	void LockChannel(int channel, bool unlock) override {}
	void SetSamplePan(float pan, int id, bool channel) override {}
	int GetSamplePan(int id, bool channel) override {return 0;}
	void SetSampleFreq(int freq, int id, bool channel) override {}
	int GetSampleFreq(int id, bool channel) override {return 0;}
	int GetSampleDuration(int id, bool channel) override {return 0;}
	int GetSamplePos(int id, bool channel) override {return 0;}
	void SetSamplePos(int pos, int id, bool channel) override {}
	void UpdateSample() override {}
	bool SampleState(int id, bool channel, bool pauseOrStop) override {return false;}
	// Sample End
	void GetKeyboardState(uint8_t* outBuffer) override;

	int GetMouseX() override;
	int GetMouseY() override;
	int GetMouseWheelMove() override;
	uint32_t GetMouseState() override;
	void HideMouseCursor() override;
	void ShowMouseCursor() override;

	unsigned int GetTicks() override { return SDL_GetTicks(); }
	float GetTimeDelta() override;
	void Delay(unsigned int ms) override;

#ifdef _DEBUG
	void ToggleDebugUI() { DEBUG_UI.ToggleEnabled(); }
	bool IsDebugUIEnabled() { return DEBUG_UI.IsEnabled(); }
#endif

private:
	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Colour RGBToSDLColor(int color);
	SDL_Colour RGBAToSDLColor(int color);

	std::unordered_map<int, SDL_Texture*> textures;

	std::unordered_map<int, TTF_Font*> fonts;
	std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> fontBuffers;

	int FusionToSDLKey(short key);

#ifdef INPUT_TOUCH
	int touchX = 0;
	int touchY = 0;
	bool touchDown = false;
#endif
}; 

#endif