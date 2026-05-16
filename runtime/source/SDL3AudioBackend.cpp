#ifdef NUCLEAR_BACKEND_SDL3

#include "SDL3AudioBackend.h"


#include "Application.h"
#include <iostream>
#include <filesystem>
#include <SDL3/SDL.h>
#include "SoundBank.h"
#include "SDL3Backend.h"
#include "./libs/stb_vorbis.c" // OGG SUPPORT
#define DR_MP3_IMPLEMENTATION
#include "./libs/dr_mp3.h" // MP3 SUPPORT

//fix for AudioCallback race condition
class AudioStreamLock {
public:
	explicit AudioStreamLock(SDL_AudioStream* stream) : stream(stream) {
		Lock();
	}

	~AudioStreamLock() {
		Unlock();
	}

	void Lock() {
		if (stream && !locked) {
			SDL_LockAudioStream(stream);
			locked = true;
		}
	}

	void Unlock() {
		if (locked) {
			SDL_UnlockAudioStream(stream);
			locked = false;
		}
	}

	AudioStreamLock(const AudioStreamLock&) = delete;
	AudioStreamLock& operator=(const AudioStreamLock&) = delete;

private:
	SDL_AudioStream* stream = nullptr;
	bool locked = false;
};

SDL_AudioDeviceID SDL3AudioBackend::audio_device = NULL;

void SDLCALL SDL3AudioBackend::AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	AudioStreamLock lock(stream);
	auto& channels = *(Channel(*)[49])userdata;
	int frames = additional_amount / (sizeof(float) * 2);
	float mixBuffer[8192 * 2] = {0}; // Initilaze array so no garbage data is found
	if (frames > 8192) frames = 8192;
	for (int i = 0; i < frames; ++i) {
		float left = 0.0f, right = 0.0f;
		for (int ch = 1; ch < SDL_arraysize(channels); ++ch) {
			Channel& channel = channels[ch];
			if (!channel.stream) continue;
			float tempData[2] = {0};
			int getData = SDL_GetAudioStreamData(channel.stream, tempData, sizeof(tempData));
			if (getData <= 0) { // Channel has finished playing.
				channel.finished = true;
				continue;
			}
			channel.position += getData / (sizeof(float) * 2);
			// Prepare volume + pan handling
			float angle = (channel.pan + 1.0f) * 0.25f * SDL_PI_F;
			float leftGain = SDL_cosf(angle) * channel.volume;
			float rightGain = SDL_sinf(angle) * channel.volume;
			left += tempData[0] * leftGain;
			right += tempData[1] * rightGain;
		}
		left = fmaxf(-1.0f, fminf(left, 1.0f));
		right = fmaxf(-1.0f, fminf(right, 1.0f));

		mixBuffer[i * 2 + 1] = right;
		mixBuffer[i * 2 + 0] = left;
	}
	SDL_PutAudioStreamData(stream, mixBuffer, static_cast<unsigned long long>(frames) * 2 * sizeof(float)); // Using static_cast from what visual studio recommended
}

void SDL3AudioBackend::Initialize()
{
	spec.freq = 44100;
	spec.channels = 2;
	spec.format = SDL_AUDIO_F32;
	audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	if (!audio_device) {
		backend->GetPlatform()->Log("SDL_OpenAudioDevice Error : " + std::string(SDL_GetError()));
		return;
	}
	masterStream = SDL_CreateAudioStream(&spec, NULL);
	SDL_BindAudioStream(audio_device, masterStream);
	SDL_SetAudioStreamGetCallback(masterStream, AudioCallback, &channels); // Put callback only runs when SDL_PutAudioStreamData is ran, so use the getcallback to put data instead
	backend->GetPlatform()->Log("Opened Audio Device.");
}

