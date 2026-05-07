#ifdef NUCLEAR_BACKEND_SDL3

#include "SDL3Backend.h"

#include <iostream>
#include "Application.h"
#include "Frame.h"
#include "FontBank.h"
#include "SoundBank.h"
#include "ImageBank.h"
#include "PakFile.h"
#include <math.h>
#include <filesystem>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "./libs/stb_vorbis.c" // OGG SUPPORT
#define DR_MP3_IMPLEMENTATION
#include "./libs/dr_mp3.h" // MP3 SUPPORT
#ifdef _DEBUG
#include "DebugUI.h"
#include "imgui.h"
#include <imgui_impl_sdl3.h>
#endif

SDL_AudioDeviceID SDL3Backend::audio_device = NULL;
SDL3Backend::SDL3Backend() {
}

SDL3Backend::~SDL3Backend() {
	Deinitialize();
}
void SDLCALL SDL3Backend::AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
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

void SDL3Backend::Initialize() {
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	std::string windowTitle = Application::Instance().GetAppData()->GetAppName();
	
	
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return;
	}

	if (!TTF_Init()) {
		std::cerr << "TTF_Init Error: " << SDL_GetError() << std::endl;
		return;
	}

	// Create the window
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
	window = SDL_CreateWindow(windowTitle.c_str(), windowWidth, windowHeight, flags);
	if (window == nullptr) {
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return;
	}
	// Create the Audio Device
	spec.freq = 44100;
	spec.channels = 2;
	spec.format = SDL_AUDIO_F32;
	audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!audio_device) {
        std::cerr << "SDL_OpenAudioDevice Error : " << SDL_GetError() << std::endl;
        return;
    }
	masterStream = SDL_CreateAudioStream(&spec, NULL);
	SDL_BindAudioStream(audio_device, masterStream);
	SDL_SetAudioStreamGetCallback(masterStream, AudioCallback, &channels); // Put callback only runs when SDL_PutAudioStreamData is ran, so use the getcallback to put data instead
	std::cout << "Opened Audio Device.\n";
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		return;
	}
		
	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1);

	
	#if !defined(PLATFORM_MACOS)
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::cerr << "GLEW Init Error: " << glewGetErrorString(glewErr) << std::endl;
		return;
	}
	#endif
	
	glEnable(GL_BLEND);

	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	//load assets
	if (!pakFile.Load(GetAssetsFileName())) {
		std::cerr << "PakFile::Load Error: " << "Failed to load assets file" << std::endl;
		return;
	}

	float verts[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};
	
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CreateStandardShaders();

	CreateRenderTarget(windowWidth, windowHeight);

#ifdef _DEBUG
	DEBUG_UI.Initialize(window, glContext);
	
	DEBUG_UI.AddWindow(Application::Instance().GetAppData()->GetAppName(), [this]() {
		ImGui::Text("Platform: %s", GetPlatformName().c_str());
		ImGui::Text("Assets File: %s", GetAssetsFileName().c_str());

		if (ImGui::CollapsingHeader("Window")) {
			ImGui::Checkbox("Fit Inside", &Application::Instance().GetAppData()->GetFitInside());
			ImGui::Checkbox("Resize Display", &Application::Instance().GetAppData()->GetResizeDisplay());
			ImGui::Checkbox("Dont Center Frame", &Application::Instance().GetAppData()->GetDontCenterFrame());
		}
		
		if(ImGui::CollapsingHeader("Global Variables")) {
			if (ImGui::CollapsingHeader("Values")) {
				std::vector<int>& altValues = Application::Instance().GetAppData()->GetGlobalValues();
				for (int i = 0; i < altValues.size(); i++) {
					ImGui::InputInt(("Value " + std::to_string(i)).c_str(), &altValues[i]);
				}
			}
			if (ImGui::CollapsingHeader("Strings")) {
				std::vector<std::string>& altStrings = Application::Instance().GetAppData()->GetGlobalStrings();
				for (int i = 0; i < altStrings.size(); i++) {
					char buffer[256];
					strncpy(buffer, altStrings[i].c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					if (ImGui::InputText(("String " + std::to_string(i)).c_str(), buffer, sizeof(buffer))) {
						altStrings[i] = buffer;
					}
				}
			}
		}

		//jump to frame
		static int frameIndex = 0;
		ImGui::InputInt("Frame Index", &frameIndex);
		if (ImGui::Button("Jump to Frame")) {
			Application::Instance().QueueStateChange(GameState::JumpToFrame, frameIndex);
		}

		if (ImGui::CollapsingHeader("Current Frame")) {
			Frame* currentFrame = Application::Instance().GetCurrentFrame().get();
			ImGui::Text("Current Frame: %s", currentFrame->Name.c_str());
			ImGui::Text("Current Frame Index: %d", currentFrame->Index);

			if (ImGui::TreeNode("Object Instances")) {
				int i = 0;
				for (auto& [handle, instance] : currentFrame->ObjectInstances) {					
					if (ImGui::TreeNode(std::string(instance->Name + "##" + std::to_string(handle)).c_str())) {
						ImGui::Text("Handle: %d", handle);
						ImGui::Text("Position: %d, %d", instance->X, instance->Y);
						ImGui::Text("Type: %d", instance->Type);

						if (instance->Type == 2)
						{
							ImGui::Checkbox("Visible", &((Active*)instance)->Visible);
						}
						else if (instance->Type == 3)
						{
							ImGui::Checkbox("Visible", &((StringObject*)instance)->Visible);
							
							if (ImGui::TreeNode("Paragraphs")) {
								ImGui::Text("Displayed Text: %s", ((StringObject*)instance)->GetText().c_str());
								ImGui::Text("Alterable Text: %s", ((StringObject*)instance)->AlterableText.c_str());

								if (ImGui::TreeNode("Paragraphs")) {
									for (int i = 0; i < ((StringObject*)instance)->Paragraphs.size(); i++) {
										ImGui::Text("Paragraph %d: %s", i, ((StringObject*)instance)->Paragraphs[i].Text.c_str());
									}
									ImGui::TreePop();
								}

								ImGui::TreePop();
							}
						}
						else if (instance->Type == 7)
						{
							ImGui::Text("Value: %d", ((Counter*)instance)->GetValue());
						}

						if (ImGui::TreeNode("Effect")) {
							ImGui::Text("Effect: %d", instance->Effect);
							ImGui::Text("Effect Parameter: %d", instance->GetEffectParameter());
							ImGui::TreePop();
						}

						ImGui::TreePop();
					}

					i++;
				}
				ImGui::TreePop();
			}
		}
	});
#endif
}

