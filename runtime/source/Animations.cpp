#include "Animations.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "Application.h"

Animations::Animations(const std::unordered_map<int, Sequence*> sequences) {
	Sequences = sequences;

	//set current sequence to the one with the lowest index
	if (!Sequences.empty()) {
		int minIndex = Sequences.begin()->first;
		for (const auto& pair : Sequences) {
			if (pair.first < minIndex) {
				minIndex = pair.first;
			}
		}
		CurrentSequenceIndex = minIndex;
		auto* firstSequence = Sequences.at(minIndex);
		// find the direction with the lowest index
		int minDirectionIndex = firstSequence->Directions.begin()->second->Index;
		for (const auto& dirPair : firstSequence->Directions) {
			if (dirPair.second->Index < minDirectionIndex) {
				minDirectionIndex = dirPair.second->Index;
			}
		}
		CurrentDirection = minDirectionIndex;
	}
}

bool Animations::IsSequencePlaying(int sequence) const {
	int displayedSequence = forcedSequence != -1 ? forcedSequence : CurrentSequenceIndex;
	return displayedSequence == sequence;
}

bool Animations::IsSequenceOver(int sequence) const {
	return SequenceOverEvents.find(sequence) != SequenceOverEvents.end();
}

std::vector<unsigned int> Animations::GetImagesUsed() const {
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
		// find the direction with the lowest index
		int minDirectionIndex = sequence->Directions.begin()->second->Index;
		for (const auto& dirPair : sequence->Directions) {
			if (dirPair.second->Index < minDirectionIndex) {
				minDirectionIndex = dirPair.second->Index;
			}
		}
		displayDirection = minDirectionIndex;
	}
	
	auto* direction = sequence->Directions.at(displayDirection);
	return direction->Frames.at(displayFrame);
}

unsigned int Animations::GetCurrentSequenceIndex() const {
	return CurrentSequenceIndex;
}

unsigned int Animations::GetCurrentDirection() const {
	return CurrentDirection;
}

unsigned int Animations::GetCurrentFrameIndex() const {
	return CurrentFrameIndex;
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
	if (index == CurrentSequenceIndex) {
		return;
	}

	if (Sequences.find(index) == Sequences.end()) {
		return;
	}

	CurrentSequenceIndex = index;
	auto* sequence = Sequences.at(index);
	// find the direction with the lowest index
	int minDirectionIndex = sequence->Directions.begin()->second->Index;
	for (const auto& dirPair : sequence->Directions) {
		if (dirPair.second->Index < minDirectionIndex) {
			minDirectionIndex = dirPair.second->Index;
		}
	}
	CurrentDirection = minDirectionIndex;
	CurrentFrameIndex = 0;
	CurrentFrameTime = 0.0f;
}

void Animations::SetCurrentDirection(int index) {
	if (index == CurrentDirection) {
		return;
	}

	auto& currentSequence = Sequences.at(CurrentSequenceIndex);
	if (index >= 0 && currentSequence->Directions.find(index) != currentSequence->Directions.end()) {
		CurrentDirection = index;
		CurrentFrameIndex = 0; // Reset frame when changing direction
		CurrentFrameTime = 0.0f; // Reset frame time
	}
}

void Animations::SetCurrentFrameIndex(int index) {
	if (index == CurrentFrameIndex) {
		return;
	}

	auto currentDirection = Sequences.at(CurrentSequenceIndex)->Directions.find(CurrentDirection);
	if (index >= 0 && currentDirection != Sequences.at(CurrentSequenceIndex)->Directions.end() && index < static_cast<int>(currentDirection->second->Frames.size())) {
		CurrentFrameIndex = index;
		CurrentFrameTime = 0.0f; // Reset frame time
	}
}

void Animations::SetForcedSequence(int sequence) {
	forcedSequence = sequence;
}

void Animations::SetForcedFrame(int frame) {
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
	CurrentFrameIndex = 0;
	CurrentFrameTime = 0.0f;
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

void Animations::Update(float deltaTime) {
	CurrentFrameTime += deltaTime;
	
	SequenceOverEvents.erase(SequenceOverEvents.begin(), SequenceOverEvents.end());

	int displayFrame = CurrentFrameIndex;
	int displaySequence = CurrentSequenceIndex;
	int displayDirection = CurrentDirection;
	if (forcedFrame != -1) {
		displayFrame = forcedFrame;
	}
	if (forcedDirection != -1) {
		displayDirection = forcedDirection;
	}
	if (forcedSequence != -1) {
		displaySequence = forcedSequence;
	}

	auto& currentSequence = Sequences.at(displaySequence);

	//if the direction doesn't exist, set to the first direction
	if (currentSequence->Directions.find(displayDirection) == currentSequence->Directions.end()) {
		forcedDirection = -1;
		// find the direction with the lowest index
		int minDirectionIndex = currentSequence->Directions.begin()->second->Index;
		for (const auto& dirPair : currentSequence->Directions) {
			if (dirPair.second->Index < minDirectionIndex) {
				minDirectionIndex = dirPair.second->Index;
			}
		}
		displayDirection = minDirectionIndex;
		CurrentDirection = displayDirection;
		CurrentFrameIndex = 0;
		CurrentFrameTime = 0.0f;
		return;
	}
	auto& currentDirection = currentSequence->Directions.at(displayDirection);

	int animSpeed = currentDirection->MaximumSpeed; // TODO: look at minimum speed too

	if (animSpeed == 0) {
		return;
	}

	if (!started) {
		// If the animation is stopped, we don't update the frame time or index
		return;
	}

	if (CurrentFrameTime >= 1.0f / animSpeed) {

		//TODO: verify if this is correct
		if (forcedFrame != -1) return; // If a forced frame is set, don't update the index

		CurrentFrameTime = 0.0f;
		CurrentFrameIndex++;

		if (CurrentFrameIndex >= static_cast<int>(currentDirection->Frames.size())) // reached end of animation
		{
			if (currentDirection->Repeat) {
				CurrentFrameIndex = currentDirection->RepeatFrame;
			} else {
				
				// If animation is not the first one (stopped), change to that animation
				// find the sequence with the lowest index (the "first" sequence)
				int firstSequenceIndex = Sequences.begin()->first;
				for (const auto& pair : Sequences) {
					if (pair.first < firstSequenceIndex) {
						firstSequenceIndex = pair.first;
					}
				}
				
				if (CurrentSequenceIndex != firstSequenceIndex) {
					//set sequence over event
					SequenceOverEvents[displaySequence] = true;

					//Change to the first sequence
					SetCurrentSequenceIndex(firstSequenceIndex);
				}
				else // if it is the first one, stay on the last frame
				{
					CurrentFrameIndex -= 1;

					//TODO: this shouldn't activate during certian conditions but idk what they are
					//set sequence over event
					SequenceOverEvents[displaySequence] = true;
				}
			}
		}
	}
}