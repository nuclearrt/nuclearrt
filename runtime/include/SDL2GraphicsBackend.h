#pragma once

#ifdef NUCLEAR_BACKEND_SDL2

#include "GraphicsBackend.h"

#include <SDL.h>
#include <SDL_ttf.h>

class SDL2Backend;

class SDL2GraphicsBackend : public GraphicsBackend {
public:
	void Initialize() override;
	void Deinitialize() override;

	void SetBackend(SDL2Backend* b) { backend = b; }

	void BeginDrawing() override;
	void EndDrawing() override;
	void Clear(int color) override;

	void CreateRenderTarget(int width, int height);

	void LoadTexture(int id) override;
	void UnloadTexture(int id) override;
	void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) override;
	void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) override;
	
	void DrawRectangle(int x, int y, int width, int height, int color);
	void DrawRectangleLines(int x, int y, int width, int height, int color);
	void DrawLine(int x1, int y1, int x2, int y2, int color);
	void DrawPixel(int x, int y, int color);

	void LoadFont(int id) override;
	void UnloadFont(int id) override;
	void DrawText(FontInfo *fontInfo, int x, int y, int width, int height, unsigned char horizontalAlignment, unsigned char verticalAlignment, int color, const std::string &text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) override;
	
	SDL_FRect CalculateRenderTargetRect();
	SDL_Window* GetSDLWindow() const { return window; }
private:
	SDL2Backend* backend = nullptr;
	
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	SDL_Texture* renderTargetTexture = nullptr;
	int renderTargetWidth = 0;
	int renderTargetHeight = 0;

	SDL_Colour RGBToSDLColor(int color);
	SDL_Colour RGBAToSDLColor(int color);

	std::unordered_map<int, SDL_Texture*> textures;

	std::unordered_map<int, TTF_Font*> fonts;
	std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> fontBuffers;

	int FusionToSDLKey(short key);
}; 

#endif