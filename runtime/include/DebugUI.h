#pragma once

#ifdef _DEBUG

#include <string>
#include <memory>
#include <vector>
#include <functional>

struct ImGuiContext;
struct SDL_Window;

class DebugUI {
public:
	static DebugUI& Instance() {
		static DebugUI instance;
		return instance;
	}

	void Initialize(SDL_Window* window, void* glContext);
	void Shutdown();
	
	void BeginFrame();
	void EndFrame();
	
	bool IsEnabled() const { return enabled; }
	void SetEnabled(bool value) { enabled = value; }
	void ToggleEnabled() { enabled = !enabled; }
	
	void AddWindow(const std::string& name, std::function<void()> renderFunction);
	
private:
	DebugUI() = default;
	~DebugUI() = default;
	DebugUI(const DebugUI&) = delete;
	DebugUI& operator=(const DebugUI&) = delete;

	void RenderWindows();
	void RenderMetrics();
	
	bool enabled = false;
	bool initialized = false;
	
	SDL_Window* window = nullptr;
	void* glContext = nullptr;
	ImGuiContext* context = nullptr;
	
	float frameTime = 0.0f;
	float fps = 0.0f;
	
	struct DebugWindow {
		std::string name;
		std::function<void()> renderFunction;
		bool open = true;
	};
	
	std::vector<DebugWindow> windows;
};

#define DEBUG_UI DebugUI::Instance()

#endif