void SDL3AudioBackend::Deinitialize()
{	
	AudioStreamLock lock(masterStream);
	// Close the Audio Device
	SDL_PauseAudioDevice(audio_device);
	SDL_SetAudioStreamGetCallback(masterStream, NULL, NULL);
	SDL_ClearAudioStream(masterStream);
	// cleanup audio
	while (!sampleFiles.empty()) DiscardSampleFile(sampleFiles.begin()->first);
	for (int i = 1; i < SDL_arraysize(channels); i++) {
		if (!channels[i].stream) continue;
		if (channels[i].data) {
			SDL_free(channels[i].data);
			channels[i].data = nullptr;
			channels[i].data_len = 0;
		}
		SDL_UnbindAudioStream(channels[i].stream);
		SDL_ClearAudioStream(channels[i].stream);
		SDL_DestroyAudioStream(channels[i].stream);
		channels[i].stream = nullptr;
	}

	lock.Unlock();
	SDL_UnbindAudioStream(masterStream);
	SDL_DestroyAudioStream(masterStream);
	SDL_CloseAudioDevice(audio_device);
	backend->GetPlatform()->Log("AudioBackend shut down successfully.");
}

bool SDL3AudioBackend::LoadSample(int id, int channel) {
	AudioStreamLock lock(masterStream);

	if (id < 0) return false;
	if (channels[channel].data && channels[channel].curHandle == id) {
		return true;
	}
	SoundInfo* soundInfo = SoundBank::Instance().GetSound(id);
	if (!soundInfo) {
		backend->GetPlatform()->Log("SoundBank Error: Sound ID " + std::to_string(id) + " not found!");
		return false;
	}
	std::vector<uint8_t> data = backend->platform->GetPakFile().GetData("sounds/" + std::to_string(id) + "." + soundInfo->Type);
	if (data.empty()) {
		backend->GetPlatform()->Log("PakFile::GetData Error: Sample with id " + std::to_string(id) + " not found");
		return false;
	}
	if (soundInfo->Type == "wav") {
		SDL_IOStream* stream = SDL_IOFromMem(data.data(), data.size());
		if (!SDL_LoadWAV_IO(stream, true, &channels[channel].spec, &channels[channel].data, &channels[channel].data_len)) {
			backend->GetPlatform()->Log("SDL_LoadWAV_IO Error (WAV) : " + std::string(SDL_GetError()));
			return false;
		}
	}
	else if (soundInfo->Type == "ogg") {
		int channels, samplerate;
		short* output = nullptr;
		int numSamples = stb_vorbis_decode_memory(data.data(), data.size(), &channels, &samplerate, &output);
		if (numSamples <= 0 || !output) {
			backend->GetPlatform()->Log("stb_vorbis_decode_memory failed.");
			return false;
		}
		int totalSamples = numSamples * channels;
		this->channels[channel].data_len = totalSamples * sizeof(short);
		this->channels[channel].data = (Uint8*)SDL_malloc(this->channels[channel].data_len);
		SDL_memcpy(this->channels[channel].data, output, this->channels[channel].data_len);
		this->channels[channel].spec.freq = samplerate;
		this->channels[channel].spec.channels = channels;
		this->channels[channel].spec.format = SDL_AUDIO_S16;
		free(output);
	}
	else if (soundInfo->Type == "mp3") {
		drmp3 mp3;
		if (!drmp3_init_memory(&mp3, data.data(), data.size(), NULL)) {
			backend->GetPlatform()->Log("Failed to decode mp3 data.");
			drmp3_uninit(&mp3);
			return false;
		}
		drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);
		if (frameCount == 0) {
			backend->GetPlatform()->Log("No sample frames in MP3");
			drmp3_uninit(&mp3);
			return false;
		}
		int totalSamples = static_cast<int>(frameCount * mp3.channels);
		Uint32 dataLen = totalSamples * sizeof(int16_t);
		channels[channel].data = (Uint8*)SDL_malloc(dataLen);
		drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(&mp3, frameCount, (drmp3_int16*)channels[channel].data);
		if (!channels[channel].data) {
			backend->GetPlatform()->Log("Bad MP3 Data");
			drmp3_uninit(&mp3);
			return false;
		}
		channels[channel].data_len = dataLen;
		channels[channel].spec.channels = mp3.channels;
		channels[channel].spec.format = SDL_AUDIO_S16;
		channels[channel].spec.freq = mp3.sampleRate;
		drmp3_uninit(&mp3);
	}
	else {
		backend->GetPlatform()->Log("Audio Data Type" + soundInfo->Type + "not supported.");
		return false;
	}
	channels[channel].name = soundInfo->Name;
	return true;
}
bool SDL3AudioBackend::LoadSampleFile(std::string path) {
	AudioStreamLock lock(masterStream);
	backend->GetPlatform()->Log("Loading Sample File : " + path);
	std::filesystem::path fullPath = path;
	std::string type = fullPath.extension().string();
	SampleFile sampleFile;
	if (type == ".wav") {
		if (!SDL_LoadWAV(path.c_str(), &sampleFile.spec, &sampleFile.data, &sampleFile.data_len)) {
			backend->GetPlatform()->Log("Failed to load WAV file : " + std::string(SDL_GetError()));
			return false;
		}
		backend->GetPlatform()->Log("Loaded WAV Sample File : " + path);
	}
	else if (type == ".ogg") {
		int channels, samplerate;
		short* output = nullptr;
		int numSamples = stb_vorbis_decode_filename(path.c_str(), &channels, &samplerate, &output);
		if (numSamples <= 0 || !output) {
			backend->GetPlatform()->Log("Failed to load OGG file : " + path);
			return false;
		}
		int totalSamples = numSamples * channels;
		sampleFile.data_len = totalSamples * sizeof(short);
		sampleFile.data = (Uint8*)SDL_malloc(sampleFile.data_len);
		SDL_memcpy(sampleFile.data, output, sampleFile.data_len);
		sampleFile.spec.freq = samplerate;
		sampleFile.spec.format = SDL_AUDIO_S16;
		sampleFile.spec.channels = channels;
		free(output);
		backend->GetPlatform()->Log("Loaded OGG file : " + path);
	}
	else if (type == ".mp3") {
		drmp3 mp3;
		if (!drmp3_init_file(&mp3, path.c_str(), NULL)) {
			backend->platform->Log("Couldn't find MP3 file");
			return false;
		}
		drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);
		if (frameCount == 0) {
			backend->GetPlatform()->Log("No sample frames in MP3");
			drmp3_uninit(&mp3);
			return false;
		}
		int totalSamples = static_cast<int>(frameCount * mp3.channels);
		Uint32 dataLen = totalSamples * sizeof(int16_t);
		sampleFile.data = (Uint8*)SDL_malloc(dataLen);
		drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(&mp3, frameCount, (drmp3_int16*)sampleFile.data);
		if (!sampleFile.data) {
			backend->platform->Log("Bad MP3 Data.");
			drmp3_uninit(&mp3);
			return false;
		}
		sampleFile.data_len = dataLen;
		sampleFile.spec.channels = mp3.channels;
		sampleFile.spec.format = SDL_AUDIO_S16;
		sampleFile.spec.freq = mp3.sampleRate;
		drmp3_uninit(&mp3);
	}
	else {
		backend->GetPlatform()->Log("Audio File" + type + "not supported.");
		return false;
	}
	sampleFile.pathName = path;
	sampleFiles.emplace(sampleFile.pathName, sampleFile);
	return true;
}
int SDL3AudioBackend::FindSample(std::string name) {
	SoundInfo* soundInfo = SoundBank::Instance().GetSoundName(name);
	if (soundInfo) {
		return soundInfo->Handle;
	}
	return -1;
}

