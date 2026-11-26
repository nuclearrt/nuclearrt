#include "Animations.h"

#include <algorithm>
#include <memory>

#include "Application.h"
#include "Frame.h"
#include "ObjectInstance.h"

Animations::Animations(std::shared_ptr<const std::vector<Sequence*>> sequences) {
	Sequences = std::move(sequences);

	// set current sequence to the one with the lowest index
	if (Sequences && !Sequences->empty()) {
		RequestedSequenceIndex = 0;
		RequestedDirection = 0;
		CurrentSequenceIndex = ResolveSequenceIndex(RequestedSequenceIndex);

		if (CurrentSequenceIndex >= 0 && static_cast<size_t>(CurrentSequenceIndex) < Sequences->size()) {
			if (Sequence* currentSequence = (*Sequences)[CurrentSequenceIndex]) {
				CurrentDirection = GetNearestDirectionIndex(currentSequence, RequestedDirection);
			}
		}
	}
}

bool Animations::IsSequencePlaying(int sequence) const {
	int displayedSequence = forcedSequence != -1 ? forcedSequence : CurrentSequenceIndex;
	return displayedSequence == sequence;
}

bool Animations::IsSequenceOver(int sequence) const {
	return sequenceOverThisTick == sequence;
}

bool Animations::IsFacingDirection(int directionIndex) const
{
	return GetCurrentDirection() == directionIndex;
}

bool Animations::IsFacingDirectionMask(int directionMask) const
{
	if (directionMask == -1) return true;
	return (1 << (GetCurrentDirection().GetIntValue())) & directionMask;
}

std::vector<unsigned int> Animations::GetImagesUsed() const
{
	std::vector<unsigned int> imagesUsed;
	for (const Sequence* sequence : *Sequences) {
		if (sequence == nullptr) continue;
		for (const auto direction : sequence->Directions) {
			if (direction == nullptr) continue;
			for (const auto& frame : direction->Frames) {
				if (std::find(imagesUsed.begin(), imagesUsed.end(), frame) == imagesUsed.end()) {
					imagesUsed.push_back(frame);
				}
			} 
		}
	}
	return imagesUsed;
}

unsigned int Animations::GetCurrentImageHandle() const {
	int displaySequence = forcedSequence != -1 ? forcedSequence : CurrentSequenceIndex;
	int displayDirection = forcedDirection != -1 ? forcedDirection : CurrentDirection;
	int displayFrame = forcedFrame != -1 ? forcedFrame : CurrentFrameIndex;

	auto* sequence = (displaySequence >= 0 && static_cast<size_t>(displaySequence) < Sequences->size()) ? (*Sequences)[displaySequence] : nullptr;
	if (!sequence) {
		sequence = (*Sequences)[CurrentSequenceIndex];
	}

	if (displayDirection < 0 || displayDirection >= Direction::MAX_DIRECTIONS || sequence->Directions[displayDirection] == nullptr || AutomaticRotation) {
		displayDirection = GetNearestDirectionIndex(sequence, displayDirection);
	}
	
	auto* direction = sequence->Directions[displayDirection];
	return direction->Frames.at(displayFrame);
}

CValue Animations::GetCurrentSequenceIndex() const {
	return CValue(forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex);
}

CValue Animations::GetCurrentDirection() const {
	return CValue(forcedDirection != -1 ? forcedDirection : RequestedDirection);
}

CValue Animations::GetCurrentFrameIndex() const {
	return CValue(forcedFrame != -1 ? forcedFrame : CurrentFrameIndex);
}

int Animations::GetAutomaticRotationDirection() const {
	return automaticRotationDirection;
}

int Animations::GetXActionPoint() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->ActionPointX;
}

int Animations::GetYActionPoint() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->ActionPointY;
}

int Animations::GetXHotspot() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->HotspotX;
}

int Animations::GetYHotspot() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->HotspotY;
}

