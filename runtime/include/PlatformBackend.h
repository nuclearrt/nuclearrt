#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "PakFile.h"

class PlatformBackend {
public:
	virtual void Initialize() {}
	virtual void Deinitialize() {}

	virtual std::string GetName() const { return ""; }

	virtual bool ShouldQuit() { return false; }

	virtual std::string GetPlatformName() { return "Unknown"; }
	virtual std::string GetAssetsFileName() { return ""; }

	virtual unsigned int GetTicks() { return 0; }
	virtual float GetTimeDelta() { return 0.0f; }
	virtual void Delay(unsigned int ms) {}

	//Pak file stuff, maybe move to application class? - shish
	virtual bool PakFileEntryExists(std::string entry) { return pakFile.Exists(entry); }
	virtual std::vector<uint8_t> GetPakFileEntryData(std::string entry) { return pakFile.GetData(entry); }

	PakFile& GetPakFile() { return pakFile; }

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

	virtual void Log(std::string text) {}
protected:
	PakFile pakFile;
	std::unordered_map<unsigned int, std::vector<uint8_t>> collisionMaskCache;
}; 