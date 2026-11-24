#include "Application.h"
#include "FrameFactory.h"
#include "Frame.h"
#include "Transition.h"
#include "GraphicsBackend.h"
#include "AudioBackend.h"
#include "PlatformBackend.h"
#include <set>

#ifdef NUCLEAR_BACKEND_SDL3
#include "SDL3Backend.h"
#endif

#ifdef NUCLEAR_BACKEND_SDL2
#include "SDL2Backend.h"
#endif

#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

Application::Application() = default;
Application::~Application() = default;

void Application::Initialize()
{
	appData = std::make_shared<AppData>();
	appData->Initialize();
	std::cout << "Initialized AppData" << std::endl;

	#ifdef NUCLEAR_BACKEND_SDL3
		backend = std::make_shared<SDL3Backend>();
	#elif NUCLEAR_BACKEND_SDL2
		backend = std::make_shared<SDL2Backend>();
	#else
		backend = std::make_shared<Backend>();
	#endif

	backend->platform->Log("Initialized Backend: " + backend->platform->GetName());

	input = std::make_shared<Input>();
	input->Reset();
	backend->platform->Log("Initialized Input");

	QueueStateChange(GameState::RestartApplication);

	srand(time(0));
}

void Application::Update()
{
	// while a transition runs the frame is frozen
	if (UpdateTransition())
		return;

	RunState();

	if (currentFrame == nullptr || transition != nullptr)
	{
		return;
	}

	input->Update();
	backend->audio->UpdateSample();
	currentFrame->Update();
}

void Application::Draw()
{
	if (currentFrame == nullptr)
	{
		return;
	}

	backend->graphics->BeginDrawing();
	if (transition != nullptr)
	{
		transition->Step(backend->platform->GetTicks());
	}
	else
	{
		currentFrame->Draw();
		if (pendingFadeIn)
		{
			StartFadeIn();
		}
	}
	backend->graphics->EndDrawing();
}

void Application::Shutdown()
{
	backend->platform->Log("Shutting down...");
}

static void Loop()
{
	Application::Instance().Update();
	Application::Instance().Draw();
}

void Application::Run()
{
#ifdef PLATFORM_WEB
	emscripten_set_main_loop(Loop, appData->GetTargetFPS(), 1);
#else
	unsigned int frameStart;
	int frameTime;
	
	while (!backend->platform->ShouldQuit())
	{
		int targetFPS = appData->GetTargetFPS();
		float targetFrameTime = 1000.0f / targetFPS;
		
		frameStart = backend->platform->GetTicks();
		
		Loop();
		
		frameTime = backend->platform->GetTicks() - frameStart;
		
		if (frameTime < targetFrameTime)
		{
			backend->platform->Delay(targetFrameTime - frameTime);
		}

		if ((currentState == GameState::EndApplication && transition == nullptr && fadeOutCompleted) || currentFrame == nullptr)
		{
			break;
		}
	}
#endif

	Shutdown();
}

void Application::QueueStateChange(GameState newState, int frameIndex)
{
	if (newState == GameState::JumpToFrame) 
	{
		if (frameIndex < 0)
		{
			return;
		}
		else if (frameIndex >= FrameFactory::GetFrameCount())
		{
			frameIndex = FrameFactory::GetFrameCount() - 1;
		}
	}

	currentState = newState;
	newFrameIndex = frameIndex;
}

CValue Application::Random(short max)
{
	return CValue(rand() % (max + 1));
}

CValue Application::Random(CValue max)
{
	return CValue(Random(max.GetIntValue()));
}

CValue Application::RandomRange(short min, short max)
{
	return CValue(rand() % (max - min + 1) + min);
}

CValue Application::RandomRange(CValue min, CValue max)
{
	return CValue(RandomRange(min.GetIntValue(), max.GetIntValue()));
}

bool Application::RandomChance(short chance, short max)
{
	if (chance >= max) return true;
	return Random(max) < chance;
}

bool Application::RandomChance(CValue chance, CValue max)
{
	return RandomChance(chance.GetIntValue(), max.GetIntValue());
}