void SDL3Backend::Deinitialize()
{
#ifdef _DEBUG
	DEBUG_UI.Shutdown();
#endif

	// cleanup textures
	for (auto& pair : textures) {
		if (pair.second.textureId != 0) {
			glDeleteTextures(1, &pair.second.textureId);
		}
	}
	textures.clear();

	// cleanup text texture cache
	for (auto& pair : textCache) {
		if (pair.second.texture.textureId != 0) {
			glDeleteTextures(1, &pair.second.texture.textureId);
		}
	}
	textCache.clear();

	// cleanup fonts
	for (auto& pair : fonts) {
		TTF_CloseFont(pair.second);
	}
	fonts.clear();
	fontBuffers.clear();
	
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
	SDL_UnbindAudioStream(masterStream);
	SDL_DestroyAudioStream(masterStream);
	SDL_CloseAudioDevice(audio_device);
	std::cout << "AudioBackend shut down successfully.\n";
	
	if (renderTarget != 0) {
		glDeleteFramebuffers(1, &renderTarget);
		renderTarget = 0;
	}

	if (renderTargetTexture != 0) {
		glDeleteTextures(1, &renderTargetTexture);
		renderTargetTexture = 0;
	}

	if (layerRenderTarget != 0) {
		glDeleteFramebuffers(1, &layerRenderTarget);
		layerRenderTarget = 0;
	}

	if (layerRenderTargetTexture != 0) {
		glDeleteTextures(1, &layerRenderTargetTexture);
		layerRenderTargetTexture = 0;
	}
	layerRenderTargetWidth = 0;
	layerRenderTargetHeight = 0;

	if (quadVAO != 0) {
		glDeleteVertexArrays(1, &quadVAO);
		quadVAO = 0;
	}

	if (quadVBO != 0) {
		glDeleteBuffers(1, &quadVBO);
		quadVBO = 0;
	}

	for (int i = 0; i < STANDARD_EFFECT_COUNT; i++) {
		if (effectShaders[i].program != 0) {
			glDeleteProgram(effectShaders[i].program);
			effectShaders[i].program = 0;
		}
	}

	if (colorShaderProgram != 0) {
		glDeleteProgram(colorShaderProgram);
		colorShaderProgram = 0;
	}

	if (textureQuickbackdropShaderProgram != 0) {
		glDeleteProgram(textureQuickbackdropShaderProgram);
		textureQuickbackdropShaderProgram = 0;
	}

	if (gradientShaderProgram != 0) {
		glDeleteProgram(gradientShaderProgram);
		gradientShaderProgram = 0;
	}

	for (auto& pair : thirdPartyShaders) {
		if (pair.second.program != 0) {
			glDeleteProgram(pair.second.program);
		}
	}
	thirdPartyShaders.clear();
	
	if (glContext != nullptr) {
		SDL_GL_DestroyContext(glContext);
		glContext = nullptr;
	}
	
	// Destroy the window
	if (window != nullptr) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	TTF_Quit();
	SDL_Quit();
}

bool SDL3Backend::ShouldQuit()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
#ifdef _DEBUG
		// Process ImGui events
		if (DEBUG_UI.IsEnabled()) {
			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		
		// Toggle debug UI with F1 key
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1 && event.key.repeat == 0) {
			DEBUG_UI.ToggleEnabled();
		}
#endif
		if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) windowFocused = true;

		if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) windowFocused = false;

		if (event.type == SDL_EVENT_QUIT) {
			return true;
		}
	}
	return false;
}

std::string SDL3Backend::GetPlatformName()
{
#if defined(PLATFORM_WINDOWS)
	return "Windows";
#elif defined(PLATFORM_MACOS)
	return "macOS";
#elif defined(PLATFORM_LINUX)
	return "Linux";
#else
	return "Unknown";
#endif
}

std::string SDL3Backend::GetAssetsFileName()
{
	const char* basePath = SDL_GetBasePath();
	return std::string(basePath) + "assets.pak";
}

void SDL3Backend::BeginDrawing()
{
	if (glContext == nullptr) {
		std::cerr << "BeginDrawing called with null renderer!" << std::endl;
		return;
	}

	currentEffect = -1;

	//resize render target if needed
	int newWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	int newHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);

	if (newWidth != renderTargetWidth || newHeight != renderTargetHeight) {
		CreateRenderTarget(newWidth, newHeight);
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
	glViewport(0, 0, renderTargetWidth, renderTargetHeight);
	drawingLayer = false;

#ifdef _DEBUG
	DEBUG_UI.BeginFrame();
#endif
}

void SDL3Backend::EndDrawing()
{
	if (glContext == nullptr) {
		std::cerr << "EndDrawing called with null renderer!" << std::endl;
		return;
	}

	// unbind render target
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);
	
	// clear with border color
	int borderColor = Application::Instance().GetAppData()->GetBorderColor();
	float r = ((borderColor >> 16) & 0xFF) / 255.0f;
	float g = ((borderColor >> 8) & 0xFF) / 255.0f;
	float b = (borderColor & 0xFF) / 255.0f;
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	SDL_FRect rect = CalculateRenderTargetRect();
	
	UseEffectShader(0);
	EffectShader& shader = effectShaders[0];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderTargetTexture);
	glUniform1i(shader.texLoc, 0);
	glUniform4f(shader.colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
	
	float mvp[16] = {
		2.0f * rect.w / windowWidth, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f * rect.h / windowHeight, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f * rect.x / windowWidth - 1.0f, -(2.0f * rect.y / windowHeight - 1.0f) - 2.0f * rect.h / windowHeight, 0.0f, 1.0f
	};
	glUniformMatrix4fv(shader.mvpLoc, 1, GL_FALSE, mvp);

	float presentVerts[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(presentVerts), presentVerts);
	
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

#ifdef _DEBUG
	DEBUG_UI.EndFrame();
#endif

	SDL_GL_SwapWindow(window);

	if (!renderedFirstFrame) {
		renderedFirstFrame = true;
		SDL_ShowWindow(window);
	}
}

void SDL3Backend::Clear(int color)
{
	float r = ((color >> 16) & 0xFF) / 255.0f;
	float g = ((color >> 8) & 0xFF) / 255.0f;
	float b = (color & 0xFF) / 255.0f;
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void SDL3Backend::BeginLayerDrawing()
{
	CreateLayerRenderTarget(renderTargetWidth, renderTargetHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, layerRenderTarget);
	glViewport(0, 0, renderTargetWidth, renderTargetHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	drawingLayer = true;
}

void SDL3Backend::EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	if (!drawingLayer) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
	glViewport(0, 0, renderTargetWidth, renderTargetHeight);

	bool needsBlendRestore = false;
	if (effect == 9) // Add
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
		needsBlendRestore = true;
	}
	else if (effect == 11) // Subtract
	{
		glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		needsBlendRestore = true;
	}

	ApplyEffectParameters(effectInstance, renderTargetWidth, renderTargetHeight, rgbCoefficient, effect, effectParameter, layerRenderTargetTexture);
	RenderQuad(0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	if (needsBlendRestore)
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}

	drawingLayer = false;
}

GLuint SDL3Backend::CompileShader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader compilation error: " << infoLog << std::endl;
		return 0;
	}
	return shader;
}

GLuint SDL3Backend::CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
	
	if (vertexShader == 0 || fragmentShader == 0) {
		return 0;
	}
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader link error: " << infoLog << std::endl;
		return 0;
	}
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	
	return program;
}

std::string SDL3Backend::LoadShaderSource(const std::string& filename) {
	std::vector<uint8_t> data = pakFile.GetData(filename);
	if (data.empty()) {
		std::cerr << "Failed to load shader: " << filename  << ". Loading default shader..." << std::endl;
		return LoadShaderSource("shaders/standard/normal.frag");
	}
	return std::string(reinterpret_cast<char*>(data.data()), data.size());
}

