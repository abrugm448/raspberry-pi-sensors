#include "kaku_transmitter.h"
#include "mqtt_client.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

//#include "kiosk-sensors.h"
//#include <gpiod.hpp>
//#include <iostream>

using namespace std;

constexpr unsigned SENDER_PIN = 4;           // GPIO pin
constexpr std::uint32_t ADDRESS = 12345678;   // zelfde adres als op de ESP32
constexpr unsigned SWITCH_COUNT = 10;          // kanalen 0 t/m 9, zoals op de ESP32

const std::string PREFIX = "kiosk";          // eigen MQTT-topics: kiosk/...
const std::string DISCOVERY = "homeassistant";  // discovery-prefix van HA

const std::string DEVICE =
R"("device":{"identifiers":["kiosk_pi"],"name":"Kiosk Pi","manufacturer":"Zelfbouw","sw_version":"1.0.0"})";

const std::string AVAILABILITY = "\"availability_topic\":\"" + PREFIX + "/status\"";

std::string switch_topic(unsigned channel) { return PREFIX + "/kaku/" + std::to_string(channel); }

std::string switch_config(unsigned channel)
{
    const std::string topic = switch_topic(channel);
    return "{\"name\":\"Switch " + std::to_string(channel) + "\"," +
        "\"unique_id\":\"kiosk_kaku_" + std::to_string(channel) + "\"," +
        "\"command_topic\":\"" + topic + "/set\"," +
        "\"state_topic\":\"" + topic + "\"," +
        "\"icon\":\"mdi:lightbulb\"," + AVAILABILITY + "," + DEVICE + "}";
}

std::string button_config(const std::string &id, const std::string &name, const std::string &icon)
{
    return "{\"name\":\"" + name + "\"," +
        "\"unique_id\":\"kiosk_" + id + "\"," +
        "\"command_topic\":\"" + PREFIX + "/" + id + "/press\"," +
        "\"icon\":\"" + icon + "\"," + AVAILABILITY + "," + DEVICE + "}";
}

std::atomic<bool> running{ true };
void stop(int) { running = false; }

std::string env(const char *name, const std::string &fallback)
{
    const char *value = std::getenv(name);
    return value ? value : fallback;
}

int main()
{
    try
    {
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);

        KakuTransmitter kaku("/dev/gpiochip0", SENDER_PIN, ADDRESS);
        MqttClient mqtt("kiosk-sensors");
        mqtt.set_credentials(env("MQTT_USER", "esp32"), env("MQTT_PASSWORD", "egabrag"));
        mqtt.set_will(PREFIX + "/status", "offline", true);


        auto publish_discovery = [&] {
            for (unsigned ch = 0; ch < SWITCH_COUNT; ++ch)
            {
                mqtt.publish(DISCOVERY + "/switch/kiosk/kaku_" + std::to_string(ch) + "/config",
                    switch_config(ch), true);
            }
            mqtt.publish(DISCOVERY + "/button/kiosk/kaku_alles_aan/config",
                button_config("kaku_alles_aan", "Alles aan", "mdi:power-plug"), true);
            mqtt.publish(DISCOVERY + "/button/kiosk/kaku_alles_uit/config",
                button_config("kaku_alles_uit", "Alles uit", "mdi:power-plug-off"), true);
            };

        mqtt.on_connect([&] {
            mqtt.publish(PREFIX + "/status", "online", true);
            publish_discovery();
            mqtt.subscribe(PREFIX + "/kaku/+/set");          // + = elk kanaal
            mqtt.subscribe(PREFIX + "/kaku_alles_aan/press");
            mqtt.subscribe(PREFIX + "/kaku_alles_uit/press");
            mqtt.subscribe(DISCOVERY + "/status");           // HA herstart → discovery opnieuw
            });

        mqtt.on_message([&](const std::string &topic, const std::string &payload) {
            // Schakelaar: kiosk/kaku/<kanaal>/set met ON of OFF
            const std::string switch_prefix = PREFIX + "/kaku/";
            if (topic.rfind(switch_prefix, 0) == 0 && topic.size() > switch_prefix.size() + 4 &&
                topic.compare(topic.size() - 4, 4, "/set") == 0)
            {
                const std::string number =
                    topic.substr(switch_prefix.size(), topic.size() - switch_prefix.size() - 4);
                unsigned channel = 0;
                try
                {
                    channel = std::stoul(number);
                }
                catch (...)
                {
                    return;
                }
                if (channel >= SWITCH_COUNT || (payload != "ON" && payload != "OFF"))
                {
                    return;
                }
                const bool on = payload == "ON";
                kaku.send_unit(channel, on);
                mqtt.publish(switch_topic(channel), payload, true);  // zoals setState()
                std::cout << "Kanaal " << channel << " -> " << (on ? "aan" : "uit") << std::endl;
                return;
            }

            // Knoppen "Alles aan" / "Alles uit"
            if (topic == PREFIX + "/kaku_alles_aan/press" || topic == PREFIX + "/kaku_alles_uit/press")
            {
                const bool on = topic == PREFIX + "/kaku_alles_aan/press";
                kaku.send_group(on);
                for (unsigned ch = 0; ch < SWITCH_COUNT; ++ch)
                {
                    mqtt.publish(switch_topic(ch), on ? "ON" : "OFF", true);
                }
                std::cout << (on ? "Alles aan" : "Alles uit") << std::endl;
                return;
            }

            if (topic == DISCOVERY + "/status" && payload == "online")
            {
                publish_discovery();
            }
            });

        mqtt.connect(env("MQTT_HOST", "192.168.2.51"), std::stoi(env("MQTT_PORT", "1883")));

        while (running)
        {
            mqtt.loop(100);
        }

        mqtt.publish(PREFIX + "/status", "offline", true);
        mqtt.loop(100);
        std::cout << "Gestopt" << std::endl;

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fout: " << e.what() << "\n";
        return 1;
    }
}