#include "MqttKaku.h"
#include "constants.h"

#include <iostream>

const int KAKU_CHANNELS_COUNT = 10;          // channels 0 to 9
const std::string AVAILABILITY = "\"availability_topic\":\"" + constants::PREFIX + "/status\"";
const std::string DEVICE =
R"("device":{"identifiers":["kiosk_pi"],"name":"Kiosk Pi","manufacturer":"Zelfbouw","sw_version":"1.0.0"})";

MqttKaku::MqttKaku(const std::string &client_id, KakuTransmitter &kaku) : MqttClient(client_id), kaku(kaku)
{
}

void MqttKaku::publish_discovery()
{
    std::cout << "publish discovery" << std::endl;

    for (int c = 0; c < KAKU_CHANNELS_COUNT; c++)
    {
        publish(constants::DISCOVERY + "/switch/kiosk/kaku_" + std::to_string(c) + "/config",
            switch_config(c), true);
    }

    publish(constants::DISCOVERY + "/button/kiosk/kaku_alles_aan/config",
        button_config("kaku_alles_aan", "Alles aan", "mdi:power-plug"), true);
    publish(constants::DISCOVERY + "/button/kiosk/kaku_alles_uit/config",
        button_config("kaku_alles_uit", "Alles uit", "mdi:power-plug-off"), true);
}

void MqttKaku::connected()
{
    std::cout << "on connect reached" << std::endl;

    publish(constants::PREFIX + "/status", "online", true);
    publish_discovery();
    subscribe(constants::PREFIX + "/kaku/+/set");          // + = elk kanaal
    subscribe(constants::PREFIX + "/kaku_alles_aan/press");
    subscribe(constants::PREFIX + "/kaku_alles_uit/press");
    subscribe(constants::DISCOVERY + "/status");           // HA herstart, discovery opnieuw
}

void MqttKaku::message_received(const std::string &topic, const std::string &payload)
{
    std::cout << "on message reached" << std::endl;

    // Schakelaar: kiosk/kaku/<kanaal>/set met ON of OFF
    const std::string switch_prefix = constants::PREFIX + "/kaku/";
    if (topic.rfind(switch_prefix, 0) == 0 && topic.size() > switch_prefix.size() + 4 &&
        topic.compare(topic.size() - 4, 4, "/set") == 0)
    {
        const std::string number =
            topic.substr(switch_prefix.size(), topic.size() - switch_prefix.size() - 4);
        int channel = 0;
        try
        {
            channel = std::stoi(number);
        }
        catch (...)
        {
            return;
        }
        if (channel < 0 || channel >= KAKU_CHANNELS_COUNT || (payload != "ON" && payload != "OFF"))
        {
            return;
        }
        const bool on = payload == "ON";
        kaku.send_unit(channel, on);
        publish(switch_topic(channel), payload, true);  // zoals setState()
        std::cout << "Kanaal " << channel << " -> " << (on ? "aan" : "uit") << std::endl;
        return;
    }

    // Knoppen "Alles aan" / "Alles uit"
    if (topic == constants::PREFIX + "/kaku_alles_aan/press" || topic == constants::PREFIX + "/kaku_alles_uit/press")
    {
        const bool on = topic == constants::PREFIX + "/kaku_alles_aan/press";
        kaku.send_group(on);
        for (unsigned ch = 0; ch < KAKU_CHANNELS_COUNT; ++ch)
        {
            publish(switch_topic(ch), on ? "ON" : "OFF", true);
        }
        std::cout << (on ? "Alles aan" : "Alles uit") << std::endl;
        return;
    }

    if (topic == constants::DISCOVERY + "/status" && payload == "online")
    {
        publish_discovery();
    }
}

std::string MqttKaku::button_config(const std::string &id, const std::string &name, const std::string &icon)
{
    return "{\"name\":\"" + name + "\"," +
        "\"unique_id\":\"kiosk_" + id + "\"," +
        "\"command_topic\":\"" + constants::PREFIX + "/" + id + "/press\"," +
        "\"icon\":\"" + icon + "\"," + AVAILABILITY + "," + DEVICE + "}";
}

std::string MqttKaku::switch_config(int channel)
{
    const std::string topic = switch_topic(channel);
    return "{\"name\":\"Switch " + std::to_string(channel) + "\"," +
        "\"unique_id\":\"kiosk_kaku_" + std::to_string(channel) + "\"," +
        "\"command_topic\":\"" + topic + "/set\"," +
        "\"state_topic\":\"" + topic + "\"," +
        "\"icon\":\"mdi:lightbulb\"," + AVAILABILITY + "," + DEVICE + "}";
}

std::string MqttKaku::switch_topic(int channel)
{
    return constants::PREFIX + "/kaku/" + std::to_string(channel);
}