void SDL3AudioBackend::PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) {
	AudioStreamLock lock(masterStream);
	bool replaceSample = false;
	bool channelsFilled = false;
	bool channelFound = false;
	if (channel < 1 || channel >= SDL_arraysize(channels)) {
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (!channels[i].stream && !channels[i].data && !channels[i].lock) {
				channel = i;
				channelFound = true;
				break;
			}
		}

		if (!channelFound) {
			channelsFilled = true;
			channel = 48;
		}
		channels[channel].uninterruptable = uninterruptable;
	}
	else { // Channel is given.
		channels[channel].uninterruptable = uninterruptable;
		if (channels[channel].uninterruptable) replaceSample = true;
	}
	if (replaceSample) {
		StopSample(channel, true);
	}
	if (!LoadSample(id, channel)) return;

	if (channels[channel].stream) StopSample(channel, true);
	channels[channel].stream = SDL_CreateAudioStream(&channels[channel].spec, &spec);
	if (!channels[channel].stream) {
		backend->GetPlatform()->Log("SDL_CreateAudioStream error : " + std::string(SDL_GetError()));
		channels[channel].stream = nullptr;
		return;
	}
	channels[channel].loop = (loops <= 0);
	channels[channel].position = 0;
	channels[channel].pause = false;
	if (channels[channel].loop) SDL_PutAudioStreamData(channels[channel].stream, channels[channel].data, channels[channel].data_len);
	else {
		for (int i = 1; i <= loops; i++) {
			SDL_PutAudioStreamData(channels[channel].stream, channels[channel].data, channels[channel].data_len);
		}
	}
	if (volume > -1) channels[channel].volume = volume;
	if (pan != -2 ) channels[channel].pan = pan;
	if (freq > 0 || freq != NULL) SetSampleFreq(freq, channel, true);
	channels[channel].curHandle = id;
	SetSampleVolume(mainVol, channel, true); // Set volume to the main one.
	
	//backend->GetPlatform()->Log("Sample ID " + std::to_string(id) + " is now playing at channel " + std::to_string(channel) + ".");
}

