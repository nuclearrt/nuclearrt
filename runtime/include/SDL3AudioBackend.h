#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "AudioBackend.h"
#include <vector>
#include <unordered_map>
#include <set>

#include <SDL3/SDL.h>

typedef struct SampleFile {
	Uint8 *data = nullptr;
	Uint32 data_len = 0;
	SDL_AudioSpec spec{};
	std::string pathName = "";
} SampleFile;

typedef struct Channel {
	Uint8 *data = nullptr; // No need to make floats, as SDL_AudioStream does convert samples into a format you give via SDL_AudioSpec
	Uint32 data_len = 0;
	SDL_AudioSpec spec{};
	bool uninterruptable = false;
	SDL_AudioStream *stream = nullptr;
	int position = 0;
	bool lock = false;
	bool finished = false;
	int curHandle = -1;
	bool loop = false;
	bool pause = false;
	float volume = 1.0f;
	float pan = 0.0f;
	std::string name = "";
} Channel;

class SDL3Backend;

class SDL3AudioBackend : public AudioBackend {
public:
	void Initialize() override;
	void Deinitialize() override;

	void SetBackend(SDL3Backend* b) { backend = b; }

	static void SDLCALL AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
	bool LoadSample(int id, int channel) override;
	bool LoadSampleFile(std::string path) override;
	int FindSample(std::string name) override;
	void PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) override;
	void PlaySampleFile(std::string path, int channel, int loops) override;
	void DiscardSampleFile(std::string path) override;
	void UpdateSample() override;
	void PauseSample(int id, bool channel, bool pause) override;
	bool SampleState(int id, bool channel, bool pauseOrStop) override;
	int GetSampleVolume(int id) override;
	int GetSampleVolume(std::string name) override;
	int GetChannelVolume(int id) override;
	std::string GetChannelName(int channel) override;
	void SetSampleVolume(float volume, int id, bool channel) override;
	void LockChannel(int channel, bool unlock) override;
	void SetSamplePan(float pan, int id, bool channel) override;
	int GetSamplePan(int id, bool channel) override;
	void SetSampleFreq(int freq, int id, bool channel) override;
	int GetSampleFreq(int id, bool channel) override;
	int GetSampleDuration(int id, bool channel) override;
	int GetSamplePos(int id, bool channel) override;
	void SetSamplePos(int pos, int id, bool channel) override;
	void StopSample(int id, bool channel) override;
	

private:
	static SDL_AudioDeviceID audio_device;
	SDL_AudioSpec spec;
	float mainVol = 100.0f;
	float mainPan = 0.0f;

	std::unordered_map<std::string, SampleFile> sampleFiles;
	Channel channels[49]; // 48 will be the last element.
	SDL_AudioStream* masterStream;
	bool sampleFocusGainApplied = false;
	bool lastWindowFocused = true;

	SDL3Backend* backend = nullptr;
}; 
#endif