#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <algorithm>
#include <type_traits>

#include "AppData.h"
#include "Backend.h"
#include "GameState.h"
#include "Input.h"
#include "ObjectGlobalData.h"

class Frame;

class Application
{
public:
	static Application& Instance()
	{
		static Application instance;
		return instance;
	}

	std::shared_ptr<AppData> GetAppData() const { return appData; }
	void SetAppData(std::shared_ptr<AppData> data) { appData = data; }

	std::shared_ptr<Backend> GetBackend() const { return backend; }
    void SetBackend(std::shared_ptr<Backend> b) { backend = b; }

	std::shared_ptr<Input> GetInput() const { return input; }
	void SetInput(std::shared_ptr<Input> i) { input = i; }

	std::unique_ptr<Frame>& GetCurrentFrame() { return currentFrame; }

	void Initialize();

	void Update();

	void Draw();

	void Shutdown();

	void Run();

	void QueueStateChange(GameState newState, int frameIndex = -1);
	GameState GetCurrentState() const { return currentState; }

	short Random(short max);
	short RandomRange(short min, short max);
	bool RandomChance(short chance, short max);

	Application();
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

private:
	std::shared_ptr<AppData> appData;
	std::shared_ptr<Backend> backend;
	std::shared_ptr<Input> input;
	
	std::unique_ptr<Frame> currentFrame;
	void LoadFrame(int frameIndex);

	GameState currentState;
	int newFrameIndex;
	void RunState();

	std::vector<ObjectGlobalData*> globalObjectData;
	void MergeGlobalObjectData(std::vector<ObjectGlobalData*> frameData);
};