void SDL3AudioBackend::PlaySampleFile(std::string path, int channel, int loops) {
	AudioStreamLock lock(masterStream);
	auto it = sampleFiles.find(path);
	if (it == sampleFiles.end()) {
		backend->GetPlatform()->Log("Can't find sample path.");
		return;
	}
	SampleFile& sampleFile = it->second;
	if (channels[channel].stream || channels[channel].data || channels[channel].lock || channels[channel].uninterruptable) return;
	StopSample(channel, true);
	channels[channel].data = (Uint8*)SDL_malloc(sampleFile.data_len);
	SDL_memcpy(channels[channel].data, sampleFile.data, sampleFile.data_len);
	channels[channel].data_len = sampleFile.data_len;
	channels[channel].spec = sampleFile.spec;

	channels[channel].stream = SDL_CreateAudioStream(&channels[channel].spec, &spec);
	channels[channel].loop = (loops <= 0);
	channels[channel].position = 0;
	channels[channel].pause = false;
	channels[channel].name = sampleFile.pathName;
	channels[channel].uninterruptable = false;
	if (channels[channel].loop) SDL_PutAudioStreamData(channels[channel].stream, channels[channel].data, channels[channel].data_len);
	else {
		for (int i = 1; i <= loops; i++) {
			SDL_PutAudioStreamData(channels[channel].stream, channels[channel].data, channels[channel].data_len);
		}
	}
	SetSampleVolume(mainVol, channel, true);
	DiscardSampleFile(path);
}

void SDL3AudioBackend::DiscardSampleFile(std::string path) {
	AudioStreamLock lock(masterStream);
	auto it = sampleFiles.find(path);
	if (it == sampleFiles.end()) {
		backend->GetPlatform()->Log("Can't find sample path.");
		return;
	}
	SampleFile& sampleFile = it->second;
	if (sampleFile.data) {
		SDL_free(sampleFile.data);
		sampleFile.data = nullptr;
	}
	sampleFiles.erase(it);
}
// ALL SAMPLE CONDITIONS HERE

