#include "DebugUI.h"

#ifdef NUCLEAR_DEBUG_UI

#include <cstdio>
#include <chrono>

#include "imgui.h"

#if defined(NUCLEAR_BACKEND_SDL3)
#include <SDL3/SDL.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#elif defined(NUCLEAR_BACKEND_SDL2)
#include <SDL.h>
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#endif

void DebugUI::Initialize(SDL_Window* window, void* renderContext) {
	if (initialized) {
		return;
	}

	this->window = window;
	this->renderContext = renderContext;

	IMGUI_CHECKVERSION();
	context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	//TODO: this init stuff should be done in the backend
	//plus the actual debug window should just be created here.

#if defined(NUCLEAR_BACKEND_SDL3)
	ImGui_ImplSDL3_InitForOpenGL(window, renderContext);
	ImGui_ImplOpenGL3_Init("#version 330");
#elif defined(NUCLEAR_BACKEND_SDL2)
	ImGui_ImplSDL2_InitForSDLRenderer(window, (SDL_Renderer*)renderContext);
	ImGui_ImplSDLRenderer2_Init((SDL_Renderer*)renderContext);
#endif

	initialized = true;
}

void DebugUI::Shutdown() {
	if (!initialized) {
		return;
	}

#if defined(NUCLEAR_BACKEND_SDL3)
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
#elif defined(NUCLEAR_BACKEND_SDL2)
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
#endif

	ImGui::DestroyContext(context);
	context = nullptr;

	initialized = false;
}

void DebugUI::BeginFrame() {
	if (!initialized || !enabled) {
		return;
	}

	static auto lastFrameTime = std::chrono::high_resolution_clock::now();
	auto currentFrameTime = std::chrono::high_resolution_clock::now();
	frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
	lastFrameTime = currentFrameTime;
	
	fps = 1.0f / frameTime;

#if defined(NUCLEAR_BACKEND_SDL3)
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
#elif defined(NUCLEAR_BACKEND_SDL2)
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
#endif

	ImGui::NewFrame();
}

void DebugUI::EndFrame() {
	if (!initialized || !enabled) {
		return;
	}

	RenderWindows();
	RenderMetrics();

	ImGui::Render();

#if defined(NUCLEAR_BACKEND_SDL3)
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#elif defined(NUCLEAR_BACKEND_SDL2)
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), (SDL_Renderer*)renderContext);
#endif
}

void DebugUI::AddWindow(const std::string& name, std::function<void()> renderFunction) {
	DebugWindow window;
	window.name = name;
	window.renderFunction = renderFunction;
	window.open = true;
	
	windows.push_back(window);
}

void DebugUI::RenderWindows() {
	for (auto& window : windows) {
		if (window.open) {
			if (ImGui::Begin(window.name.c_str(), &window.open)) {
				window.renderFunction();
			}
			ImGui::End();
		}
	}
}

void DebugUI::RenderMetrics() {
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(170, 80), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
	ImGui::Text("Frame Time: %.3f ms", frameTime * 1000.0f);
	ImGui::Text("FPS: %.1f", fps);
	ImGui::End();
}

#endif