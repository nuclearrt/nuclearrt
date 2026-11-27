#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "CValue.h"
#include "Sequence.h"
#include "ImageBank.h"

class ObjectInstance;

class Animations {
public:
	Animations() = default;
	Animations(std::shared_ptr<const std::vector<Sequence*>> sequences);

	bool IsSequencePlaying(int sequence) const;
	bool IsSequenceOver(int sequence) const;
	bool IsFacingDirectionMask(int directionMask) const;
	bool IsFacingDirection(int directionIndex) const;
	std::vector<unsigned int> GetImagesUsed() const;
	unsigned int GetCurrentImageHandle() const;
	CValue GetCurrentSequenceIndex() const;
	CValue GetCurrentDirection() const;
	CValue GetCurrentFrameIndex() const;
	int GetAutomaticRotationDirection() const;

	int GetXActionPoint() const;
	int GetYActionPoint() const;
	int GetXHotspot() const;
	int GetYHotspot() const;

	int GetWidth() const;
	int GetHeight() const;
	
	void Start();
	void Stop();

	void SetCurrentSequenceIndex(int index);
	void SetCurrentDirectionMask(int directionMask);
	void SetCurrentDirection(int index);

	void SetForcedSpeed(const CValue& speed);
	void SetForcedFrame(const CValue& frame);
	void SetForcedDirection(const CValue& directionMask);

	void RestoreForcedSpeed();
	void RestoreForcedSequence();
	void RestoreForcedDirection();
	void RestoreForcedFrame();

	bool IsDirectionForced() const;

	void SetAnimationSpeed(int speed, int minimumSpeed, int maximumSpeed);

	void Update(float deltaTime);

	void SetOwner(ObjectInstance* owner) { this->owner = owner; }

	bool AutomaticRotation = false;
private:
	ObjectInstance* owner = nullptr;
	std::shared_ptr<const std::vector<Sequence*>> Sequences;
	float CurrentFrameTime = 0.0f;

	int RequestedSequenceIndex = 0;
	int RequestedDirection = 0;

	int CurrentSequenceIndex = 0;
	int CurrentDirection = 0;
	int CurrentFrameIndex = 0;

	bool started = true;

	int forcedSpeed = -1;
	int forcedFrame = -1;
	int forcedDirection = -1;
	int forcedSequence = -1;

	int speed = 0;
	int minimumSpeed = 0;
	int maximumSpeed = 100;
	
	int automaticRotationDirection = -1;

	int lastSequenceOverIndex = -1;
	int sequenceOverThisTick = -1;

	int GetLowestDirectionIndex(const Sequence* sequence) const;
	int GetNearestDirectionIndex(const Sequence* sequence, int directionIndex) const;
	int GetFirstSequenceIndex();
	int ResolveSequenceIndex(int requestedSequenceIndex);
	bool IsTwoSpeedAnimation(int sequenceIndex);
};