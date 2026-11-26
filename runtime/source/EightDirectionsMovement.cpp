#include "EightDirectionsMovement.h"

#include <cmath>

#include "Active.h"
#include "Application.h"

void EightDirectionsMovement::Initialize()
{
	//this should probably be done in base movement class
	SetMovementDirection(DirectionAtStart);

	if (!((Active *)Instance)->AutomaticRotation)
	{
		((Active *)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}

void EightDirectionsMovement::Update(float deltaTime)
{
	int wishX = 0;
	int wishY = 0;

	const float movementBaseTime = deltaTime * 60.0f;

	if (Application::Instance().GetInput()->IsControlsDown(Player, 1)) // up
	{
		wishY -= 1;
	}
	if (Application::Instance().GetInput()->IsControlsDown(Player, 2)) // down
	{
		wishY += 1;
	}
	if (Application::Instance().GetInput()->IsControlsDown(Player, 4)) // left
	{
		wishX -= 1;
	}
	if (Application::Instance().GetInput()->IsControlsDown(Player, 8)) // right
	{
		wishX += 1;
	}

	bool moved = false;
	if (wishX == 0 && wishY < 0 && IsDirectionValid(8, Directions)) // up
	{
		movementDirection = 8;
		moved = true;
	}
	else if (wishX > 0 && wishY < 0 && IsDirectionValid(4, Directions)) // up-right
	{
		movementDirection = 4;
		moved = true;
	}	
	else if (wishX > 0 && wishY == 0 && IsDirectionValid(0, Directions)) // right
	{
		movementDirection = 0;
		moved = true;
	}
	else if (wishX > 0 && wishY > 0 && IsDirectionValid(28, Directions)) // down-right
	{
		movementDirection = 28;
		moved = true;
	}
	else if (wishX == 0 && wishY > 0 && IsDirectionValid(24, Directions)) // down
	{
		movementDirection = 24;
		moved = true;
	}
	else if (wishX < 0 && wishY > 0 && IsDirectionValid(20, Directions)) // down-left
	{
		movementDirection = 20;
		moved = true;
	}
	else if (wishX < 0 && wishY == 0 && IsDirectionValid(16, Directions)) // left
	{
		movementDirection = 16;
		moved = true;
	}
	else if (wishX < 0 && wishY < 0 && IsDirectionValid(12, Directions)) // up-left
	{
		movementDirection = 12;
		moved = true;
	}

	//accelerate
	if (moved)
	{
		realSpeed += static_cast<float>(GetAcceleratorValue(Acceleration)) * movementBaseTime / 256.0f;
		if (realSpeed > Speed)
			realSpeed = Speed;
	}
	else // decelerate
	{
		realSpeed -= static_cast<float>(GetAcceleratorValue(Deceleration)) * movementBaseTime / 256.0f;
		if (realSpeed < 0)
			realSpeed = 0;
	}
	
	const double deltaX = static_cast<double>(GetCosinus32(movementDirection & 31)) * static_cast<int>(realSpeed) * movementBaseTime / 2048.0;
	const double deltaY = static_cast<double>(GetSinus32(movementDirection & 31)) * static_cast<int>(realSpeed) * movementBaseTime / 2048.0;

	const double nextX = static_cast<double>(Instance->GetX().GetIntValue()) + subPixelX + deltaX;
	const double nextY = static_cast<double>(Instance->GetY().GetIntValue()) + subPixelY + deltaY;

	Instance->SetX(static_cast<int>(nextX));
	Instance->SetY(static_cast<int>(nextY));

	subPixelX = static_cast<float>(nextX - static_cast<int>(nextX));
	subPixelY = static_cast<float>(nextY - static_cast<int>(nextY));

	if (!((Active*)Instance)->AutomaticRotation ) {
		((Active*)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}