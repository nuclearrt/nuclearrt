#pragma once
#include <vector>

#include "Movement.h"

class Movements
{
public:
	Movements() = default;
	Movements(std::vector<Movement*> movementItems)
		: items(std::move(movementItems)), currentMovementIndex(0) {}

	void Deinitialize() {
		if (Movement* mvt = GetCurrentMovement()) {
			mvt->OnDisabled();
		}

		for (auto& movement : items) delete movement;
		items.clear();
		currentMovementIndex = 0;
	}

	std::vector<Movement*> items;
	unsigned int currentMovementIndex = 0;

	void Update(float deltaTime) {
		Movement* mvt = GetCurrentMovement();
		if (!mvt) return;
		mvt->Update(deltaTime);
	}

	void SetMovement(const CValue& index) {
		int newMovementIndex = index.GetIntValue();
		if (newMovementIndex < 0 || static_cast<size_t>(newMovementIndex) >= items.size()) return;

		Movement* oldmvt = GetCurrentMovement();
		if (oldmvt) {
			oldmvt->OnDisabled();
		}
		currentMovementIndex = static_cast<unsigned int>(newMovementIndex);
		items[currentMovementIndex]->OnEnabled();
	}

	void NextMovement() {
		if (static_cast<size_t>(currentMovementIndex) + 1 >= items.size()) return;

		items[currentMovementIndex]->OnDisabled();
		currentMovementIndex++;
		items[currentMovementIndex]->OnEnabled();
	}

	void PreviousMovement() {
		if (currentMovementIndex == 0 || currentMovementIndex >= items.size()) return;

		items[currentMovementIndex]->OnDisabled();
		currentMovementIndex--;
		items[currentMovementIndex]->OnEnabled();
	}

	Movement* GetCurrentMovement() {
		if (currentMovementIndex < items.size()) return items[currentMovementIndex];
		return nullptr;
	}
};