void SDL3Backend::CreateStandardShaders() {
	std::string vertexSrc = LoadShaderSource("shaders/standard/default.vert");
	if (vertexSrc.empty()) {
		std::cerr << "Failed to load default vertex shader" << std::endl;
		return;
	}
	
	// guh
	const char* effectFiles[STANDARD_EFFECT_COUNT] = {
		"shaders/standard/normal.frag",
		nullptr,
		"shaders/standard/inverted.frag",
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		"shaders/standard/monochrome.frag",
		nullptr,
		nullptr
	};
	
	for (int i = 0; i < STANDARD_EFFECT_COUNT; i++) {
		// fallback the normal shader
		const char* fragFile = effectFiles[i] ? effectFiles[i] : effectFiles[0];
		std::string fragSrc = LoadShaderSource(fragFile);
		if (fragSrc.empty()) {
			std::cerr << "Failed to load fragment shader: " << fragFile << std::endl;
			continue;
		}
		
		effectShaders[i].program = CreateShaderProgram(vertexSrc.c_str(), fragSrc.c_str());
		if (effectShaders[i].program == 0) {
			std::cerr << "Failed to create effect shader " << i << std::endl;
			continue;
		}
		
		effectShaders[i].mvpLoc = glGetUniformLocation(effectShaders[i].program, "uMVP");
		effectShaders[i].texLoc = glGetUniformLocation(effectShaders[i].program, "uTexture");
		effectShaders[i].colorLoc = glGetUniformLocation(effectShaders[i].program, "uColor");
	}
	
	// load color shaders for shapes and shit
	std::string colorFragSrc = LoadShaderSource("shaders/standard/color.frag");
	if (colorFragSrc.empty()) {
		std::cerr << "Failed to load color shaders" << std::endl;
		return;
	}
	
	colorShaderProgram = CreateShaderProgram(vertexSrc.c_str(), colorFragSrc.c_str());
	if (colorShaderProgram == 0) {
		std::cerr << "Failed to create color shader program" << std::endl;
		return;
	}
	
	colorShaderMVPLoc = glGetUniformLocation(colorShaderProgram, "uMVP");
	colorShaderColorLoc = glGetUniformLocation(colorShaderProgram, "uColor");
	colorShaderCircleClipLoc = glGetUniformLocation(colorShaderProgram, "circleClip");

	std::string textureQuickbackdropFragSrc = LoadShaderSource("shaders/standard/normal_quickbackdrop.frag");
	if (textureQuickbackdropFragSrc.empty()) {
		std::cerr << "Failed to load textured quick backdrop shader" << std::endl;
		return;
	}
	
	textureQuickbackdropShaderProgram = CreateShaderProgram(vertexSrc.c_str(), textureQuickbackdropFragSrc.c_str());
	if (textureQuickbackdropShaderProgram == 0) {
		std::cerr << "Failed to create textured quick backdrop shader program" << std::endl;
		return;
	}
	textureQuickbackdropShaderMVPLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uMVP");
	textureQuickbackdropShaderTextureLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uTexture");
	textureQuickbackdropShaderColorLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uColor");
	textureQuickbackdropShaderCircleClipLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "circleClip");
	textureQuickbackdropShaderTileScaleLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uTileScale");

	std::string gradientFragSrc = LoadShaderSource("shaders/standard/gradient.frag");
	if (gradientFragSrc.empty()) {
		std::cerr << "Failed to load gradient shader" << std::endl;
		return;
	}

	gradientShaderProgram = CreateShaderProgram(vertexSrc.c_str(), gradientFragSrc.c_str());
	if (gradientShaderProgram == 0) {
		std::cerr << "Failed to create gradient shader program" << std::endl;
		return;
	}

	gradientShaderMVPLoc = glGetUniformLocation(gradientShaderProgram, "uMVP");
	gradientShaderColor1Loc = glGetUniformLocation(gradientShaderProgram, "uColor1");
	gradientShaderColor2Loc = glGetUniformLocation(gradientShaderProgram, "uColor2");
	gradientShaderVerticalLoc = glGetUniformLocation(gradientShaderProgram, "uVertical");
	gradientShaderCircleClipLoc = glGetUniformLocation(gradientShaderProgram, "circleClip");
}

void SDL3Backend::UseEffectShader(int effect) {
	if (effect < 0 || effect >= STANDARD_EFFECT_COUNT) {
		effect = 0;
	}
	
	if (currentEffect == effect && currentEffect >= 0) {
		return;
	}
	
	currentEffect = effect;
	glUseProgram(effectShaders[effect].program);
}

void SDL3Backend::CreateRenderTarget(int width, int height) {
	if (renderTarget != 0) {
		glDeleteFramebuffers(1, &renderTarget);
		glDeleteTextures(1, &renderTargetTexture);
	}
	
	renderTargetWidth = width;
	renderTargetHeight = height;
	
	glGenFramebuffers(1, &renderTarget);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
	
	glGenTextures(1, &renderTargetTexture);
	glBindTexture(GL_TEXTURE_2D, renderTargetTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	
	if (Application::Instance().GetAppData()->GetAntiAliasingWhenResizing()) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetTexture, 0);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer is not complete!" << std::endl;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SDL3Backend::CreateLayerRenderTarget(int width, int height) {
	if (layerRenderTarget != 0 && layerRenderTargetTexture != 0 &&
		layerRenderTargetWidth == width && layerRenderTargetHeight == height) {
		return;
	}

	if (layerRenderTarget != 0) {
		glDeleteFramebuffers(1, &layerRenderTarget);
		layerRenderTarget = 0;
	}

	if (layerRenderTargetTexture != 0) {
		glDeleteTextures(1, &layerRenderTargetTexture);
		layerRenderTargetTexture = 0;
	}

	layerRenderTargetWidth = width;
	layerRenderTargetHeight = height;

	glGenFramebuffers(1, &layerRenderTarget);
	glBindFramebuffer(GL_FRAMEBUFFER, layerRenderTarget);

	glGenTextures(1, &layerRenderTargetTexture);
	glBindTexture(GL_TEXTURE_2D, layerRenderTargetTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, layerRenderTargetTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Layer framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SDL3Backend::SetOrthoProjection(GLuint program, GLint mvpLoc, float width, float height) {
	float mvp[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
}

void SDL3Backend::ApplyEffectParameters(EffectInstance* effectInstance, int textureWidth, int textureHeight, int rgbCoefficient, int effect, unsigned char effectParameter, GLuint textureId) {
	float r = ((rgbCoefficient >> 16) & 0xFF) / 255.0f;
	float g = ((rgbCoefficient >> 8) & 0xFF) / 255.0f;
	float b = (rgbCoefficient & 0xFF) / 255.0f;
	float a = (255 - effectParameter) / 255.0f;

	GLint texLoc = -1;
	GLint colorLoc = -1;
	GLuint program = 0;

	if (effectInstance != nullptr) {
		EffectShader* shader = LoadShader(effectInstance->filename);
		if (shader != nullptr) {
			glUseProgram(shader->program);
			currentEffect = -1;
			program = shader->program;
			texLoc = shader->texLoc;
			colorLoc = shader->colorLoc;

			GLint pixelWidthLoc = glGetUniformLocation(shader->program, "fPixelWidth");
			GLint pixelHeightLoc = glGetUniformLocation(shader->program, "fPixelHeight");
			if (pixelWidthLoc >= 0) glUniform1f(pixelWidthLoc, 1.0f / textureWidth);
			if (pixelHeightLoc >= 0) glUniform1f(pixelHeightLoc, 1.0f / textureHeight);

			for (auto& param : effectInstance->Parameters) {
				GLint loc = glGetUniformLocation(shader->program, param.Name.c_str());
				if (loc < 0) continue;
				if (param.Type == 0) { // Int
					glUniform1i(loc, std::any_cast<int>(param.Value));
				} else if (param.Type == 1) { // Float
					glUniform1f(loc, std::any_cast<float>(param.Value));
				} else if (param.Type == 2) { // Color
					int c = std::any_cast<int>(param.Value);
					float pr = (c & 0xFF) / 255.0f;
					float pg = ((c >> 8) & 0xFF) / 255.0f;
					float pb = ((c >> 16) & 0xFF) / 255.0f;
					glUniform4f(loc, pr, pg, pb, 1.0f);
				}
			}
		}
	}

	if (program == 0) {
		UseEffectShader(effect);
		EffectShader& shader = effectShaders[effect];
		program = shader.program;
		texLoc = shader.texLoc;
		colorLoc = shader.colorLoc;
	}

	if (textureId != 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);
		if (texLoc >= 0) glUniform1i(texLoc, 0);
		if (colorLoc >= 0) glUniform4f(colorLoc, r, g, b, a);
	}
}

void SDL3Backend::RenderQuad(float x, float y, float w, float h, float angle, float pivotX, float pivotY, float u0, float v0, float u1, float v1) {
	float rad = angle * (3.14159265358979323846f / 180.0f);
	float cosA = cosf(rad);
	float sinA = sinf(rad);
	
	// Translate to position, rotate around pivot, scale
	float transform[16] = {
		w * cosA, w * sinA, 0.0f, 0.0f,
		-h * sinA, h * cosA, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		x + pivotX * (1 - cosA) + pivotY * sinA,
		y + pivotY * (1 - cosA) - pivotX * sinA,
		0.0f, 1.0f
	};
	
	float orthoW = static_cast<float>(renderTargetWidth);
	float orthoH = static_cast<float>(renderTargetHeight);
	
	float mvp[16] = {
		2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
		2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
	};
	
	GLint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	
	GLint mvpLoc = -1;
	// check effect shaders
	if (currentEffect >= 0 && currentEffect < STANDARD_EFFECT_COUNT && 
		static_cast<GLuint>(currentProgram) == effectShaders[currentEffect].program) {
		mvpLoc = effectShaders[currentEffect].mvpLoc;
	} else if (static_cast<GLuint>(currentProgram) == colorShaderProgram) {
		mvpLoc = colorShaderMVPLoc;
	} else if (static_cast<GLuint>(currentProgram) == textureQuickbackdropShaderProgram) {
		mvpLoc = textureQuickbackdropShaderMVPLoc;
	} else if (static_cast<GLuint>(currentProgram) == gradientShaderProgram) {
		mvpLoc = gradientShaderMVPLoc;
	} else {
		for (auto& pair : thirdPartyShaders) {
			if (pair.second.program == static_cast<GLuint>(currentProgram)) {
				mvpLoc = pair.second.mvpLoc;
				break;
			}
		}
	}
	
	if (mvpLoc != -1) {
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
	}

	// update uv coords for this draw
	float verts[] = {
		0.0f, 0.0f, u0, v0,
		1.0f, 0.0f, u1, v0,
		1.0f, 1.0f, u1, v1,
		0.0f, 0.0f, u0, v0,
		1.0f, 1.0f, u1, v1,
		0.0f, 1.0f, u0, v1
	};
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void SDL3Backend::LoadTexture(int id) {
	//check if texture is already loaded
	if (textures.find(id) != textures.end()) {
		return;
	}

	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo) {
		std::cerr << "ImageBank::GetImage Error: " << "Image with id " << id << " not found" << std::endl;
		return;
	}

	char imageFileName[32];
	std::snprintf(imageFileName, sizeof(imageFileName), "images/%d.png", id);
	
	std::vector<uint8_t> data = pakFile.GetData(imageFileName);
	if (data.empty()) {
		std::cerr << "PakFile::GetData Error: " << "Image " << imageFileName << " not found" << std::endl;
		return;
	}

	SDL_IOStream* stream = SDL_IOFromMem(data.data(), data.size());
	SDL_Surface* surface = IMG_Load_IO(stream, true);
	if (surface == nullptr) {
		std::cerr << "IMG_Load_IO Error: " << SDL_GetError() << std::endl;
		return;
	}
	
	SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surface);
	if (rgbaSurface == nullptr) {
		std::cerr << "SDL_ConvertSurface Error: " << SDL_GetError() << std::endl;
		return;
	}
	
	GLTexture texture;
	glGenTextures(1, &texture.textureId);
	glBindTexture(GL_TEXTURE_2D, texture.textureId);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);
	
	texture.width = rgbaSurface->w;
	texture.height = rgbaSurface->h;
	
	SDL_DestroySurface(rgbaSurface);
	
	textures[id] = texture;
}

