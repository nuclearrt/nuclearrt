#include "BouncingBallMovement.h"

#include <cmath>

#include "Active.h"
#include "Application.h"

void BouncingBallMovement::OnEnabled() {
	std::vector<int> directions;
	for (int i = 0; i < 32; i++) {
		if (DirectionAtStart & (1 << i)) {
			directions.push_back(i);
		}
	}

	// direction will just be right if no directions are set
	if (!directions.empty()) {
		movementDirection = directions[Application::Instance().RandomRange(0, static_cast<short>(directions.size() - 1))];
	}
}

void BouncingBallMovement::Update(float deltaTime) {
	deltaTime *= 10;

	realSpeed += speed * deltaTime;
	if (realSpeed > speed)
		realSpeed = speed;
	
	float xDifference = realSpeed * deltaTime * cos(movementDirection * 3.14159265358979323846f / 16);
	float yDifference = -realSpeed * deltaTime * sin(movementDirection * 3.14159265358979323846f / 16);

	Instance->X += xDifference;
	Instance->Y += yDifference;

	if (!((Active*)Instance)->AutomaticRotation ) {
		((Active*)Instance)->animations.SetCurrentDirection(movementDirection);
	}
}