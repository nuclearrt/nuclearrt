#include "GeneratedFrame{{ FRAME_INDEX }}.h"

#include "Application.h"
#include "AudioBackend.h"
#include "GameState.h"
#include "Input.h"
#include "ObjectFactory.h"
#include "MathHelper.h"
#include "PlatformBackend.h"

void GeneratedFrame{{ FRAME_INDEX }}::Initialize() {
	Frame::Initialize();

	Index = {{ FRAME_INDEX }};
	Name = "{{ FRAME_NAME }}";

	Width = {{ FRAME_WIDTH }};
	Height = {{ FRAME_HEIGHT }};
	BackgroundColor = {{ FRAME_BACKGROUND_COLOR }};

	{{ LAYER_INIT }}

	auto& factory = ObjectFactory::Instance();
	{{ OBJECT_INSTANCES }}

	{{ OBJECT_SELECTORS_INIT }}

	{{ GROUP_ACTIVE }}

	Frame::PostInitialize();
}

void GeneratedFrame{{ FRAME_INDEX }}::Update() {
	Frame::Update();

	//Animation events
	{{ EVENT_ANIMATION_UPDATE_LOOP }}

	//Timer events
	{{ EVENT_TIMER_UPDATE_LOOP }}

	//Normal events
	{{ EVENT_NORMAL_UPDATE_LOOP }}

	Frame::DeleteMarkedInstances();
}

void GeneratedFrame{{ FRAME_INDEX }}::Draw() {
	Frame::Draw();
}

{{ EVENT_FUNCTIONS }}