void SDL3Backend::UnloadTexture(int id) {
	auto it = textures.find(id);
	if (it == textures.end()) {
		return;
	}
	
	if (it->second.textureId != 0) {
		glDeleteTextures(1, &it->second.textureId);
	}

	textures.erase(it);
}

void SDL3Backend::DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo) {
		return;
	}
	
	auto texIt = textures.find(id);
	if (texIt == textures.end()) {
		return;
	}
	
	GLTexture& texture = texIt->second;

	// semitransparent doesn't have rgb coeff
	if (effect == 1) color = 0xFFFFFF;

	bool needsBlendRestore = false;
	
	if (effect == 9) // Add
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
		needsBlendRestore = true;
	}
	else if (effect == 11) // Subtract
	{
		glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		needsBlendRestore = true;
	}

	ApplyEffectParameters(effectInstance, texture.width, texture.height, color, effect, effectParameter, texture.textureId);
	
	float drawX = static_cast<float>(x) - (static_cast<float>(offsetX) * scaleX);
	float drawY = static_cast<float>(y) - (static_cast<float>(offsetY) * scaleY);
	float width = static_cast<float>(imageInfo->Width) * scaleX;
	float height = static_cast<float>(imageInfo->Height) * scaleY;
	float drawAngle = static_cast<float>(360 - angle);
	
	RenderQuad(drawX, drawY, width, height, drawAngle, static_cast<float>(offsetX) * scaleX, static_cast<float>(offsetY) * scaleY);
	
	if (needsBlendRestore) {
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
}

EffectShader* SDL3Backend::LoadShader(const std::string& hash)
{
	auto it = thirdPartyShaders.find(hash);
	if (it != thirdPartyShaders.end()) {
		return &it->second;
	}

	std::string fragSrc = LoadShaderSource("shaders/thirdparty/" + hash + ".frag");
	if (fragSrc.empty()) {
		return nullptr;
	}

	return LoadShader(hash, fragSrc);
}

EffectShader* SDL3Backend::LoadShader(const std::string& name, const std::string& fragSrc)
{
	auto it = thirdPartyShaders.find(name);
	if (it != thirdPartyShaders.end()) {
		return &it->second;
	}

	std::string vertSrc = LoadShaderSource("shaders/standard/default.vert");
	if (vertSrc.empty()) {
		return nullptr;
	}

	GLuint program = CreateShaderProgram(vertSrc.c_str(), fragSrc.c_str());
	if (program == 0) {
		std::cerr << "Failed to create third party shader: " << name << std::endl;
		return nullptr;
	}

	EffectShader shader;
	shader.program = program;
	shader.mvpLoc = glGetUniformLocation(program, "uMVP");
	shader.texLoc = glGetUniformLocation(program, "uTexture");
	shader.colorLoc = glGetUniformLocation(program, "uColor");

	auto inserted = thirdPartyShaders.emplace(name, shader);
	return &inserted.first->second;
}

