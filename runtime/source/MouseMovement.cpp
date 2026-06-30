#include "MouseMovement.h"

#include <algorithm>

#include "Application.h"
#include "InputBackend.h"

void MouseMovement::Initialize() {
	initialX = Instance->GetX();
	initialY = Instance->GetY();
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

	Instance->SetX(Instance->GetX() + xDifference);
	Instance->SetY(Instance->GetY() + yDifference);

	Instance->SetX(std::clamp(Instance->GetX() - initialX, MinX, MaxX) + initialX);
	Instance->SetY(std::clamp(Instance->GetY() - initialY, MinY, MaxY) + initialY);
}