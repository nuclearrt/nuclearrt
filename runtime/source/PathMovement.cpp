#include "PathMovement.h"

#include <cmath>

void PathMovement::Initialize() {
	originX = Instance->GetX();
	originY = Instance->GetY();
	currentNodeIndex = 0;
}

void PathMovement::OnEnabled() {
	originX = Instance->GetX();
	originY = Instance->GetY();
	currentNodeIndex = 0;
	movingForward = true;
	stopped = false;
}

void PathMovement::Start() {
	stopped = false;
}

void PathMovement::Stop() {
	stopped = true;
}

void PathMovement::Update(float deltaTime) {
	if (Nodes.empty()) {
		return;
	}

	if (stopped) {
		return;
	}

	if (currentNodeIndex >= static_cast<int>(Nodes.size())) {
		if (Loop) {
			bool wasMovingForward = movingForward;
			
			if (RepositionAtEnd) {
				Instance->SetX(static_cast<float>(originX));
				Instance->SetY(static_cast<float>(originY));
			} else {
				if (!ReverseAtEnd) {
					originX = static_cast<int>(Instance->GetX());
					originY = static_cast<int>(Instance->GetY());
				}
			}

			if (ReverseAtEnd) {
				movingForward = !movingForward;
				if (wasMovingForward && !movingForward && !RepositionAtEnd) {
					int endX = originX;
					int endY = originY;
					for (size_t i = 0; i < Nodes.size(); ++i) {
						endX += Nodes[i].DestinationX;
						endY += Nodes[i].DestinationY;
					}
					Instance->SetX(static_cast<float>(endX));
					Instance->SetY(static_cast<float>(endY));
				}
			}

			if (movingForward) {
				currentNodeIndex = 0;
			} else {
				currentNodeIndex = static_cast<int>(Nodes.size()) - 1;
			}

		} else {
			bool wasMovingForward = movingForward;
			
			if (RepositionAtEnd) {
				Instance->SetX(static_cast<float>(originX));
				Instance->SetY(static_cast<float>(originY));
			} else {
				if (!ReverseAtEnd) {
					originX = static_cast<int>(Instance->GetX());
					originY = static_cast<int>(Instance->GetY());
				}
			}

			if (ReverseAtEnd) {
				movingForward = !movingForward;
				if (wasMovingForward && !movingForward && !RepositionAtEnd) {
					int endX = originX;
					int endY = originY;
					for (size_t i = 0; i < Nodes.size(); ++i) {
						endX += Nodes[i].DestinationX;
						endY += Nodes[i].DestinationY;
					}
					Instance->SetX(static_cast<float>(endX));
					Instance->SetY(static_cast<float>(endY));
				}
				
				if (movingForward) {
					currentNodeIndex = 0;
				} else {
					currentNodeIndex = static_cast<int>(Nodes.size()) - 1;
				}
			} else {
				stopped = true;
				return;
			}
		}
	}
	
	if (currentNodeIndex < 0 && !movingForward) {
		float dx = static_cast<float>(originX) - Instance->GetX();
		float dy = static_cast<float>(originY) - Instance->GetY();
		float distanceToOrigin = sqrtf(dx * dx + dy * dy);
		
		if (distanceToOrigin <= 0.0001f) {
			Instance->SetX(static_cast<float>(originX));
			Instance->SetY(static_cast<float>(originY));
			
			if (Loop && ReverseAtEnd) {
				movingForward = !movingForward;
				currentNodeIndex = 0;
			} else if (Loop) {
				movingForward = !movingForward;
				currentNodeIndex = 0;
			} else {
				stopped = true;
				return;
			}
		} else {
			float scaledDelta = deltaTime * 10.0f;
			int nodeSpeed = Nodes.empty() ? MinimumSpeed : static_cast<int>(Nodes[0].Speed);
			if (nodeSpeed < MinimumSpeed) nodeSpeed = MinimumSpeed;
			if (nodeSpeed > MaximumSpeed) nodeSpeed = MaximumSpeed;
			
			float step = nodeSpeed * scaledDelta;
			if (step >= distanceToOrigin) {
				Instance->SetX(static_cast<float>(originX));
				Instance->SetY(static_cast<float>(originY));
				
				if (Loop && ReverseAtEnd) {
					movingForward = !movingForward;
					currentNodeIndex = 0;
				} else if (Loop) {
					movingForward = !movingForward;
					currentNodeIndex = 0;
				} else {
					stopped = true;
					return;
				}
			} else {
				float nx = dx / distanceToOrigin;
				float ny = dy / distanceToOrigin;
				Instance->SetX(Instance->GetX() + nx * step);
				Instance->SetY(Instance->GetY() + ny * step);
			}
		}
		return;
	}

	int targetX = originX;
	int targetY = originY;
	if (movingForward) {
		for (int i = 0; i <= currentNodeIndex; ++i) {
			targetX += Nodes[i].DestinationX;
			targetY += Nodes[i].DestinationY;
		}
	} else {
		for (int i = 0; i <= currentNodeIndex; ++i) {
			targetX += Nodes[i].DestinationX;
			targetY += Nodes[i].DestinationY;
		}
	}

	deltaTime *= 10.0f;

	float dx = static_cast<float>(targetX) - Instance->GetX();
	float dy = static_cast<float>(targetY) - Instance->GetY();
	float distanceToTarget = sqrtf(dx * dx + dy * dy);

	if (distanceToTarget <= 0.0001f) {
		Instance->SetX(static_cast<float>(targetX));
		Instance->SetY(static_cast<float>(targetY));
		currentNodeIndex += movingForward ? 1 : -1;
		return;
	}

	int nodeSpeed = static_cast<int>(Nodes[currentNodeIndex].Speed);
	if (nodeSpeed < MinimumSpeed) nodeSpeed = MinimumSpeed;
	if (nodeSpeed > MaximumSpeed) nodeSpeed = MaximumSpeed;

	float step = nodeSpeed * deltaTime;
	if (step >= distanceToTarget) {
		Instance->SetX(static_cast<float>(targetX));
		Instance->SetY(static_cast<float>(targetY));
		currentNodeIndex += movingForward ? 1 : -1;
	} else {
		float nx = dx / distanceToTarget;
		float ny = dy / distanceToTarget;
		Instance->SetX(Instance->GetX() + nx * step);
		Instance->SetY(Instance->GetY() + ny * step);
	}
}

int PathMovement::GetRealSpeed() {
	if (Nodes.empty()) {
		return 0;
	}

	if (stopped) {
		return 0;
	}

	if (currentNodeIndex >= static_cast<int>(Nodes.size()) || currentNodeIndex < 0) {
		return 0;
	}

	return static_cast<int>(Nodes[currentNodeIndex].Speed);
}

int PathMovement::GetMovementDirection() {
	if (Nodes.empty()) {
		return 0;
	}

	if (currentNodeIndex >= static_cast<int>(Nodes.size()) || currentNodeIndex < 0) {
		return 0;
	}

	return static_cast<int>(Nodes[currentNodeIndex].Direction);
}