void SDL3Backend::DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape)
{
	if (shape->ShapeType == 1) { // Line
		int x1 = shape->FlipX ? width - 1 : 0;
		int y1 = shape->FlipY ? height - 1 : 0;
		int x2 = shape->FlipX ? 0 : width - 1;
		int y2 = shape->FlipY ? 0 : height - 1;

		//Windows DX9 runtime doesn't support border width to my knowledge
		Bitmap line(width, height);
		line.DrawLine(x1, y1, x2, y2, shape->BorderColor | 0xFF000000);
		DrawBitmap(line, x, y);
	}
	else {
		if (shape->FillType == 1) { // Solid Color
			glUseProgram(colorShaderProgram);
			currentEffect = -1;
			glUniform4f(colorShaderColorLoc, ((shape->Color1 >> 16) & 0xFF) / 255.0f, ((shape->Color1 >> 8) & 0xFF) / 255.0f, (shape->Color1 & 0xFF) / 255.0f, 1.0f);
			glUniform1i(colorShaderCircleClipLoc, shape->ShapeType == 3);
			RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
		}
		else if (shape->FillType == 2) { // Gradient
			float r1 = ((shape->Color1 >> 16) & 0xFF) / 255.0f;
			float g1 = ((shape->Color1 >> 8) & 0xFF) / 255.0f;
			float b1 = (shape->Color1 & 0xFF) / 255.0f;
			
			float r2 = ((shape->Color2 >> 16) & 0xFF) / 255.0f;
			float g2 = ((shape->Color2 >> 8) & 0xFF) / 255.0f;
			float b2 = (shape->Color2 & 0xFF) / 255.0f;

			glUseProgram(gradientShaderProgram);
			currentEffect = -1;
			glUniform4f(gradientShaderColor1Loc, r1, g1, b1, 1.0f);
			glUniform4f(gradientShaderColor2Loc, r2, g2, b2, 1.0f);
			glUniform1i(gradientShaderVerticalLoc, shape->VerticalGradient ? 1 : 0);
			glUniform1i(gradientShaderCircleClipLoc, shape->ShapeType == 3);
			RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
		}
		else if (shape->FillType == 3) { // Motif 
			auto texIt = textures.find(shape->Image);
			if (texIt == textures.end()) {
				return;
			}
			
			GLTexture& texture = texIt->second;			

			glUseProgram(textureQuickbackdropShaderProgram);
			currentEffect = -1;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture.textureId);
			glUniform1i(textureQuickbackdropShaderTextureLoc, 0);
			glUniform4f(textureQuickbackdropShaderColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform1i(textureQuickbackdropShaderCircleClipLoc, shape->ShapeType == 3);
			int tw = std::max(1, texture.width);
			int th = std::max(1, texture.height);
			glUniform2f(textureQuickbackdropShaderTileScaleLoc,
				static_cast<float>(width) / static_cast<float>(tw),
				static_cast<float>(height) / static_cast<float>(th));
			RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
		}

		if (shape->BorderSize > 0) {
			Bitmap border(width, height);
			for (int i = 0; i < shape->BorderSize; i++) {
				int innerW = width - i * 2;
				int innerH = height - i * 2;
				if (innerW <= 0 || innerH <= 0) {
					break;
				}
				if (shape->ShapeType == 2) {
					border.DrawRectangleLines(i, i, innerW, innerH, shape->BorderColor | 0xFF000000);
				}
				else if (shape->ShapeType == 3 && shape->FillType > 1) { // On Windows DX9, only draw ellipse lines if the fill type is a gradient or motif
					border.DrawEllipseLines(i, i, innerW, innerH, shape->BorderColor | 0xFF000000);
				}
			}
			DrawBitmap(border, x, y);
		}
	}
}

void SDL3Backend::DrawBitmap(Bitmap& bitmap, int x, int y)
{
	if (bitmap.GetWidth() <= 0 || bitmap.GetHeight() <= 0) {
		return;
	}
	
	GLuint tempTexture = 0;
	glGenTextures(1, &tempTexture);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.GetWidth(), bitmap.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap.GetData());

	UseEffectShader(0);
	EffectShader& shader = effectShaders[0];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glUniform1i(shader.texLoc, 0);
	glUniform4f(shader.colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

	RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(bitmap.GetWidth()), static_cast<float>(bitmap.GetHeight()), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	glDeleteTextures(1, &tempTexture);
}

void SDL3Backend::DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	if (width <= 0 || height <= 0) {
		return;
	}

	int srcX = std::max(0, x);
	int srcY = std::max(0, y);
	int srcW = std::min(width, renderTargetWidth - srcX);
	int srcH = std::min(height, renderTargetHeight - srcY);
	if (srcW <= 0 || srcH <= 0) {
		return;
	}

	GLuint copyFramebuffer = drawingLayer ? layerRenderTarget : renderTarget;
	GLuint composedFramebuffer = 0;
	GLuint composedTexture = 0;
	if (drawingLayer) {
		glGenFramebuffers(1, &composedFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, composedFramebuffer);

		glGenTextures(1, &composedTexture);
		glBindTexture(GL_TEXTURE_2D, composedTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetWidth, renderTargetHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, composedTexture, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
			glViewport(0, 0, renderTargetWidth, renderTargetHeight);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			UseEffectShader(0);
			EffectShader& shader = effectShaders[0];
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, renderTargetTexture);
			glUniform1i(shader.texLoc, 0);
			glUniform4f(shader.colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
			RenderQuad(0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, layerRenderTargetTexture);
			glUniform1i(shader.texLoc, 0);
			glUniform4f(shader.colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
			RenderQuad(0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

			copyFramebuffer = composedFramebuffer;
		}
	}

	GLuint tempTexture = 0;
	glGenTextures(1, &tempTexture);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, srcW, srcH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindFramebuffer(GL_FRAMEBUFFER, copyFramebuffer);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcX, renderTargetHeight - (srcY + srcH), srcW, srcH);
	glBindFramebuffer(GL_FRAMEBUFFER, drawingLayer ? layerRenderTarget : renderTarget);
	glViewport(0, 0, renderTargetWidth, renderTargetHeight);

	ApplyEffectParameters(effectInstance, srcW, srcH, rgbCoefficient, effect, effectParameter, tempTexture);

	RenderQuad(static_cast<float>(srcX), static_cast<float>(srcY), static_cast<float>(srcW), static_cast<float>(srcH), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	
	glDeleteTextures(1, &tempTexture);
	if (composedFramebuffer != 0) {
		glDeleteFramebuffers(1, &composedFramebuffer);
	}
	if (composedTexture != 0) {
		glDeleteTextures(1, &composedTexture);
	}
}

void SDL3Backend::LoadFont(int id)
{
	//check if font already exists
	if (fonts.find(id) != fonts.end()) {
		return;
	}

	//get font info
	FontInfo* fontInfo = FontBank::Instance().GetFont(id);
	if (fontInfo == nullptr) {
		std::cerr << "FontBank::GetFont Error: " << "Font with id " << id << " not found" << std::endl;
		return;
	}

	SDL_IOStream* stream;

	//if buffer is already loaded, use it
	if (fontBuffers.find(fontInfo->FontFileName) != fontBuffers.end()) {
		stream = SDL_IOFromMem(fontBuffers[fontInfo->FontFileName]->data(), fontBuffers[fontInfo->FontFileName]->size());
	}
	else {
		//load buffer from pak file
		std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(pakFile.GetData("fonts/" + fontInfo->FontFileName));
		if (buffer->empty()) {
			std::cerr << "PakFile::GetData Error: " << "Font with file name " << fontInfo->FontFileName << " not found" << std::endl;
			return;
		}
		stream = SDL_IOFromMem(buffer->data(), buffer->size());
		fontBuffers[fontInfo->FontFileName] = buffer;
	}

	TTF_Font* font = TTF_OpenFontIO(stream, true, static_cast<float>(fontInfo->Height));
	if (!font) {
		std::cerr << "TTF_OpenFontIO Error: " << SDL_GetError() << std::endl;
		return;
	}
	
	//render flags
	int renderFlags = TTF_STYLE_NORMAL;
	if (fontInfo->Weight > 400) {
		renderFlags |= TTF_STYLE_BOLD;
	}
	if (fontInfo->Italic) {
		renderFlags |= TTF_STYLE_ITALIC;
	}
	if (fontInfo->Underline) {
		renderFlags |= TTF_STYLE_UNDERLINE;
	}
	if (fontInfo->Strikeout) {
		renderFlags |= TTF_STYLE_STRIKETHROUGH;
	}	

	TTF_SetFontStyle(font, renderFlags);

	fonts[id] = font;
}

void SDL3Backend::UnloadFont(int id)
{
	auto it = fonts.find(id);
	if (it != fonts.end()) {
		// Find the FontInfo associated with this font id to remove buffer
		FontInfo* fontInfo = FontBank::Instance().GetFont(id);
		if (fontInfo != nullptr) {
			// Check if any other loaded font is using the same buffer
			bool bufferUsedByOtherFont = false;
			for (const auto& pair : fonts) {
				if (pair.first != id) {
					FontInfo* otherFontInfo = FontBank::Instance().GetFont(pair.first);
					if (otherFontInfo && otherFontInfo->FontFileName == fontInfo->FontFileName) {
						bufferUsedByOtherFont = true;
						break;
					}
				}
			}
			if (!bufferUsedByOtherFont) {
				fontBuffers.erase(fontInfo->FontFileName);
			}
		}
		
		ClearTextCacheForFont(id);
		
		TTF_CloseFont(it->second);
		fonts.erase(it);
	}
}

void SDL3Backend::DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	if (fontInfo == nullptr) {
		return;
	}
	
	if (fonts.find(fontInfo->Handle) == fonts.end()) {
		return;
	}

	TTF_Font* font = fonts[fontInfo->Handle];
	if (font == nullptr) {
		return;
	}

	TextCacheKey cacheKey{ fontInfo->Handle, text, color, objectHandle };
	auto cacheIt = textCache.find(cacheKey);
	
	GLTexture texture;
	int width = 0;
	int height = 0;
	
	if (cacheIt != textCache.end()) {
		texture = cacheIt->second.texture;
		width = cacheIt->second.width;
		height = cacheIt->second.height;
	} else {
		//something changed in the text, clear texture cache for this object
		if (objectHandle != -1) {
			auto it = textCache.begin();
			while (it != textCache.end()) {
				if (it->first.objectHandle == objectHandle) {
					if (it->second.texture.textureId != 0) {
						glDeleteTextures(1, &it->second.texture.textureId);
					}
					it = textCache.erase(it);
				} else {
					++it;
				}
			}
		}

		//remove \r from text
		std::string modifiedText = text;
		modifiedText.erase(std::remove(modifiedText.begin(), modifiedText.end(), '\r'), modifiedText.end());

		//make tabs 4 spaces
		for (size_t i = 0; i < modifiedText.size(); i++) {
			if (modifiedText[i] == '\t') {
				modifiedText.replace(i, 1, "    ");
			}
		}

		//Check if text is empty/just whitespace
		if (modifiedText.find_first_not_of(" \n\r\t") == std::string::npos) {
			return;
		}
		
		SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(font, modifiedText.c_str(), 0, RGBToSDLColor(color), 0);
		if (surface == nullptr) {
			std::cerr << "TTF_RenderText_Blended_Wrapped Error: " << SDL_GetError() << std::endl;
			return;
		}

		SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		if (rgbaSurface == nullptr) {
			std::cerr << "SDL_ConvertSurface Error: " << SDL_GetError() << std::endl;
			return;
		}

		glGenTextures(1, &texture.textureId);
		glBindTexture(GL_TEXTURE_2D, texture.textureId);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);
		
		texture.width = rgbaSurface->w;
		texture.height = rgbaSurface->h;
		width = rgbaSurface->w;
		height = rgbaSurface->h;
		
		SDL_DestroySurface(rgbaSurface);
		
		if (textCache.size() >= 256) {
			RemoveOldTextCache();
		}
		
		//cache the texture
		CachedText cached;
		cached.texture = texture;
		cached.width = width;
		cached.height = height;
		textCache[cacheKey] = cached;
	}

	ApplyEffectParameters(effectInstance, width, height, rgbCoefficient, effect, effectParameter, texture.textureId);
	
	RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
}

