#pragma once

#include <utility>
#include <vector>

class Direction {
public:
    static constexpr int MAX_DIRECTIONS = 32;

    Direction(int index, int minimumSpeed, int maximumSpeed, bool repeat, int repeatFrame, std::vector<unsigned int> frames)
        : Index(index), MinimumSpeed(minimumSpeed), MaximumSpeed(maximumSpeed), 
          Repeat(repeat), RepeatFrame(repeatFrame), Frames(std::move(frames)) {}

    int Index;
    int MinimumSpeed;
    int MaximumSpeed;
    bool Repeat;
    int RepeatFrame;
    std::vector<unsigned int> Frames;
}; 