bool SDL3AudioBackend::SampleState(int id, bool channel, bool pause) {
	AudioStreamLock lock(masterStream);
	if (id == -1 && !channel && !pause) { // No Sample is playing
		int countStream = 0;
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].stream) countStream++;
		}
		if (countStream == 0) return true;
		else return false;
	}
	if (channel) { // Check if channel is not playing/paused
		if (id < 1 || id >= SDL_arraysize(channels)) return false;
		if (pause && channels[id].pause) return true;
		if (!channels[id].stream && !pause) return true;
	}
	if (id > -1 && !channel) { // Check for specific sample not playing/paused.
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) {
				if (pause && channels[i].pause) {
					return true;
				}
				if (!channels[i].stream && !pause) return true;
			}
			else { 
				if (!pause) return true;
				else return false;
			}
		}
	}
	return false;
}
void SDL3AudioBackend::PauseSample(int id, bool channel, bool pause) {
	AudioStreamLock lock(masterStream);
	if (id == -1 && !channel) { // Pause/Resume all sounds
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			PauseSample(i, true, pause);
		}
	}
	if (channel) { // Pause/Resume specific channel
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) {
			if (pause) {
				SDL_PauseAudioStreamDevice(channels[id].stream);
				channels[id].pause = true;
			}
			else {
				SDL_ResumeAudioStreamDevice(channels[id].stream);
				channels[id].pause = false;
			}
		}
		return;
	}
	if (id > -1 && !channel) { // Pause/Resume sample handle.
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) PauseSample(i, true, pause);
		}
	}
}
void SDL3AudioBackend::SetSamplePan(float pan, int id, bool channel) {
	AudioStreamLock lock(masterStream);
	bool setMain = false;
	pan /= 100;
	if (pan < -1.0f) pan = -1.0f;
	if (pan > 1.0f) pan = 1.0f;
	if (!channel && id <= -1) { // Set Main Pan
		setMain = true;
		mainPan = pan;
		for (int i = 1; i < SDL_arraysize(channels); ++i) {
			channels[i].pan = channels[i].pan + mainPan;
		}
	}
	if (channel) { // Set Channel Pan
		setMain = false;
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) channels[id].pan = pan;
	}
	if (id > -1 && !channel) { // Set Sample Pan
		setMain = false;
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) channels[i].pan = pan;
			else continue;
		}
	}
}
int SDL3AudioBackend::GetSamplePan(int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (id == -1 && !channel) return mainPan;
	if (channel) { // Get Channel Volume
		if (id < 1 || id >= SDL_arraysize(channels)) return 0;
		return channels[id].pan * 100;
	}
	if (!channel && id >= 0) {
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) return channels[i].pan * 100;
		}
	}
	return 0;
}
void SDL3AudioBackend::SetSamplePos(int pos, int id, bool channel)
{
	AudioStreamLock lock(masterStream);
	if (channel) {
		if (id < 0 || id >= SDL_arraysize(channels)) return;
		if (!channels[id].data || !channels[id].stream) return;
		int length = channels[id].data_len / (sizeof(float) * 2);
		pos = SDL_clamp(pos, 0, length);
		channels[id].position = pos;
		SDL_ClearAudioStream(channels[id].stream);
		Uint8* positionData = channels[id].data + pos * sizeof(float) * 2;
		Uint32 positionLength = channels[id].data_len - pos * sizeof(float) * 2;
		SDL_PutAudioStreamData(channels[id].stream, positionData, positionLength);
	}
	else {
		if (id < 0) return;
		for (int i = 0; i < SDL_arraysize(channels); ++i) {
			if (channels[i].curHandle == id) SetSamplePos(pos, i, true);
		}
	}
}
void SDL3AudioBackend::SetSampleVolume(float volume, int id, bool channel) {
	AudioStreamLock lock(masterStream);
	bool setMain = false;
	if (id == -1 && !channel) { // Set Main Volume
		mainVol = volume;
		setMain = true;
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			SetSampleVolume(volume, i, true);
		}
	}
	if (channel) { // Set Channel Volume
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) {
			channels[id].volume = volume / 100;
			if (!setMain) channels[id].volume = volume / 100;
			else {
				mainVol = (volume / 100) * channels[id].volume;
				channels[id].volume = mainVol;
			}	
		}
	}
	if (id > -1 && !channel) { // Set Sample Volume
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) SetSampleVolume(volume, i, true);
			else continue;
		}
	}
}
void SDL3AudioBackend::SetSampleFreq(int freq, int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (channel) { // Set Channel Freq
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) {
			if (freq > 0) SDL_SetAudioStreamFrequencyRatio(channels[id].stream, static_cast<float>(freq) / static_cast<float>(channels[id].spec.freq));
			else SDL_SetAudioStreamFrequencyRatio(channels[id].stream, 1.0f);
		}
	}
	if (id > -1 && !channel) { // Set Sample Freq
		for (int i = 1; i < SDL_arraysize(channels); ++i) {
			if (channels[i].curHandle == id) SetSampleFreq(freq, i, true);
			else continue;
		}
	}
}
int SDL3AudioBackend::GetSampleFreq(int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (channel) { // Get Channel Freq
		if (id < 1 || id >= SDL_arraysize(channels) || !channels[id].stream) return 0;
		return channels[id].spec.freq * SDL_GetAudioStreamFrequencyRatio(channels[id].stream);
	}
	if (id > -1 && !channel) {
		for (int i = 1; i < SDL_arraysize(channels); ++i) {
			if (channels[i].curHandle == id && channels[i].stream) return channels[i].spec.freq * SDL_GetAudioStreamFrequencyRatio(channels[i].stream);
		}
	}
	return 0;
}
int SDL3AudioBackend::GetSampleVolume(int id) {
	AudioStreamLock lock(masterStream);
	if (id == -1) return mainVol;
	if (id > -1) { // Get Sample Volume
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id && channels[i].stream) return channels[i].volume;
		}
	}
	return 0;
}

