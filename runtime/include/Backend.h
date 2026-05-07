#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "FontBank.h"
#include "Shape.h"
#include "PakFile.h"
#include "EffectInstance.h"
#include "Bitmap.h"

class Backend {
public:
	Backend() = default;
	virtual ~Backend() = default;

	virtual std::string GetName() const { return ""; }

	virtual void Initialize() {}
	virtual void Deinitialize() {}

	virtual bool ShouldQuit() { return false; }

	virtual std::string GetPlatformName() { return "Unknown"; }
	virtual std::string GetAssetsFileName() { return ""; }

	virtual unsigned int GetTicks() { return 0; }
	virtual float GetTimeDelta() { return 0.0f; }
	virtual void Delay(unsigned int ms) {}

	virtual void BeginDrawing() {}
	virtual void EndDrawing() {}
	virtual void Clear(int color) {}
	virtual void BeginLayerDrawing() {}
	virtual void EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) {}

	virtual void LoadTexture(int id) {}
	virtual void UnloadTexture(int id) {}
	virtual void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) {}
	virtual void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape) {}
	virtual void DrawBitmap(Bitmap& bitmap, int x, int y) {}
	virtual void DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) {}

	virtual void LoadFont(int id) {}
	virtual void UnloadFont(int id) {}
	virtual void DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) {}
	// Sample Start
	virtual bool LoadSample(int id, int channel) {return false;}
	virtual bool LoadSampleFile(std::string path) {return false;}
	virtual int FindSample(std::string name) {return -1;}
	virtual void PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) {}
	virtual void PlaySampleFile(std::string path, int channel, int loops) {}
	virtual void DiscardSampleFile(std::string path) {}
	virtual void StopSample(int id, bool channel) {}
	virtual void PauseSample(int id, bool channel, bool pause) {}
	virtual void SetSampleVolume(float volume, int id, bool channel) {}
	virtual int GetSampleVolume(int id) {return 0;}
	virtual int GetSampleVolume(std::string name) {return 0;}
	virtual int GetChannelVolume(int id) {return 0;}
	virtual std::string GetChannelName(int channel) {return "";}
	virtual void LockChannel(int channel, bool unlock) {}
	virtual void SetSamplePan(float pan, int id, bool channel) {}
	virtual int GetSamplePan(int id, bool channel) {return 0;}
	virtual int GetSampleFreq(int id, bool channel) {return 0;}
	virtual void SetSampleFreq(int freq, int id, bool channel) {}
	virtual int GetSampleDuration(int id, bool channel) {return 0;}
	virtual int GetSamplePos(int id, bool channel) {return 0;}
	virtual void SetSamplePos(int pos, int id, bool channel) {}
	virtual void UpdateSample() {}
	virtual bool SampleState(int id, bool channel, bool pauseOrStop) {return false;}
	// Sample End
	virtual void GetKeyboardState(uint8_t* outBuffer) {}
	virtual int GetMouseX() { return 0; }
	virtual int GetMouseY() { return 0; }
	virtual void SetMouseX(int x) {}
	virtual void SetMouseY(int y) {}
	virtual int GetMouseWheelMove() { return 0; }
	virtual uint32_t GetMouseState() { return 0; }
	virtual void HideMouseCursor() {}
	virtual void ShowMouseCursor() {}

	//Pak file stuff, maybe move to application class? - shish
	virtual bool PakFileEntryExists(std::string entry) { return pakFile.Exists(entry); }
	virtual std::vector<uint8_t> GetPakFileEntryData(std::string entry) { return pakFile.GetData(entry); }

	const std::vector<uint8_t>* GetCollisionMaskData(unsigned int imageId) {
		auto it = collisionMaskCache.find(imageId);
		if (it != collisionMaskCache.end())
			return &it->second;
		std::vector<uint8_t> data = pakFile.GetData("images/masks/" + std::to_string(imageId) + ".bin");
		if (data.empty())
			return nullptr;
		it = collisionMaskCache.emplace(imageId, std::move(data)).first;
		return &it->second;
	}

protected:
	PakFile pakFile;
	std::unordered_map<unsigned int, std::vector<uint8_t>> collisionMaskCache;
}; 