int Animations::GetWidth() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->Width;
}

int Animations::GetHeight() const {
	unsigned int currentImageHandle = GetCurrentImageHandle();
	return ImageBank::Instance().GetImage(currentImageHandle)->Height;
}

void Animations::Start() {
	started = true;
}
void Animations::Stop() {
	started = false;
}

void Animations::SetCurrentSequenceIndex(int index) {
	RequestedSequenceIndex = index;
	lastSequenceOverIndex = -1;
	int resolvedSequenceIndex = ResolveSequenceIndex(RequestedSequenceIndex);
	if (resolvedSequenceIndex == CurrentSequenceIndex) {
		return;
	}

	auto* sequence = (*Sequences)[resolvedSequenceIndex];
	CurrentSequenceIndex = resolvedSequenceIndex;
	CurrentDirection = GetNearestDirectionIndex(sequence, RequestedDirection);
	CurrentFrameIndex = 0;
	CurrentFrameTime = 0.0f;
}

void Animations::SetCurrentDirectionMask(int directionMask)
{
	int validDirections[Direction::MAX_DIRECTIONS];
	int validCount = 0;
	if (directionMask == -1) // set to any valid direction
	{
		for (int i = 0; i < Direction::MAX_DIRECTIONS; ++i)
		{
			validDirections[validCount++] = i;
		}
	}
	else
	{
		for (int i = 0; i < Direction::MAX_DIRECTIONS; ++i)
		{
			if (directionMask & (1 << i))
			{
				validDirections[validCount++] = i;
			}
		}
	}

	if (validCount == 0) return;

	// get a random valid direction from the list
	int index = Application::Instance().RandomRange(0, static_cast<short>(validCount - 1)).GetIntValue();

	SetCurrentDirection(validDirections[index]);
}

void Animations::SetCurrentDirection(int index) {
	if (index == RequestedDirection) {
		return;
	}

	RequestedDirection = index;
	lastSequenceOverIndex = -1;
	auto* currentSequence = (*Sequences)[CurrentSequenceIndex];
	CurrentDirection = GetNearestDirectionIndex(currentSequence, RequestedDirection);
	automaticRotationDirection = CurrentDirection;
	auto* direction = currentSequence->Directions[CurrentDirection];
	if (direction->Frames.empty()) {
		CurrentFrameIndex = 0;
	} else {
		CurrentFrameIndex = std::clamp(CurrentFrameIndex, 0, static_cast<int>(direction->Frames.size() - 1));
	}
}

void Animations::SetForcedFrame(const CValue& frame) {
	if (forcedFrame == frame.GetIntValue()) return;
	
	auto* sequence = (*Sequences)[ResolveSequenceIndex(forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex)];
	auto* direction = sequence->Directions[forcedDirection != -1 ? forcedDirection : GetNearestDirectionIndex(sequence, RequestedDirection)];

	forcedFrame = std::clamp(frame.GetIntValue(), 0, static_cast<int>(direction->Frames.size() - 1));
}

void Animations::SetForcedDirection(const CValue& directionMask) {
	auto* sequence = (*Sequences)[CurrentSequenceIndex];

	int validDirections[Direction::MAX_DIRECTIONS];
	int validCount = 0;
	int mask = directionMask.GetIntValue();
	for (int i = 0; i < Direction::MAX_DIRECTIONS; ++i) {
		if (sequence->Directions[i] != nullptr && (mask == -1 || (mask & (1 << i)))) {
			validDirections[validCount++] = i;
		}
	}

	if (validCount == 0) {
		return;
	}

	//get a random valid direction from the list
	int index = Application::Instance().RandomRange(0, static_cast<short>(validCount - 1)).GetIntValue();
	int newDirection = validDirections[index];
	automaticRotationDirection = newDirection;

	if (forcedDirection == newDirection) return;

	forcedDirection = newDirection;
	RequestedDirection = forcedDirection;
	auto* direction = sequence->Directions[forcedDirection];
	if (direction->Frames.empty()) {
		CurrentFrameIndex = 0;
	} else {
		CurrentFrameIndex = std::clamp(CurrentFrameIndex, 0, static_cast<int>(direction->Frames.size() - 1));
	}
}

