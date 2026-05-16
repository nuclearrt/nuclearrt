#ifdef NUCLEAR_BACKEND_SDL3

#include "SDL3GraphicsBackend.h"

#include <iostream>
#include "Application.h"
#include "SDL3Backend.h"
#include "Frame.h"
#include "FontBank.h"
#include "ImageBank.h"
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#ifdef NUCLEAR_DEBUG_UI
#include "DebugUI.h"
#include "imgui.h"
#include <imgui_impl_sdl3.h>
#endif

void SDL3GraphicsBackend::Initialize() {
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	std::string windowTitle = Application::Instance().GetAppData()->GetAppName();
	
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
		backend->GetPlatform()->Log("SDL_Init Error: " + std::string(SDL_GetError()));
		return;
	}

	if (!TTF_Init()) {
		backend->GetPlatform()->Log("TTF_Init Error: " + std::string(SDL_GetError()));
		return;
	}

	// Create the window
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
	window = SDL_CreateWindow(windowTitle.c_str(), windowWidth, windowHeight, flags);
	if (window == nullptr) {
		backend->GetPlatform()->Log("SDL_CreateWindow Error: " + std::string(SDL_GetError()));
		return;
	}
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		backend->GetPlatform()->Log("SDL_GL_CreateContext Error: " + std::string(SDL_GetError()));
		return;
	}
		
	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1);

	
	#if !defined(PLATFORM_MACOS) && !defined(PLATFORM_WEB)
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::string errorStr = reinterpret_cast<const char*>(glewGetErrorString(glewErr));
		backend->GetPlatform()->Log("GLEW Init Error: " + errorStr);
		return;
	}
	#endif
	
	glEnable(GL_BLEND);

	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

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

