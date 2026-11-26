#pragma once

#include "Movement.h"

class BulletMovement : public Movement
{
public:
	BulletMovement(unsigned short player, bool movingAtStart, int directionAtStart, short speed)
		: Movement(player, movingAtStart, directionAtStart), speed(speed) {}

	short speed;
	bool useInstanceDirection;
	float subPixelX = 0;
	float subPixelY = 0;

	CValue GetRealSpeed() override { return CValue(speed); }
	int GetMinimumSpeed() override { return 0; }
	int GetMaximumSpeed() override { return static_cast<int>(speed); }
	int GetMovementDirection() override { return movementDirection; }
	
	void OnEnabled() override;
	void Update(float deltaTime) override;
};