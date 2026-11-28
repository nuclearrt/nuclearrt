#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "FontBank.h"
#include "Bitmap.h"

class Counter;
class EffectInstance;
class Shape;

class GraphicsBackend {
public:
	virtual void Initialize() {}
	virtual void Deinitialize() {}
	
	virtual void BeginDrawing() {}
	virtual void EndDrawing() {}
	virtual void Clear(int color) {}

	virtual void BeginLayerDrawing() {}
	virtual void EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) {}

	virtual void LoadTexture(int id) {}
	virtual void UnloadTexture(int id) {}
	virtual void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) {}
	virtual void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) {}
	virtual void DrawCounterBar(int x, int y, Counter* counter) {}
	virtual void DrawBitmap(Bitmap& bitmap, int x, int y) {}
	virtual void DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) {}

	virtual void LoadFont(int id) {}
	virtual void UnloadFont(int id) {}
	virtual void DrawText(FontInfo *fontInfo, int x, int y, int width, int height, unsigned char horizontalAlignment, unsigned char verticalAlignment, int color, const std::string &text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) {}

	virtual bool SupportsTransitions() { return false; }
	virtual void GetTransitionDisplaySize(int& width, int& height) { width = 0; height = 0; }
	virtual void CaptureTransitionSurface(int surface) {}
	virtual void FillTransitionSurface(int surface, int color) {}
	virtual void DrawTransitionSurface(int surface, float xDest, float yDest, float wDest, float hDest, float u0, float v0, float u1, float v1, float alpha, int pattern, float patternWidth, float patternHeight, float surfaceWidth, float surfaceHeight) {}
	virtual void ReleaseTransitionSurfaces() {}
};