void Animations::RestoreForcedSequence() {
	forcedSequence = -1;
}

void Animations::RestoreForcedDirection() {
	forcedDirection = -1;
}

void Animations::RestoreForcedFrame() {
	if (forcedFrame != -1) {
		CurrentFrameIndex = forcedFrame;
	}
	forcedFrame = -1;
}

bool Animations::IsDirectionForced() const {
	return forcedDirection != -1;
}

void Animations::SetAnimationSpeed(int speed, int minimumSpeed, int maximumSpeed)
{
	this->minimumSpeed = std::max(0, minimumSpeed);
	this->maximumSpeed = std::max(this->minimumSpeed, maximumSpeed);
	this->speed = std::clamp(speed, this->minimumSpeed, this->maximumSpeed);
}

void Animations::Update(float deltaTime) {
	sequenceOverThisTick = -1;

	int requestedSequence = forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex;
	int requestedDirection = forcedDirection != -1 ? forcedDirection : RequestedDirection;
	int displaySequence = forcedSequence != -1 ? ResolveSequenceIndex(forcedSequence) : CurrentSequenceIndex;
	int displayDirection = requestedDirection;

	if (forcedDirection != -1) {
		displayDirection = forcedDirection;
	}

	auto* currentSequence = (*Sequences)[displaySequence];

	//if the direction doesn't exist, set to the first direction
	if (displayDirection < 0 || displayDirection >= Direction::MAX_DIRECTIONS || currentSequence->Directions[displayDirection] == nullptr) {
		forcedDirection = -1;
		displayDirection = GetNearestDirectionIndex(currentSequence, displayDirection);
		CurrentDirection = displayDirection;
		CurrentFrameTime = 0.0f;
	}
	else if (displayDirection != CurrentDirection) {
		CurrentDirection = displayDirection;
		CurrentFrameIndex = 0;
		CurrentFrameTime = 0.0f;
	}
	auto* currentDirection = currentSequence->Directions[displayDirection];
	if (currentDirection->Frames.empty()) {
		CurrentFrameIndex = 0;
		return;
	}

	CurrentFrameIndex = std::clamp(CurrentFrameIndex, 0, static_cast<int>(currentDirection->Frames.size() - 1));

	if (forcedFrame != -1)
	{
		return;
	}

	if (!started)
	{
		// If the animation is stopped, we don't update the frame time or index
		return;
	}

	int animMinSpeed = std::clamp(currentDirection->MinimumSpeed, 0, 100);
	int animMaxSpeed = std::clamp(currentDirection->MaximumSpeed, 0, 100);
	if (animMaxSpeed < animMinSpeed) {
		animMaxSpeed = animMinSpeed;
	}
	if (!IsTwoSpeedAnimation(displaySequence)) {
		animMinSpeed = animMaxSpeed;
	}

	int animSpeed = animMinSpeed;
	int animDeltaSpeed = animMaxSpeed - animMinSpeed;
	if (animDeltaSpeed > 0) {
		int movementDelta = maximumSpeed - minimumSpeed;
		if (movementDelta == 0) {
			animSpeed = animMinSpeed + (animDeltaSpeed / 2);
		} else {
			int normalizedSpeed = std::clamp(speed - minimumSpeed, 0, movementDelta);
			animSpeed = animMinSpeed + ((animDeltaSpeed * normalizedSpeed) / movementDelta);
		}
		animSpeed = std::clamp(animSpeed, animMinSpeed, animMaxSpeed);
	}

	if (animSpeed == 0) {
		return;
	}

	CurrentFrameTime += static_cast<float>(animSpeed) * deltaTime * 60.0f;

	while (CurrentFrameTime > 100.0f)
	{
		CurrentFrameTime -= 100.0f;
		CurrentFrameIndex++;

		if (CurrentFrameIndex >= static_cast<int>(currentDirection->Frames.size())) // reached end of animation
		{
			if (currentDirection->Repeat) {
				CurrentFrameIndex = std::clamp(currentDirection->RepeatFrame, 0, static_cast<int>(currentDirection->Frames.size() - 1));
			} else {
				
				// If animation is not the first one (stopped), change to that animation
				int firstSequenceIndex = GetFirstSequenceIndex();
				
				if (displaySequence != firstSequenceIndex) {
					// pick the lowest available seq
					RequestedSequenceIndex = 0;
					CurrentSequenceIndex = ResolveSequenceIndex(RequestedSequenceIndex);
					CurrentDirection = GetNearestDirectionIndex((*Sequences)[CurrentSequenceIndex], RequestedDirection);
					RequestedDirection = CurrentDirection;
					CurrentFrameIndex = 0;
					CurrentFrameTime = 0.0f;

					if (lastSequenceOverIndex != requestedSequence) {
						sequenceOverThisTick = requestedSequence;
						lastSequenceOverIndex = requestedSequence;

						Application::Instance().GetCurrentFrame()->GenerateEvent(2, -2, owner);
					}
				}
				else // if it is the first one, stay on the last frame
				{
					CurrentFrameIndex -= 1;
					if (lastSequenceOverIndex != requestedSequence) {
						sequenceOverThisTick = requestedSequence;
						lastSequenceOverIndex = requestedSequence;
						
						Application::Instance().GetCurrentFrame()->GenerateEvent(2, -2, owner);
					}
				}
			}
		}
	}
}

