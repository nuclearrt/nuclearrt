#pragma once

#include <utility>
#include "Direction.h"
#include <vector>
#include <array>

class Sequence {
public:
	Sequence(std::array<Direction*, Direction::MAX_DIRECTIONS> directions)
		: Directions(std::move(directions)) {}

	std::array<Direction*, Direction::MAX_DIRECTIONS> Directions;
}; 