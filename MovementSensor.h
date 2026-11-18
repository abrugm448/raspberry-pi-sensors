#pragma once

#include <gpiod.hpp>

#include <chrono>
#include <cstdint>
#include <string>

enum class MotionState { Detected, NotDetected };

class MovementSensor
{
public:
    MovementSensor(const std::string &chip_path, unsigned pin);

    MotionState getMotionState();

private:
    static gpiod::line_request createRequest(const std::string &chip_path, unsigned int pin);

    unsigned pin;
    gpiod::line_request request;
    gpiod::edge_event_buffer buffer{ 16 };
};