int Animations::GetLowestDirectionIndex(const Sequence *sequence) const
{
	const auto& dirs = sequence->Directions;
	for (int i = 0; i < static_cast<int>(dirs.size()); ++i) {
		if (dirs[i]) {
			return i;
		}
	}

	return 0;
}

int Animations::GetNearestDirectionIndex(const Sequence *sequence, int directionIndex) const
{
	if (AutomaticRotation)
	{
		directionIndex = 0;
	}

	if (directionIndex >= 0 && directionIndex < Direction::MAX_DIRECTIONS && sequence->Directions[directionIndex] != nullptr)
	{
		return directionIndex;
	}

	int bestDirectionIndex = GetLowestDirectionIndex(sequence);
	int bestDistance = Direction::MAX_DIRECTIONS;
	for (int candidateIndex = 0; candidateIndex < Direction::MAX_DIRECTIONS; ++candidateIndex)
	{
		if (!sequence->Directions[candidateIndex]) continue;
		int distance = std::abs(candidateIndex - directionIndex);
		distance = std::min(distance, Direction::MAX_DIRECTIONS - distance);
		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestDirectionIndex = candidateIndex;
		}
	}
	return bestDirectionIndex;
}

int Animations::GetFirstSequenceIndex()
{
	for (int i = 0; i < static_cast<int>(Sequences->size()); ++i)
	{
		if ((*Sequences)[i]) {
			return i;
		}
	}
	return 0;
}

int Animations::ResolveSequenceIndex(int requestedSequenceIndex)
{
	if (requestedSequenceIndex >= 0 && static_cast<size_t>(requestedSequenceIndex) < Sequences->size() && (*Sequences)[requestedSequenceIndex])
	{
		return requestedSequenceIndex;
	}

	return GetFirstSequenceIndex();
}

bool Animations::IsTwoSpeedAnimation(int sequenceIndex)
{
	if (sequenceIndex == 0 || // Stopped
		sequenceIndex == 3 || // Appearing
		sequenceIndex == 4 || // Disappearing
		sequenceIndex == 6    // Launching
		) 
		return false;
	
	return true;
}