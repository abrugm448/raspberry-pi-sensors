#include "mqtt_client.h"

#include <mosquitto.h>

#include <iostream>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

MqttClient::MqttClient(const std::string &client_id)
{
    mosquitto_lib_init();
    mosq_ = mosquitto_new(client_id.c_str(), true, this);
    if (!mosq_)
    {
        throw std::runtime_error("mosquitto_new mislukt");
    }
    mosquitto_connect_callback_set(mosq_, &MqttClient::handle_connect);
    mosquitto_message_callback_set(mosq_, &MqttClient::handle_message);
}

MqttClient::~MqttClient()
{
    if (mosq_)
    {
        mosquitto_disconnect(mosq_);
        mosquitto_loop(mosq_, 100, 1);
        mosquitto_destroy(mosq_);
    }
    mosquitto_lib_cleanup();
}

void MqttClient::set_credentials(const std::string &user, const std::string &password)
{
    mosquitto_username_pw_set(mosq_, user.c_str(), password.c_str());
}

void MqttClient::set_will(const std::string &topic, const std::string &payload, bool retain)
{
    mosquitto_will_set(mosq_, topic.c_str(), static_cast<int>(payload.size()), payload.data(), 1, retain);
}

void MqttClient::connect(const std::string &host, int port)
{
    int rc = mosquitto_connect(mosq_, host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        // Geen ramp: loop() blijft het opnieuw proberen.
        std::cerr << "MQTT: verbinden met " << host << ":" << port << " mislukt ("
            << mosquitto_strerror(rc) << "), probeer het later opnieuw\n";
    }
}

void MqttClient::loop(int timeout_ms)
{
    int rc = mosquitto_loop(mosq_, timeout_ms, 1);
    if (rc == MOSQ_ERR_SUCCESS)
    {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (now >= next_reconnect_)
    {
        next_reconnect_ = now + 5s;
        std::cerr << "MQTT: geen verbinding (" << mosquitto_strerror(rc) << "), opnieuw verbinden...\n";
        mosquitto_reconnect(mosq_);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    }
}

void MqttClient::publish(const std::string &topic, const std::string &payload, bool retain)
{
    mosquitto_publish(mosq_, nullptr, topic.c_str(), static_cast<int>(payload.size()), payload.data(), 1, retain);
}

void MqttClient::subscribe(const std::string &topic)
{
    mosquitto_subscribe(mosq_, nullptr, topic.c_str(), 1);
}

void MqttClient::handle_connect(mosquitto *, void *self, int rc)
{
    auto *client = static_cast<MqttClient *>(self);
    if (rc != 0)
    {
        std::cerr << "MQTT: verbinding geweigerd: " << mosquitto_connack_string(rc) << "\n";
        return;
    }
    std::cerr << "MQTT: verbonden\n";
    if (client->connect_handler_)
    {
        client->connect_handler_();
    }
}

void MqttClient::handle_message(mosquitto *, void *self, const mosquitto_message *msg)
{
    auto *client = static_cast<MqttClient *>(self);
    if (!client->message_handler_)
    {
        return;
    }
    std::string payload(static_cast<const char *>(msg->payload), static_cast<size_t>(msg->payloadlen));
    client->message_handler_(msg->topic, payload);
}
