#include "KakuTransmitter.h"
#include "MqttKaku.h"
#include "constants.h"

#include <atomic>
#include <iostream>
#include <string>
#include <csignal>

constexpr unsigned TRANSMITTER_PIN = 4;           // GPIO pin for 433 mhz sending
constexpr std::uint32_t KAKU_ADDRESS = 12345678;

void stop(int); 

std::atomic<bool> running{ true };

//int main(int argc, char *argv[])
int main()
{
    try
    {
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);

        KakuTransmitter kaku("/dev/gpiochip0", TRANSMITTER_PIN, KAKU_ADDRESS);
        MqttKaku mqtt("kiosk-sensors", kaku);
        mqtt.set_credentials("esp32", "egabrag");
        mqtt.set_will(constants::PREFIX + "/status", "offline", true);

        mqtt.connect("192.168.2.51", 1883);

        while (running)
        {
            mqtt.loop(100);
        }

        mqtt.publish(constants::PREFIX + "/status", "offline", true);
        mqtt.loop(100);
        std::cout << "Gestopt" << std::endl;

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "got exception: " << e.what() << std::endl;
    }
}

void stop(int) 
{ 
    running = false; 
}