void SDL3Backend::RemoveOldTextCache()
{
	if (textCache.empty()) {
		return;
	}

	auto oldestIt = textCache.begin();
	if (oldestIt->second.texture.textureId != 0) {
		glDeleteTextures(1, &oldestIt->second.texture.textureId);
	}
	textCache.erase(oldestIt);
}

void SDL3Backend::ClearTextCacheForFont(int fontHandle)
{
	auto it = textCache.begin();
	while (it != textCache.end()) {
		if (it->first.fontHandle == fontHandle) {
			if (it->second.texture.textureId != 0) {
				glDeleteTextures(1, &it->second.texture.textureId);
			}
			it = textCache.erase(it);
		} else {
			++it;
		}
	}
}

// Sample Start

bool SDL3Backend::LoadSample(int id, int channel) {
	std::cout << "Loading Sample : " << id << "\n";
	if (id < 0) return false;
	if (channels[channel].data) {
		std::cout << "Sample already loaded, returning true.\n";
		return true;
	}
	SoundInfo* soundInfo = SoundBank::Instance().GetSound(id);
	if (!soundInfo) {
		std::cerr << "SoundBank Error: Sound ID " << id << " not found!" << std::endl;
		return false;
	}
	std::cout << soundInfo->Type << "\n";
	std::vector<uint8_t> data = pakFile.GetData("sounds/" + std::to_string(id) + "." + soundInfo->Type);
	if (data.empty()) {
		std::cerr << "PakFile::GetData Error: Sample with id " << id << " not found" << std::endl;
		return false;
	}
	if (soundInfo->Type == "wav") {
		SDL_IOStream* stream = SDL_IOFromMem(data.data(), data.size());
		if (!SDL_LoadWAV_IO(stream, true, &channels[channel].spec, &channels[channel].data, &channels[channel].data_len)) {
			std::cout << "SDL_LoadWAV_IO Error (WAV) : " << SDL_GetError() << std::endl;
			return false;
		}
		std::cout << "Loaded WAV Sample ID : " << id << "\n";
	}
	else if (soundInfo->Type == "ogg") {
		int channels, samplerate;
		short* output = nullptr;
		int numSamples = stb_vorbis_decode_memory(data.data(), data.size(), &channels, &samplerate, &output);
		if (numSamples <= 0 || !output) {
			std::cout << "stb_vorbis_decode_memory failed.\n";
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
		std::cout << "Loaded OGG Sample ID : " << id << "\n";
	}
	else if (soundInfo->Type == "mp3") {
		drmp3 mp3;
		if (!drmp3_init_memory(&mp3, data.data(), data.size(), NULL)) {
			std::cout << "Failed to decode mp3 data.\n";
			drmp3_uninit(&mp3);
			return false;
		}
		drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);
		if (frameCount == 0) {
			std::cout << "No sample frames in MP3\n";
			drmp3_uninit(&mp3);
			return false;
		}
		int totalSamples = static_cast<int>(frameCount * mp3.channels);
		Uint32 dataLen = totalSamples * sizeof(int16_t);
		channels[channel].data = (Uint8*)SDL_malloc(dataLen);
		drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(&mp3, frameCount, (drmp3_int16*)channels[channel].data);
		if (!channels[channel].data) {
			std::cout << "Bad MP3 Data\n";
			SDL_free(channels[channel].data);
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
		std::cout << "Audio Data Type" << soundInfo->Type << "not supported.\n";
		return false;
	}
	channels[channel].name = soundInfo->Name;
	return true;
}
bool SDL3Backend::LoadSampleFile(std::string path) {
	std::cout << "Loading Sample File : " << path << "\n";
	std::filesystem::path fullPath = path;
	std::string type = fullPath.extension().string();
	SampleFile sampleFile;
	if (type == ".wav") {
		if (!SDL_LoadWAV(path.c_str(), &sampleFile.spec, &sampleFile.data, &sampleFile.data_len)) {
			std::cout << "Failed to load WAV file : " << SDL_GetError() << "\n";
			return false;
		}
		std::cout << "Loaded WAV Sample File : " << path << "\n";
	}
	else if (type == ".ogg") {
		int channels, samplerate;
		short* output = nullptr;
		int numSamples = stb_vorbis_decode_filename(path.c_str(), &channels, &samplerate, &output);
		if (numSamples <= 0 || !output) {
			std::cout << "Failed to load OGG file : " << path << "\n";
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
		std::cout << "Loaded OGG file : " << path << "\n";
	}
	else {
		std::cout << "Audio File" << type << "not supported.\n";
		return false;
	}
	sampleFile.pathName = path;
	sampleFiles.emplace(sampleFile.pathName, sampleFile);
	return true;
}
int SDL3Backend::FindSample(std::string name) {
	SoundInfo* soundInfo = SoundBank::Instance().GetSoundName(name);
	if (soundInfo) {
		if (soundInfo->Name == name) return soundInfo->Handle;
	}
	else std::cout << "Failed to find Sound " << name << "\n";
	return -1;
}

void SDL3Backend::PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) {
	bool replaceSample = false;
	bool channelsFilled = false;
	bool channelFound = false;
	if (channel < 1 || channel >= SDL_arraysize(channels)) {
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (!channels[i].stream || !channels[i].data || !channels[i].lock) {
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
		std::cerr << "SDL_CreateAudioStream error : " << SDL_GetError() << "\n";
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
	
	std::cout << "Sample ID " << id << " is now playing at channel " << channel << ".\n";
}
void SDL3Backend::PlaySampleFile(std::string path, int channel, int loops) {
	auto it = sampleFiles.find(path);
	if (it == sampleFiles.end()) {
		std::cout << "Can't find sample path.\n";
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
void SDL3Backend::DiscardSampleFile(std::string path) {
	auto it = sampleFiles.find(path);
	if (it == sampleFiles.end()) {
		std::cout << "Can't find sample path.\n";
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

bool SDL3Backend::SampleState(int id, bool channel, bool pause) {
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
void SDL3Backend::PauseSample(int id, bool channel, bool pause) {
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
void SDL3Backend::SetSamplePan(float pan, int id, bool channel) {
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
int SDL3Backend::GetSamplePan(int id, bool channel) {
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
void SDL3Backend::SetSamplePos(int pos, int id, bool channel)
{
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
void SDL3Backend::SetSampleVolume(float volume, int id, bool channel) {
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
void SDL3Backend::SetSampleFreq(int freq, int id, bool channel) {
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
int SDL3Backend::GetSampleFreq(int id, bool channel) {
	if (channel) { // Get Channel Freq
		if (id < 1 || id >= SDL_arraysize(channels)) return 0;
		return channels[id].spec.freq * SDL_GetAudioStreamFrequencyRatio(channels[id].stream);
	}
	if (id > -1 && !channel) {
		for (int i = 1; i < SDL_arraysize(channels); ++i) {
			if (channels[i].curHandle == id) return channels[i].spec.freq * SDL_GetAudioStreamFrequencyRatio(channels[id].stream);
		}
	}
	return 0;
}
int SDL3Backend::GetSampleVolume(int id) {
	if (id == -1) return mainVol;
	if (id > -1) { // Get Sample Volume
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			if (channels[i].curHandle == id && channels[i].stream) return channels[i].volume;
		}
	}
	return 0;
}

int SDL3Backend::GetSampleVolume(std::string name) {
	return GetSampleVolume(FindSample(name));
}

int SDL3Backend::GetChannelVolume(int id) {
	if (id < 1 || id >= SDL_arraysize(channels)) return -1;
	return channels[id].volume;
}

void SDL3Backend::StopSample(int id, bool channel) {
	if (id == -1) { // Stop any sample
		for (int i = 1; i < SDL_arraysize(channels); i++) {
			StopSample(i, true);
		}
		return;
	}
	if (channel) { // check for the channel
		if (id < 1 || id >= SDL_arraysize(channels)) return;
		if (channels[id].stream) {
			std::cout << "Stopping Sample : " << id << "\n";
			SDL_UnbindAudioStream(channels[id].stream);
			SDL_DestroyAudioStream(channels[id].stream);
			channels[id].stream = nullptr;
			if (channels[id].data) {
				SDL_free(channels[id].data);
			}
			channels[id].data = nullptr;
			channels[id].data_len = 0;
		}
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
void SDL3Backend::UpdateSample() {
	if (!Application::Instance().GetAppData()->GetSampleFocus()) {
		if (windowFocused) SDL_SetAudioDeviceGain(audio_device, 1.0f);
		else SDL_SetAudioDeviceGain(audio_device, 0.0f);
	}
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
// Sample End
void SDL3Backend::GetKeyboardState(uint8_t* outBuffer)
{
#ifdef _DEBUG
	if (DEBUG_UI.IsEnabled() && ImGui::GetIO().WantCaptureKeyboard) return;
#endif

	//return the keyboard state in a new array which matches the Fusion key codes
	const bool* keyboardState = SDL_GetKeyboardState(nullptr);
	for (int i = 0; i < 256; i++)
	{
		outBuffer[i] = keyboardState[FusionToSDLKey(i)] ? 1 : 0;
	}
}

SDL_FRect SDL3Backend::CalculateRenderTargetRect()
{
	// get actual current window size
	int currentWindowWidth, currentWindowHeight;
	SDL_GetWindowSize(window, &currentWindowWidth, &currentWindowHeight);
	
	// get app size
	int renderTargetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	int renderTargetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);

	SDL_FRect rect = { 0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight) };

	if (Application::Instance().GetAppData()->GetResizeDisplay()) {
		rect.w = static_cast<float>(currentWindowWidth);
		rect.h = static_cast<float>(currentWindowHeight);

		if (Application::Instance().GetAppData()->GetFitInside()) {
			//keeps the aspect ratio of the application and fits inside the window while staying in the center
			float aspectRatio = static_cast<float>(renderTargetWidth) / static_cast<float>(renderTargetHeight);
			if (rect.w / rect.h > aspectRatio) {
				rect.w = rect.h * aspectRatio;
			}
			else {
				rect.h = rect.w / aspectRatio;
			}
			rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
			rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
		}
	}
	else if (!Application::Instance().GetAppData()->GetDontCenterFrame()) {
		rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
		rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
	}
	
	return rect;
}

int SDL3Backend::GetMouseX()
{
	float x;
#ifndef PLATFORM_WEB
	int windowX;
	SDL_GetWindowPosition(window, &windowX, NULL);
	SDL_GetGlobalMouseState(&x, NULL);
	float mouseX = x - windowX;
#else
	float mouseX;
	SDL_GetMouseState(&mouseX, NULL);
#endif
	
	//get mouse position relative to render target
	SDL_FRect rect = CalculateRenderTargetRect();
	int windowWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	float relativeX = (mouseX - rect.x) * (windowWidth / rect.w);
	return static_cast<int>(relativeX);
}

int SDL3Backend::GetMouseY()
{
	float y;
#ifndef PLATFORM_WEB
	int windowY;
	SDL_GetWindowPosition(window, NULL, &windowY);
	SDL_GetGlobalMouseState(NULL, &y);
	float mouseY = y - windowY;
#else
	float mouseY;
	SDL_GetMouseState(NULL, &mouseY);
#endif

	//get mouse position relative to render target
	SDL_FRect rect = CalculateRenderTargetRect();
	int windowHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);
	float relativeY = (mouseY - rect.y) * (windowHeight / rect.h);
	return static_cast<int>(relativeY);
}

void SDL3Backend::SetMouseX(int x)
{
	SDL_FRect rect = CalculateRenderTargetRect();
	int renderTargetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	
	float windowX = rect.x + (x * rect.w / renderTargetWidth);
	float windowY;
	
	SDL_GetMouseState(NULL, &windowY);
	SDL_WarpMouseInWindow(window, windowX, windowY);
}

void SDL3Backend::SetMouseY(int y)
{
	SDL_FRect rect = CalculateRenderTargetRect();
	int renderTargetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);
	
	float windowX;
	float windowY = rect.y + (y * rect.h / renderTargetHeight);
	
	SDL_GetMouseState(&windowX, NULL);
	SDL_WarpMouseInWindow(window, windowX, windowY);
}

int SDL3Backend::GetMouseWheelMove()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_MOUSE_WHEEL) {
			return event.wheel.y;
		}
	}
	return 0;
}

uint32_t SDL3Backend::GetMouseState()
{
#ifdef _DEBUG
	if (DEBUG_UI.IsEnabled() && ImGui::GetIO().WantCaptureMouse) return 0;
#endif

	return SDL_GetMouseState(nullptr, nullptr);
}

void SDL3Backend::HideMouseCursor()
{
	SDL_HideCursor();
}

void SDL3Backend::ShowMouseCursor()
{
	SDL_ShowCursor();
}

SDL_Color SDL3Backend::RGBToSDLColor(int color)
{
	return SDL_Color{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		255
	};
}

SDL_Color SDL3Backend::RGBAToSDLColor(int color)
{
	return SDL_Color{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		static_cast<Uint8>((color >> 24) & 0xFF)
	};
}

int SDL3Backend::FusionToSDLKey(short key)
{
	switch (key)
	{
		default:
			return SDL_SCANCODE_UNKNOWN;
		case 0x08:
			return SDL_SCANCODE_BACKSPACE;
		case 0x09:
			return SDL_SCANCODE_TAB;
		case 0x0D:
			return SDL_SCANCODE_RETURN;
		case 0x10:
			return SDL_SCANCODE_LSHIFT;
		case 0x11:
			return SDL_SCANCODE_LCTRL;
		case 0x13:
			return SDL_SCANCODE_PAUSE;
		case 0x14:
			return SDL_SCANCODE_CAPSLOCK;
		case 0x1B:
			return SDL_SCANCODE_ESCAPE;
		case 0x20:
			return SDL_SCANCODE_SPACE;
		case 0x21:
			return SDL_SCANCODE_PAGEUP;
		case 0x22:
			return SDL_SCANCODE_PAGEDOWN;
		case 0x23:
			return SDL_SCANCODE_END;
		case 0x24:
			return SDL_SCANCODE_HOME;
		case 0x25:
			return SDL_SCANCODE_LEFT;
		case 0x26:
			return SDL_SCANCODE_UP;
		case 0x27:
			return SDL_SCANCODE_RIGHT;
		case 0x28:
			return SDL_SCANCODE_DOWN;
		case 0x2D:
			return SDL_SCANCODE_INSERT;
		case 0x2E:
			return SDL_SCANCODE_DELETE;
		case 0x30:
			return SDL_SCANCODE_0;
		case 0x31:
			return SDL_SCANCODE_1;
		case 0x32:
			return SDL_SCANCODE_2;
		case 0x33:
			return SDL_SCANCODE_3;
		case 0x34:
			return SDL_SCANCODE_4;
		case 0x35:
			return SDL_SCANCODE_5;
		case 0x36:
			return SDL_SCANCODE_6;
		case 0x37:
			return SDL_SCANCODE_7;
		case 0x38:
			return SDL_SCANCODE_8;
		case 0x39:
			return SDL_SCANCODE_9;
		case 0x41:
			return SDL_SCANCODE_A;
		case 0x42:
			return SDL_SCANCODE_B;
		case 0x43:
			return SDL_SCANCODE_C;
		case 0x44:
			return SDL_SCANCODE_D;
		case 0x45:
			return SDL_SCANCODE_E;
		case 0x46:
			return SDL_SCANCODE_F;
		case 0x47:
			return SDL_SCANCODE_G;
		case 0x48:
			return SDL_SCANCODE_H;
		case 0x49:
			return SDL_SCANCODE_I;
		case 0x4A:
			return SDL_SCANCODE_J;
		case 0x4B:
			return SDL_SCANCODE_K;
		case 0x4C:
			return SDL_SCANCODE_L;
		case 0x4D:
			return SDL_SCANCODE_M;
		case 0x4E:
			return SDL_SCANCODE_N;
		case 0x4F:
			return SDL_SCANCODE_O;
		case 0x50:
			return SDL_SCANCODE_P;
		case 0x51:
			return SDL_SCANCODE_Q;
		case 0x52:
			return SDL_SCANCODE_R;
		case 0x53:
			return SDL_SCANCODE_S;
		case 0x54:
			return SDL_SCANCODE_T;
		case 0x55:
			return SDL_SCANCODE_U;
		case 0x56:
			return SDL_SCANCODE_V;
		case 0x57:
			return SDL_SCANCODE_W;
		case 0x58:
			return SDL_SCANCODE_X;
		case 0x59:
			return SDL_SCANCODE_Y;
		case 0x5A:
			return SDL_SCANCODE_Z;
		case 0x60:
			return SDL_SCANCODE_KP_0;
		case 0x61:
			return SDL_SCANCODE_KP_1;
		case 0x62:
			return SDL_SCANCODE_KP_2;
		case 0x63:
			return SDL_SCANCODE_KP_3;
		case 0x64:
			return SDL_SCANCODE_KP_4;
		case 0x65:
			return SDL_SCANCODE_KP_5;
		case 0x66:
			return SDL_SCANCODE_KP_6;
		case 0x67:
			return SDL_SCANCODE_KP_7;
		case 0x68:
			return SDL_SCANCODE_KP_8;
		case 0x69:
			return SDL_SCANCODE_KP_9;
		case 0x6A:
			return SDL_SCANCODE_KP_MULTIPLY;
		case 0x6B:
			return SDL_SCANCODE_KP_PLUS;
		case 0x6D:
			return SDL_SCANCODE_KP_MINUS;
		case 0x6E:
			return SDL_SCANCODE_KP_PERIOD;
		case 0x6F:
			return SDL_SCANCODE_KP_DIVIDE;
		case 0x70:
			return SDL_SCANCODE_F1;
		case 0x71:
			return SDL_SCANCODE_F2;
		case 0x72:
			return SDL_SCANCODE_F3;
		case 0x73:
			return SDL_SCANCODE_F4;
		case 0x74:
			return SDL_SCANCODE_F5;
		case 0x75:
			return SDL_SCANCODE_F6;
		case 0x76:
			return SDL_SCANCODE_F7;
		case 0x77:
			return SDL_SCANCODE_F8;
		case 0x78:
			return SDL_SCANCODE_F9;
		case 0x79:
			return SDL_SCANCODE_F10;
		case 0x7A:
			return SDL_SCANCODE_F11;
		case 0x7B:
			return SDL_SCANCODE_F12;
		case 0x90:
			return SDL_SCANCODE_NUMLOCKCLEAR;
		case 0xBA:
			return SDL_SCANCODE_SEMICOLON;
		case 0xBB:
			return SDL_SCANCODE_EQUALS;
		case 0xBC:
			return SDL_SCANCODE_COMMA;
		case 0xBD:
			return SDL_SCANCODE_MINUS;
		case 0xBE:
			return SDL_SCANCODE_PERIOD;
		case 0xBF:
			return SDL_SCANCODE_SLASH;
		case 0xC0:
			return SDL_SCANCODE_GRAVE;
		case 0xDB:
			return SDL_SCANCODE_LEFTBRACKET;
		case 0xDC:
			return SDL_SCANCODE_BACKSLASH;
		case 0xDD:
			return SDL_SCANCODE_RIGHTBRACKET;
		case 0xDE:
			return SDL_SCANCODE_APOSTROPHE;
	}
}

float SDL3Backend::GetTimeDelta()
{
	static Uint32 previousTicks = SDL_GetTicks();
	Uint32 currentTicks = SDL_GetTicks();
	float delta = (currentTicks - previousTicks) / 1000.0f;
	previousTicks = currentTicks;
	return delta;
}

void SDL3Backend::Delay(unsigned int ms)
{
	SDL_Delay(ms);
}
#endif