void Application::LoadFrame(int frameIndex)
{
	if (frameIndex < 0)
	{
		frameIndex = 0;
	}	
	backend->platform->Log("Loading frame " + std::to_string(frameIndex));

	hasPreviousFrameImage = false;
	pendingFadeIn = false;
	if (currentFrame != nullptr && backend->graphics->SupportsTransitions())
	{
		backend->graphics->CaptureTransitionSurface(TRANSITION_SURFACE_START);
		hasPreviousFrameImage = true;
	}

	std::vector<unsigned int> oldImagesUsed;
	std::vector<unsigned int> oldFontsUsed;
	std::vector<unsigned int> oldSoundsUsed;
	if (currentFrame != nullptr)
	{
		oldImagesUsed = currentFrame->GetImagesUsed();
		oldFontsUsed = currentFrame->GetFontsUsed();
		oldSoundsUsed = currentFrame->GetSoundsUsed();

		// merge current frame's global object data with existing data
		std::vector<ObjectGlobalData*> frameData = currentFrame->GetGlobalObjectData();
		MergeGlobalObjectData(frameData);
	}

	currentFrame = FrameFactory::CreateFrame(frameIndex);

	if (currentFrame == nullptr)
	{
		QueueStateChange(GameState::EndApplication);
		return;
	}

	currentFrame->Initialize();
	
	// apply global object data to new frame
	if (!globalObjectData.empty())
	{
		currentFrame->ApplyGlobalObjectData(globalObjectData);
	}
	
	std::vector<unsigned int> newImagesUsed = currentFrame->GetImagesUsed();
	std::vector<unsigned int> newFontsUsed = currentFrame->GetFontsUsed();
	std::vector<unsigned int> newSoundsUsed = currentFrame->GetSoundsUsed();

	std::vector<unsigned int> imagesToUnload;
	std::vector<unsigned int> fontsToUnload;
	std::vector<unsigned int> soundsToUnload;

	for (unsigned int image : oldImagesUsed)
	{
		if (std::find(newImagesUsed.begin(), newImagesUsed.end(), image) == newImagesUsed.end())
		{
			imagesToUnload.push_back(image);
		}
	}

	for (unsigned int font : oldFontsUsed)
	{
		if (std::find(newFontsUsed.begin(), newFontsUsed.end(), font) == newFontsUsed.end())
		{
			fontsToUnload.push_back(font);
		}
	}

	for (unsigned int sound : oldSoundsUsed)
	{
		if (std::find(newSoundsUsed.begin(), newSoundsUsed.end(), sound) == newSoundsUsed.end())
		{
			soundsToUnload.push_back(sound);
		}
	}

	//unload the images that are no longer used
	for (unsigned int image : imagesToUnload)
	{
		backend->graphics->UnloadTexture(image);
	}

	//load the images that are new
	for (unsigned int image : newImagesUsed)
	{
		backend->graphics->LoadTexture(image);
	}

	//load the fonts that are new
	for (unsigned int font : newFontsUsed)
	{
		backend->graphics->LoadFont(font);
	}

	//unload the fonts that are no longer used
	for (unsigned int font : fontsToUnload)
	{
		backend->graphics->UnloadFont(font);
	}

	//unload the sounds that are no longer used
	for (unsigned int sound : soundsToUnload)
	{
		backend->audio->UnloadSound(sound);
	}

	//load the sounds that are new
	for (unsigned int sound : newSoundsUsed)
	{
		backend->audio->LoadSound(sound);
	}

	pendingFadeIn = currentFrame->FadeIn != nullptr && backend->graphics->SupportsTransitions();

	backend->platform->Log("Loaded frame " + std::to_string(frameIndex));
	if (!GetAppData()->GetSampleOverFrame()) backend->audio->StopSample(-1, false);
}

void Application::MergeGlobalObjectData(std::vector<ObjectGlobalData*> frameData)
{
	std::map<unsigned int, std::vector<ObjectGlobalData*>> existingByHandle;
	for (auto* data : globalObjectData)
	{
		existingByHandle[data->objectInfoHandle].push_back(data);
	}
	
	std::map<unsigned int, std::vector<ObjectGlobalData*>> frameByHandle;
	for (auto* data : frameData)
	{
		frameByHandle[data->objectInfoHandle].push_back(data);
	}
	
	globalObjectData.clear();
	
	std::set<unsigned int> allHandles;
	for (auto& [handle, dataList] : existingByHandle)
	{
		allHandles.insert(handle);
	}
	for (auto& [handle, dataList] : frameByHandle)
	{
		allHandles.insert(handle);
	}
	
	for (unsigned int handle : allHandles)
	{
		bool hasExisting = existingByHandle.find(handle) != existingByHandle.end();
		bool hasFrame = frameByHandle.find(handle) != frameByHandle.end();
		
		if (hasFrame)
		{
			int frameCount = frameByHandle[handle].size();
			
			if (hasExisting)
			{
				int savedCount = existingByHandle[handle].size();
				
				if (frameCount <= savedCount)
				{
					auto& savedList = existingByHandle[handle];
					auto& frameList = frameByHandle[handle];
					
					for (int i = 0; i < savedCount - frameCount; i++)
					{
						globalObjectData.push_back(savedList[i]);
					}
					for (int i = 0; i < frameCount; i++)
					{
						globalObjectData.push_back(frameList[i]);
					}
				}
				else
				{
					for (auto* data : frameByHandle[handle])
					{
						globalObjectData.push_back(data);
					}
				}
			}
			else
			{
				for (auto* data : frameByHandle[handle])
				{
					globalObjectData.push_back(data);
				}
			}
		}
		else
		{
			for (auto* data : existingByHandle[handle])
			{
				globalObjectData.push_back(data);
			}
		}
	}
}

