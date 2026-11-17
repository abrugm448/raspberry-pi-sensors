#pragma once

#include <gpiod.hpp>

#include <chrono>
#include <cstdint>
#include <string>

class MovementSensor
{
public:
    MovementSensor(const std::string &chip_path, unsigned pin);

    void processEvents();

private:
    static gpiod::line_request createRequest(const std::string &chip_path, unsigned int pin);

    unsigned pin;
    gpiod::line_request request;
    gpiod::edge_event_buffer buffer{16};
};

