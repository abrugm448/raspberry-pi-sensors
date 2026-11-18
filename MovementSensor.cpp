#include "MovementSensor.h"

#include <sched.h>

#include <stdexcept>


MovementSensor::MovementSensor(const std::string &chip_path, unsigned pin)
    : request(createRequest(chip_path, pin)),
    pin(pin)
{
}

gpiod::line_request MovementSensor::createRequest(const std::string &chip_path, unsigned int pin)
{
    return gpiod::chip(chip_path)
        .prepare_request()
        .set_consumer("movement-sensor")
        .add_line_settings(
            pin,
            gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_debounce_period(std::chrono::milliseconds(10)))
        .do_request();
}

MotionState MovementSensor::getMotionState()
{
    if (!request.wait_edge_events(std::chrono::milliseconds(0)))
        return MotionState::NotDetected;

    request.read_edge_events(buffer);

    for (const gpiod::edge_event &event : buffer)
    {
        if (event.type() == gpiod::edge_event::event_type::RISING_EDGE)
        {
            std::cout << "Movement!" << std::endl;
            return MotionState::Detected;
        }
        else
        {
            std::cout << "Nothing here." << std::endl;
            return MotionState::NotDetected;
        }
    }

    return MotionState::NotDetected;
}
