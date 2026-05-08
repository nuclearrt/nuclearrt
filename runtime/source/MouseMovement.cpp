#include "MouseMovement.h"

#include <algorithm>

#include "Application.h"
#include "InputBackend.h"

void MouseMovement::Initialize() {
	initialX = Instance->X;
	initialY = Instance->Y;
}

void MouseMovement::OnEnabled() {
	Application::Instance().GetBackend()->input->HideMouseCursor();

	disabledCursorX = Application::Instance().GetBackend()->input->GetMouseX();
	disabledCursorY = Application::Instance().GetBackend()->input->GetMouseY();
}

void MouseMovement::OnDisabled() {
	Application::Instance().GetBackend()->input->ShowMouseCursor();
}

void MouseMovement::Update(float deltaTime) {
	int mouseX = Application::Instance().GetInput()->GetMouseX();
	int mouseY = Application::Instance().GetInput()->GetMouseY();

	int xDifference = mouseX - disabledCursorX;
	int yDifference = mouseY - disabledCursorY;
	
	Application::Instance().GetBackend()->input->SetMouseX(disabledCursorX);
	Application::Instance().GetBackend()->input->SetMouseY(disabledCursorY);

	Instance->X += xDifference;
	Instance->Y += yDifference;

	Instance->X = std::clamp(Instance->X - initialX, MinX, MaxX) + initialX;
	Instance->Y = std::clamp(Instance->Y - initialY, MinY, MaxY) + initialY;
}