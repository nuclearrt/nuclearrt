#include "Animations.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "Application.h"

Animations::Animations(const std::unordered_map<int, Sequence*> sequences) {
	Sequences = sequences;

	//set current sequence to the one with the lowest index
	if (!Sequences.empty()) {
		RequestedSequenceIndex = 0;
		RequestedDirection = 0;
		CurrentSequenceIndex = ResolveSequenceIndex(RequestedSequenceIndex);
		auto* currentSequence = Sequences.at(CurrentSequenceIndex);
		CurrentDirection = GetNearestDirectionIndex(currentSequence, RequestedDirection);
	}
}

bool Animations::IsSequencePlaying(int sequence) const {
	int displayedSequence = forcedSequence != -1 ? forcedSequence : CurrentSequenceIndex;
	return displayedSequence == sequence;
}

bool Animations::IsSequenceOver(int sequence) const {
	return SequenceOverEvents.find(sequence) != SequenceOverEvents.end();
}

bool Animations::IsFacingDirection(int directionIndex) const
{
	return GetCurrentDirection() == directionIndex;
}

bool Animations::IsFacingDirectionMask(int directionMask) const
{
	if (directionMask == -1) return true;
	return (1 << GetCurrentDirection()) & directionMask;
}

std::vector<unsigned int> Animations::GetImagesUsed() const
{
	std::vector<unsigned int> imagesUsed;
	for (const auto& sequencePair : Sequences) {
		const auto& sequence = sequencePair.second;
		for (const auto& directionPair : sequence->Directions) {
			for (const auto& frame : directionPair.second->Frames) {
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

	auto* sequence = Sequences.at(displaySequence);

	if (sequence->Directions.find(displayDirection) == sequence->Directions.end() || AutomaticRotation) {
		displayDirection = GetNearestDirectionIndex(sequence, displayDirection);
	}
	
	auto* direction = sequence->Directions.at(displayDirection);
	return direction->Frames.at(displayFrame);
}

unsigned int Animations::GetCurrentSequenceIndex() const {
	return forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex;
}

unsigned int Animations::GetCurrentDirection() const {
	return forcedDirection != -1 ? forcedDirection : RequestedDirection;
}

unsigned int Animations::GetCurrentFrameIndex() const {
	return forcedFrame != -1 ? forcedFrame : CurrentFrameIndex;
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

	auto* sequence = Sequences.at(resolvedSequenceIndex);
	CurrentSequenceIndex = resolvedSequenceIndex;
	CurrentDirection = GetNearestDirectionIndex(sequence, RequestedDirection);
	CurrentFrameIndex = 0;
	CurrentFrameTime = 0.0f;
}

void Animations::SetCurrentDirectionMask(int directionMask)
{
	int newDirection = -1;

	std::vector<int> validDirections;
	if (directionMask == -1) // set to any valid direction
	{
		// set to all
		for (int i = 0; i < 32; ++i)
		{
			validDirections.push_back(i);
		}
	}
	else
	{
		for (int i = 0; i < 32; ++i)
		{
			if (directionMask & (1 << i))
			{
				validDirections.push_back(i);
			}
		}
	}

	if (validDirections.empty()) return;

	// get a random valid direction from the list
	int index = Application::Instance().RandomRange(0, static_cast<short>(validDirections.size() - 1));

	SetCurrentDirection(validDirections.at(index));
}

void Animations::SetCurrentDirection(int index) {
	if (index == RequestedDirection) {
		return;
	}

	RequestedDirection = index;
	lastSequenceOverIndex = -1;
	auto* currentSequence = Sequences.at(CurrentSequenceIndex);
	CurrentDirection = GetNearestDirectionIndex(currentSequence, RequestedDirection);
	automaticRotationDirection = CurrentDirection;
	auto* direction = currentSequence->Directions.at(CurrentDirection);
	if (direction->Frames.empty()) {
		CurrentFrameIndex = 0;
	} else {
		CurrentFrameIndex = std::clamp(CurrentFrameIndex, 0, static_cast<int>(direction->Frames.size() - 1));
	}
}

void Animations::SetForcedFrame(int frame) {
	if (forcedFrame == frame) return;
	
	auto* sequence = Sequences.at(ResolveSequenceIndex(forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex));
	auto* direction = sequence->Directions.at(forcedDirection != -1 ? forcedDirection : GetNearestDirectionIndex(sequence, RequestedDirection));

	frame = std::clamp(frame, 0, static_cast<int>(direction->Frames.size() - 1));
	forcedFrame = frame;
}

void Animations::SetForcedDirection(int directionMask) {
	int newDirection = -1;
	auto& sequence = Sequences.at(CurrentSequenceIndex);

	std::vector<int> validDirections;
	if (directionMask == -1) // set to any valid direction
	{
		//set to all
		for (int i = 0; i < 32; ++i) {
			validDirections.push_back(i);
		}
	}
	else
	{
		for (int i = 0; i < 32; ++i) {
			if (directionMask & (1 << i)) {
				validDirections.push_back(i);
			}
		}
	}

	//remove any directions that don't exist
	validDirections.erase(std::remove_if(validDirections.begin(), validDirections.end(), [sequence](int direction) {
		return sequence->Directions.find(direction) == sequence->Directions.end();
	}), validDirections.end());

	if (validDirections.empty()) {
		return;
	}

	//get a random valid direction from the list
	int index = Application::Instance().RandomRange(0, static_cast<short>(validDirections.size() - 1));
	newDirection = validDirections.at(index);
	automaticRotationDirection = newDirection;

	if (forcedDirection == newDirection) return;

	forcedDirection = newDirection;
	RequestedDirection = forcedDirection;
	auto* direction = sequence->Directions.at(forcedDirection);
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
	SequenceOverEvents.erase(SequenceOverEvents.begin(), SequenceOverEvents.end());

	int requestedSequence = forcedSequence != -1 ? forcedSequence : RequestedSequenceIndex;
	int requestedDirection = forcedDirection != -1 ? forcedDirection : RequestedDirection;
	int displaySequence = forcedSequence != -1 ? forcedSequence : CurrentSequenceIndex;
	int displayDirection = requestedDirection;

	if (forcedDirection != -1) {
		displayDirection = forcedDirection;
	}

	auto* currentSequence = Sequences.at(displaySequence);

	//if the direction doesn't exist, set to the first direction
	if (currentSequence->Directions.find(displayDirection) == currentSequence->Directions.end()) {
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
	auto& currentDirection = currentSequence->Directions.at(displayDirection);
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

					if (lastSequenceOverIndex != requestedSequence) {
						SequenceOverEvents[requestedSequence] = true;
						lastSequenceOverIndex = requestedSequence;
					}

					// pick the lowest available seq
					RequestedSequenceIndex = 0;
					CurrentSequenceIndex = ResolveSequenceIndex(RequestedSequenceIndex);
					CurrentDirection = GetNearestDirectionIndex(Sequences.at(CurrentSequenceIndex), RequestedDirection);
					RequestedDirection = CurrentDirection;
					CurrentFrameIndex = 0;
					CurrentFrameTime = 0.0f;
				}
				else // if it is the first one, stay on the last frame
				{
					CurrentFrameIndex -= 1;
					if (lastSequenceOverIndex != requestedSequence) {
						SequenceOverEvents[requestedSequence] = true;
						lastSequenceOverIndex = requestedSequence;
					}
				}
			}
		}
	}
}

