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
		movementDirection = validDirections[Application::Instance().RandomRange(0, static_cast<short>(validDirections.size() - 1))];
	else //all directions are valid
		movementDirection = Application::Instance().RandomRange(0, 31);

	if (!((Active *)Instance)->AutomaticRotation)
	{
		((Active *)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}

void BulletMovement::Update(float deltaTime)
{
	deltaTime *= 10;

	float xDifference = speed * deltaTime * cos(movementDirection * 3.14159265358979323846f / 16);
	float yDifference = -speed * deltaTime * sin(movementDirection * 3.14159265358979323846f / 16);

	Instance->SetX(Instance->GetX() + xDifference);
	Instance->SetY(Instance->GetY() + yDifference);

	if (!((Active*)Instance)->AutomaticRotation ) {
		((Active*)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}