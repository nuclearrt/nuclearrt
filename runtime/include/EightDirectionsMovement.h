#pragma once

#include "Movement.h"
#include <iostream>

class EightDirectionsMovement : public Movement
{
public:
	EightDirectionsMovement(unsigned short player, bool movingAtStart, int directionAtStart, short speed, short acceleration, short deceleration, short bounceFactor, int directions)
		: Movement(player, movingAtStart, directionAtStart), Speed(speed), Acceleration(acceleration), Deceleration(deceleration), BounceFactor(bounceFactor), Directions(directions) {}

	short Speed;
	short Acceleration;
	short Deceleration;
	short BounceFactor;
	int Directions;

	float realSpeed = 0;
	float subPixelX = 0;
	float subPixelY = 0;

	void Initialize() override;
	CValue GetRealSpeed() override { return CValue(realSpeed); }
	int GetMinimumSpeed() override { return 0; }
	int GetMaximumSpeed() override { return static_cast<int>(Speed); }
	int GetMovementDirection() override { return movementDirection; }
	
	void Update(float deltaTime) override;
};