int SDL3AudioBackend::GetSampleVolume(std::string name) {
	return GetSampleVolume(FindSample(name));
}

int SDL3AudioBackend::GetChannelVolume(int id) {
	AudioStreamLock lock(masterStream);
	if (id < 1 || id >= SDL_arraysize(channels)) return -1;
	return channels[id].volume;
}

std::string SDL3AudioBackend::GetChannelName(int channel) {
	AudioStreamLock lock(masterStream);
	if (channel < 0 || channel >= SDL_arraysize(channels)) return "";
	return channels[channel].name;
}

void SDL3AudioBackend::LockChannel(int channel, bool unlock) {
	AudioStreamLock lock(masterStream);
	if (channel < 0 || channel >= SDL_arraysize(channels)) return;
	channels[channel].lock = !unlock;
}

int SDL3AudioBackend::GetSampleDuration(int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (channel && id > 0 && id < SDL_arraysize(channels)) return static_cast<int>(channels[id].data_len);
	if (!channel && id > -1) {
		for (int i = 1; i < SDL_arraysize(channels); ++i) if (channels[i].curHandle == id) return static_cast<int>(channels[i].data_len);
	}
	return 0;
}

int SDL3AudioBackend::GetSamplePos(int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (channel && id > 0 && id < SDL_arraysize(channels)) return channels[id].position;
	if (!channel && id > -1) {
		for (int i = 1; i < SDL_arraysize(channels); ++i) if (channels[i].curHandle == id) return channels[i].position;
	}
	return 0;
}

void SDL3AudioBackend::StopSample(int id, bool channel) {
	AudioStreamLock lock(masterStream);
	if (id == -1) { // Stop any sample
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			StopSample(i, true);
		}
		return;
	}
	if (channel) { // check for the channel
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) {
			SDL_UnbindAudioStream(channels[id].stream);
			SDL_DestroyAudioStream(channels[id].stream);
			channels[id].stream = nullptr;
			if (channels[id].data) {
				SDL_free(channels[id].data);
			}
			channels[id].data = nullptr;
			channels[id].data_len = 0;
		}
		channels[id].name = "";
		channels[id].curHandle = -1;
		channels[id].uninterruptable = false;
		channels[id].position = 0;
		return;
	}
	if (!channel && id > -1) { // check for sample handle
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id) StopSample(i, true);
		}
	}
}
void SDL3AudioBackend::UpdateSample() {
	if (!Application::Instance().GetAppData()->GetSampleFocus()) {
		const bool focused = backend->platform && backend->GetPlatform()->IsWindowFocused();
		if (!sampleFocusGainApplied || focused != lastWindowFocused) {
			SDL_SetAudioDeviceGain(audio_device, focused ? 1.0f : 0.0f);
			lastWindowFocused = focused;
			sampleFocusGainApplied = true;
		}
	}
	else {
		sampleFocusGainApplied = false;
	}
	AudioStreamLock lock(masterStream);
	for (int i = 1; i < SDL_arraysize(channels); ++i) {
		channels[i].volume = SDL_clamp(channels[i].volume, 0, 1); // Clamp Volume
		if (channels[i].stream) {
			if (channels[i].finished) {
				channels[i].finished = false;
				if (!channels[i].loop) {
					StopSample(i, true);
					continue;
				}
				else SDL_PutAudioStreamData(channels[i].stream, channels[i].data, channels[i].data_len);
			}
		}
		else continue;
	}
}
#endif