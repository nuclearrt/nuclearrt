#pragma once

#include "Extension.h"
	
class PerspectiveExtention : public Extension {
public:
PerspectiveExtention(unsigned int objectInfoHandle, int type, std::string name, short width, short height, char effect, bool direction, int zoomValue, int offset, int sineWaveWaves, bool perspectiveDir)
		: Extension(objectInfoHandle, type, name), width(width), height(height), effect(effect), direction(direction), zoomValue(zoomValue), offset(offset), sineWaveWaves(sineWaveWaves), perspectiveDir(perspectiveDir) {}

	void Initialize() override;
	void Draw() override;

	void UpdateShaderParameters();

	void SetEffectType(char effect);
	void SetDirection(bool direction);
	void SetPerspectiveDirection(bool perspectiveDir);

	void SetWidth(int width);
	void SetHeight(int height);
	void SetZoomValue(int zoomValue);
	void SetOffset(int offset);
	void SetSineWaveWaves(int sineWaveWaves);

	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetZoomValue() const { return zoomValue; }
	int GetOffset() const { return offset; }
	int GetSineWaveWaves() const { return sineWaveWaves; }
private:
	short width;
	short height;
	char effect;
	bool direction;
	int zoomValue;
	int offset;
	int sineWaveWaves;
	bool perspectiveDir;
};