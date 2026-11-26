#include "BulletMovement.h"

#include <cmath>

#include "Active.h"
#include "Application.h"

void BulletMovement::OnEnabled()
{
	std::vector<int> validDirections;
	for (int i = 0; i < 32; i++) {
		if (IsDirectionValid(i, DirectionAtStart)) {
			validDirections.push_back(i);
		}
	}

	if (!validDirections.empty())
		movementDirection = validDirections[Application::Instance().RandomRange(0, static_cast<short>(validDirections.size() - 1)).GetIntValue()];
	else //all directions are valid
		movementDirection = Application::Instance().RandomRange(0, 31).GetIntValue();

	if (!((Active *)Instance)->AutomaticRotation)
	{
		((Active *)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}

void BulletMovement::Update(float deltaTime)
{
	const float movementBaseTime = deltaTime * 60.0f;

	const double deltaX = static_cast<double>(GetCosinus32(movementDirection & 31)) * speed * movementBaseTime / 2048.0;
	const double deltaY = static_cast<double>(GetSinus32(movementDirection & 31)) * speed * movementBaseTime / 2048.0;

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