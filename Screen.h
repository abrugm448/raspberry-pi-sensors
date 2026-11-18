#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>

enum class MotionState;

class Screen
{
public:
    enum class State { On, Dimmed, Off };

    Screen();

    void processMovement(MotionState motionState);

private:
    void updateState(State newState);

    std::filesystem::path screenPath;
    int maxBrightness = 255;
    State state;
    std::chrono::steady_clock::time_point lastMovement = std::chrono::steady_clock::now();
};
