#include "DebugUI.h"

#ifdef _DEBUG

#include <SDL3/SDL.h>
#include <cstdio>
#include <chrono>

// Dear ImGui includes
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

void DebugUI::Initialize(SDL_Window* window, void* glContext) {
	if (initialized) {
		return;
	}

	this->window = window;
	this->glContext = glContext;

	IMGUI_CHECKVERSION();
	context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui_ImplSDL3_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	initialized = true;
}

void DebugUI::Shutdown() {
	if (!initialized) {
		return;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
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

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void DebugUI::EndFrame() {
	if (!initialized || !enabled) {
		return;
	}

	RenderWindows();
	RenderMetrics();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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