int Animations::GetLowestDirectionIndex(const Sequence *sequence) const
{
	int minDirectionIndex = sequence->Directions.begin()->second->Index;
	for (const auto &dirPair : sequence->Directions)
	{
		if (dirPair.second->Index < minDirectionIndex)
		{
			minDirectionIndex = dirPair.second->Index;
		}
	}
	return minDirectionIndex;
}

int Animations::GetNearestDirectionIndex(const Sequence *sequence, int directionIndex) const
{
	if (AutomaticRotation)
	{
		directionIndex = 0;
	}

	if (sequence->Directions.find(directionIndex) != sequence->Directions.end())
	{
		return directionIndex;
	}

	int bestDirectionIndex = GetLowestDirectionIndex(sequence);
	int bestDistance = 32;
	for (const auto &dirPair : sequence->Directions)
	{
		int candidateIndex = dirPair.second->Index;
		int distance = std::abs(candidateIndex - directionIndex);
		distance = std::min(distance, 32 - distance);
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
	int firstSequenceIndex = Sequences.begin()->first;
	for (const auto &pair : Sequences)
	{
		if (pair.first < firstSequenceIndex)
		{
			firstSequenceIndex = pair.first;
		}
	}
	return firstSequenceIndex;
}

int Animations::ResolveSequenceIndex(int requestedSequenceIndex)
{
	if (Sequences.find(requestedSequenceIndex) != Sequences.end())
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