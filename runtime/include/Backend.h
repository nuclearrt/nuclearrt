#pragma once

class PlatformBackend;
class GraphicsBackend;
class AudioBackend;
class InputBackend;

class Backend {
public:
	PlatformBackend* platform;
	GraphicsBackend* graphics;
	AudioBackend* audio;
	InputBackend* input;
}; 