bool Application::StartFadeOut()
{
	GraphicsBackend* graphics = backend->graphics;
	if (currentFrame == nullptr || currentFrame->FadeOut == nullptr || !graphics->SupportsTransitions())
	{
		return false;
	}

	const TransitionData& data = *currentFrame->FadeOut;
	std::unique_ptr<Transition> fade = Transition::Create(data);
	if (fade == nullptr)
	{
		backend->platform->Log("Unsupported fade-out transition " + data.Id);
		return false;
	}

	// the render target still holds the last drawn image of the frame
	graphics->CaptureTransitionSurface(TRANSITION_SURFACE_START);
	graphics->FillTransitionSurface(TRANSITION_SURFACE_END, (data.Flags & TRANSITION_FLAG_COLOR) ? data.Color : 0x000000);

	int width, height;
	graphics->GetTransitionDisplaySize(width, height);
	fade->Start(data, graphics, true, backend->platform->GetTicks(), width, height);
	transition = std::move(fade);
	return true;
}

void Application::StartFadeIn()
{
	pendingFadeIn = false;

	GraphicsBackend* graphics = backend->graphics;
	if (currentFrame == nullptr || currentFrame->FadeIn == nullptr || !graphics->SupportsTransitions())
	{
		return;
	}

	const TransitionData& data = *currentFrame->FadeIn;
	std::unique_ptr<Transition> fade = Transition::Create(data);
	if (fade == nullptr)
	{
		backend->platform->Log("Unsupported fade-in transition " + data.Id);
		return;
	}

	// the frame has just been drawn once: that is where the transition ends
	graphics->CaptureTransitionSurface(TRANSITION_SURFACE_END);
	if (data.Flags & TRANSITION_FLAG_COLOR)
	{
		graphics->FillTransitionSurface(TRANSITION_SURFACE_START, data.Color);
	}
	else if (!hasPreviousFrameImage)
	{
		graphics->FillTransitionSurface(TRANSITION_SURFACE_START, appData->GetBorderColor());
	}

	int width, height;
	graphics->GetTransitionDisplaySize(width, height);
	unsigned int now = backend->platform->GetTicks();
	fade->Start(data, graphics, false, now, width, height);
	fade->Step(now);
	transition = std::move(fade);
}

bool Application::UpdateTransition()
{
	if (transition == nullptr)
	{
		return false;
	}

	// frame time does not advance during transitions
	backend->platform->GetTimeDelta();
	backend->audio->UpdateSample();

	if (!transition->IsCompleted(backend->platform->GetTicks()))
	{
		return true;
	}

	transition->Finish();
	bool wasFadeOut = transition->IsFadeOut();
	transition.reset();

	// a finished fade-out lets the queued frame change run now
	if (wasFadeOut)
	{
		fadeOutCompleted = true;
	}
	return false;
}

void Application::RunState()
{
	bool changesFrame = currentState != GameState::Running && currentState != GameState::StartOfFrame;
	if (changesFrame && currentFrame != nullptr && !fadeOutCompleted)
	{
		if (StartFadeOut())
		{
			// the state change is carried out once the fade-out has finished
			return;
		}
		fadeOutCompleted = true;
	}

	switch (currentState)
	{
		case GameState::Running:
			break;
		case GameState::RestartApplication:
			fadeOutCompleted = false;
			globalObjectData.clear();
			LoadFrame(0);
			currentState = GameState::StartOfFrame;
			break;
		case GameState::StartOfFrame:
			currentState = GameState::Running;
			break;
		case GameState::NextFrame:
			fadeOutCompleted = false;
			LoadFrame(currentFrame->Index + 1);
			currentState = GameState::StartOfFrame;
			break;
		case GameState::PreviousFrame:
			fadeOutCompleted = false;
			LoadFrame(currentFrame->Index - 1);
			currentState = GameState::StartOfFrame;
			break;
		case GameState::JumpToFrame:
			fadeOutCompleted = false;
			LoadFrame(newFrameIndex);
			currentState = GameState::StartOfFrame;
			break;
		case GameState::RestartFrame:
			fadeOutCompleted = false;
			LoadFrame(currentFrame->Index);
			currentState = GameState::StartOfFrame;
			break;
		case GameState::EndApplication:
			break;
	}
}
