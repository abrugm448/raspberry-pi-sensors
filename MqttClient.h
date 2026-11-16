#pragma once

#include <chrono>
#include <string>

struct mosquitto;
struct mosquitto_message;

// Dunne C++-wrapper rond libmosquitto. Single-threaded: roep loop() regelmatig aan.
// Afleiden en connected()/message_received() overschrijven om op events te reageren.
class MqttClient {
public:
    explicit MqttClient(const std::string& client_id);
    virtual ~MqttClient();

    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    void set_credentials(const std::string& user, const std::string& password);
    void set_will(const std::string& topic, const std::string& payload, bool retain);

    void connect(const std::string& host, int port);

    // Verwerkt netwerkverkeer en verbindt opnieuw als de verbinding weg is.
    void loop(int timeout_ms);

    void publish(const std::string& topic, const std::string& payload, bool retain = false);
    void subscribe(const std::string& topic);

protected:
    // Wordt aangeroepen na een geslaagde (her)verbinding.
    virtual void connected() {}
    // Wordt aangeroepen voor elk binnenkomend bericht.
    virtual void message_received(const std::string& topic, const std::string& payload) {}

private:
    static void handle_connect(mosquitto*, void* self, int rc);
    static void handle_message(mosquitto*, void* self, const mosquitto_message* msg);

    mosquitto* mosq_ = nullptr;
    std::chrono::steady_clock::time_point next_reconnect_{};
};