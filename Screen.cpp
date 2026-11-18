#include "Screen.h"
#include "MovementSensor.h"

Screen::Screen()
    : state(State::On)
{
    //
    // get the screen config directory
    for (auto &e : std::filesystem::directory_iterator("/sys/class/backlight"))
    {
        screenPath = e.path();
    }

    //
    // get the screen max brightness
    std::ifstream f(screenPath / "max_brightness");
    f >> maxBrightness;
}

void Screen::processMovement(MotionState motionState)
{
    if (motionState == MotionState::Detected)
    {
        lastMovement = std::chrono::steady_clock::now();
        updateState(State::On);
    }
    else if (motionState == MotionState::NotDetected && state == State::On)
    {
        auto now = std::chrono::steady_clock::now();
        if (now - lastMovement > std::chrono::minutes(1))
        {
            updateState(State::Dimmed);
        }
    }
}

void Screen::updateState(State newState)
{
    if (newState == state)
    {
        return;//no new state
    }

    //
    // new values
    std::string power = "1";
    int brightness = maxBrightness;
    if (newState == State::Off)
    {
        power = "0";
        brightness = 0;
    }
    else if (newState == State::Dimmed)
    {
        brightness = 1;
    }
    state = newState;

    //
    // update screen
    std::cout << "update screen brightness " << brightness << screenPath << std::endl;
    std::ofstream(screenPath / "bl_power") << power;
    std::ofstream(screenPath / "brightness") << brightness;
    std::ofstream out(screenPath / "brightness");
    if (!out)
        std::cerr << "kan " << (screenPath / "brightness") << " niet openen\n";
    out << brightness;
}