#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.Initialize(window, glContext);
	
	DEBUG_UI.AddWindow(Application::Instance().GetAppData()->GetAppName(), [this]() {
		ImGui::Text("Platform: %s", backend->platform ? backend->platform->GetPlatformName().c_str() : "");
		ImGui::Text("Assets directory: %s", backend->platform ? backend->platform->GetAssetsDirectory().c_str() : "");

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

void SDL3GraphicsBackend::Deinitialize()
{
#ifdef NUCLEAR_DEBUG_UI
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
}

void SDL3GraphicsBackend::BeginDrawing()
{
	if (glContext == nullptr) {
		backend->GetPlatform()->Log("BeginDrawing called with null renderer!");
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

#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.BeginFrame();
#endif
}

void SDL3GraphicsBackend::EndDrawing()
{
	if (glContext == nullptr) {
		backend->GetPlatform()->Log("EndDrawing called with null renderer!");
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

#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.EndFrame();
#endif

	SDL_GL_SwapWindow(window);

	if (!renderedFirstFrame) {
		renderedFirstFrame = true;
		SDL_ShowWindow(window);
	}
}

void SDL3GraphicsBackend::Clear(int color)
{
	float r = ((color >> 16) & 0xFF) / 255.0f;
	float g = ((color >> 8) & 0xFF) / 255.0f;
	float b = (color & 0xFF) / 255.0f;
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void SDL3GraphicsBackend::BeginLayerDrawing()
{
	CreateLayerRenderTarget(renderTargetWidth, renderTargetHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, layerRenderTarget);
	glViewport(0, 0, renderTargetWidth, renderTargetHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	drawingLayer = true;
}

void SDL3GraphicsBackend::EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
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

GLuint SDL3GraphicsBackend::CompileShader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		backend->GetPlatform()->Log("Shader compilation error: " + std::string(infoLog));
		return 0;
	}
	return shader;
}

GLuint SDL3GraphicsBackend::CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
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
		backend->GetPlatform()->Log("Shader link error: " + std::string(infoLog));
		return 0;
	}
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	
	return program;
}

std::string SDL3GraphicsBackend::LoadShaderSource(const std::string& filename) {
	if (!backend) return "";
	std::vector<uint8_t> data = backend->GetPlatform()->GetPakFile().GetData(filename);
	if (data.empty()) {
		backend->GetPlatform()->Log("Failed to load shader: " + filename  + ". Loading default shader...");
		return LoadShaderSource("shaders/standard/normal.frag");
	}
	return std::string(reinterpret_cast<char*>(data.data()), data.size());
}

void SDL3GraphicsBackend::CreateStandardShaders() {
	std::string vertexSrc = LoadShaderSource("shaders/standard/default.vert");
	if (vertexSrc.empty()) {
		backend->GetPlatform()->Log("Failed to load default vertex shader");
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
		nullptr, // "shaders/standard/add.frag" is supposed to be in the pak file, but it's not so it's very funny. -indeednotfunny
		"shaders/standard/monochrome.frag",
		nullptr, // same here with subtract.frag -- "shaders/standard/subtract.frag" -indeednotfunny
		nullptr
	};
	
	for (int i = 0; i < STANDARD_EFFECT_COUNT; i++) {
		// fallback the normal shader
		const char* fragFile = effectFiles[i] ? effectFiles[i] : effectFiles[0];
		std::string fragSrc = LoadShaderSource(fragFile);
		if (fragSrc.empty()) {
			backend->GetPlatform()->Log("Failed to load fragment shader: " + std::string(fragFile));
			continue;
		}
		
		effectShaders[i].program = CreateShaderProgram(vertexSrc.c_str(), fragSrc.c_str());
		if (effectShaders[i].program == 0) {
			backend->GetPlatform()->Log("Failed to create effect shader " + std::to_string(i));
			continue;
		}
		
		effectShaders[i].mvpLoc = glGetUniformLocation(effectShaders[i].program, "uMVP");
		effectShaders[i].texLoc = glGetUniformLocation(effectShaders[i].program, "uTexture");
		effectShaders[i].colorLoc = glGetUniformLocation(effectShaders[i].program, "uColor");
	}
	
	// load color shaders for shapes and shit
	std::string colorFragSrc = LoadShaderSource("shaders/standard/color.frag");
	if (colorFragSrc.empty()) {
		backend->GetPlatform()->Log("Failed to load color shaders");
		return;
	}
	
	colorShaderProgram = CreateShaderProgram(vertexSrc.c_str(), colorFragSrc.c_str());
	if (colorShaderProgram == 0) {
		backend->GetPlatform()->Log("Failed to create color shader program");
		return;
	}
	
	colorShaderMVPLoc = glGetUniformLocation(colorShaderProgram, "uMVP");
	colorShaderColorLoc = glGetUniformLocation(colorShaderProgram, "uColor");
	colorShaderCircleClipLoc = glGetUniformLocation(colorShaderProgram, "circleClip");

	std::string textureQuickbackdropFragSrc = LoadShaderSource("shaders/standard/normal_quickbackdrop.frag");
	if (textureQuickbackdropFragSrc.empty()) {
		backend->GetPlatform()->Log("Failed to load textured quick backdrop shader");
		return;
	}
	
	textureQuickbackdropShaderProgram = CreateShaderProgram(vertexSrc.c_str(), textureQuickbackdropFragSrc.c_str());
	if (textureQuickbackdropShaderProgram == 0) {
		backend->GetPlatform()->Log("Failed to create textured quick backdrop shader program");
		return;
	}
	textureQuickbackdropShaderMVPLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uMVP");
	textureQuickbackdropShaderTextureLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uTexture");
	textureQuickbackdropShaderColorLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uColor");
	textureQuickbackdropShaderCircleClipLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "circleClip");
	textureQuickbackdropShaderTileScaleLoc = glGetUniformLocation(textureQuickbackdropShaderProgram, "uTileScale");

	std::string gradientFragSrc = LoadShaderSource("shaders/standard/gradient.frag");
	if (gradientFragSrc.empty()) {
		backend->GetPlatform()->Log("Failed to load gradient shader");
		return;
	}

	gradientShaderProgram = CreateShaderProgram(vertexSrc.c_str(), gradientFragSrc.c_str());
	if (gradientShaderProgram == 0) {
		backend->GetPlatform()->Log("Failed to create gradient shader program");
		return;
	}

	gradientShaderMVPLoc = glGetUniformLocation(gradientShaderProgram, "uMVP");
	gradientShaderColor1Loc = glGetUniformLocation(gradientShaderProgram, "uColor1");
	gradientShaderColor2Loc = glGetUniformLocation(gradientShaderProgram, "uColor2");
	gradientShaderVerticalLoc = glGetUniformLocation(gradientShaderProgram, "uVertical");
	gradientShaderCircleClipLoc = glGetUniformLocation(gradientShaderProgram, "circleClip");
}

void SDL3GraphicsBackend::UseEffectShader(int effect) {
	if (effect < 0 || effect >= STANDARD_EFFECT_COUNT) {
		effect = 0;
	}
	
	if (currentEffect == effect && currentEffect >= 0) {
		return;
	}
	
	currentEffect = effect;
	glUseProgram(effectShaders[effect].program);
}

void SDL3GraphicsBackend::CreateRenderTarget(int width, int height) {
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
		backend->GetPlatform()->Log("Framebuffer is not complete!");
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SDL3GraphicsBackend::CreateLayerRenderTarget(int width, int height) {
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
		backend->GetPlatform()->Log("Layer framebuffer is not complete!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void SDL3GraphicsBackend::SetOrthoProjection(GLuint program, GLint mvpLoc, float width, float height) {
	float mvp[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
}

void SDL3GraphicsBackend::ApplyEffectParameters(EffectInstance* effectInstance, int textureWidth, int textureHeight, int rgbCoefficient, int effect, unsigned char effectParameter, GLuint textureId) {
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

void SDL3GraphicsBackend::RenderQuad(float x, float y, float w, float h, float angle, float pivotX, float pivotY, float u0, float v0, float u1, float v1) {
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

void SDL3GraphicsBackend::LoadTexture(int id) {
	//check if texture is already loaded
	if (textures.find(id) != textures.end()) {
		return;
	}

	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo) {
		backend->GetPlatform()->Log("ImageBank::GetImage Error: Image with id " + std::to_string(id) + " not found");
		return;
	}

	char imageFileName[32];
	std::snprintf(imageFileName, sizeof(imageFileName), "images/%d.png", id);
	
	if (!backend->platform) return;
	std::vector<uint8_t> data = backend->GetPlatform()->GetPakFile().GetData(imageFileName);
	if (data.empty()) {
		backend->GetPlatform()->Log("PakFile::GetData Error: Image " + std::string(imageFileName) + " not found");
		return;
	}

	SDL_IOStream* stream = SDL_IOFromMem(data.data(), data.size());
	SDL_Surface* surface = IMG_Load_IO(stream, true);
	if (surface == nullptr) {
		backend->GetPlatform()->Log("IMG_Load_IO Error: " + std::string(SDL_GetError()));
		return;
	}
	
	SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surface);
	if (rgbaSurface == nullptr) {
		backend->GetPlatform()->Log("SDL_ConvertSurface Error: " + std::string(SDL_GetError()));
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

void SDL3GraphicsBackend::UnloadTexture(int id) {
	auto it = textures.find(id);
	if (it == textures.end()) {
		return;
	}
	
	if (it->second.textureId != 0) {
		glDeleteTextures(1, &it->second.textureId);
	}

	textures.erase(it);
}

void SDL3GraphicsBackend::DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
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


EffectShader* SDL3GraphicsBackend::LoadShader(const std::string& hash)
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

EffectShader* SDL3GraphicsBackend::LoadShader(const std::string& name, const std::string& fragSrc)
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
		backend->GetPlatform()->Log("Failed to create third party shader: " + name);
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

void SDL3GraphicsBackend::DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape)
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

void SDL3GraphicsBackend::DrawBitmap(Bitmap& bitmap, int x, int y)
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.GetWidth(), bitmap.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetData());

	UseEffectShader(0);
	EffectShader& shader = effectShaders[0];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glUniform1i(shader.texLoc, 0);
	glUniform4f(shader.colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

	RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(bitmap.GetWidth()), static_cast<float>(bitmap.GetHeight()), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	glDeleteTextures(1, &tempTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void SDL3GraphicsBackend::DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
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

void SDL3GraphicsBackend::LoadFont(int id)
{
	//check if font already exists
	if (fonts.find(id) != fonts.end()) {
		return;
	}

	//get font info
	FontInfo* fontInfo = FontBank::Instance().GetFont(id);
	if (fontInfo == nullptr) {
		backend->GetPlatform()->Log("FontBank::GetFont Error: Font with id " + std::to_string(id) + " not found");
		return;
	}

	SDL_IOStream* stream;

	//if buffer is already loaded, use it
	if (fontBuffers.find(fontInfo->FontFileName) != fontBuffers.end()) {
		stream = SDL_IOFromMem(fontBuffers[fontInfo->FontFileName]->data(), fontBuffers[fontInfo->FontFileName]->size());
	}
	else {
		//load buffer from pak file
		if (!backend->platform) return;
		std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(backend->platform->GetPakFile().GetData("fonts/" + fontInfo->FontFileName));
		if (buffer->empty()) {
			backend->GetPlatform()->Log("PakFile::GetData Error: Font with file name " + fontInfo->FontFileName + " not found");
			return;
		}
		stream = SDL_IOFromMem(buffer->data(), buffer->size());
		fontBuffers[fontInfo->FontFileName] = buffer;
	}

	TTF_Font* font = TTF_OpenFontIO(stream, true, static_cast<float>(fontInfo->Height));
	if (!font) {
		backend->GetPlatform()->Log("TTF_OpenFontIO Error: " + std::string(SDL_GetError()));
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

void SDL3GraphicsBackend::UnloadFont(int id)
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

void SDL3GraphicsBackend::DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
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
			backend->GetPlatform()->Log("TTF_RenderText_Blended_Wrapped Error: " + std::string(SDL_GetError()));
			return;
		}

		SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		if (rgbaSurface == nullptr) {
			backend->GetPlatform()->Log("SDL_ConvertSurface Error: " + std::string(SDL_GetError()));
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

void SDL3GraphicsBackend::RemoveOldTextCache()
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

void SDL3GraphicsBackend::ClearTextCacheForFont(int fontHandle)
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

SDL_Color SDL3GraphicsBackend::RGBToSDLColor(int color)
{
	return SDL_Color{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		255
	};
}

SDL_Color SDL3GraphicsBackend::RGBAToSDLColor(int color)
{
	return SDL_Color{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		static_cast<Uint8>((color >> 24) & 0xFF)
	};
}

SDL_FRect SDL3GraphicsBackend::CalculateRenderTargetRect()
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
#endif