#pragma once

#include <chrono>
#include <functional>
#include <string>

struct mosquitto;
struct mosquitto_message;

// Dunne C++-wrapper rond libmosquitto. Single-threaded: roep loop() regelmatig aan.
class MqttClient {
public:
    using ConnectHandler = std::function<void()>;
    using MessageHandler = std::function<void(const std::string& topic, const std::string& payload)>;

    explicit MqttClient(const std::string& client_id);
    ~MqttClient();

    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    void set_credentials(const std::string& user, const std::string& password);
    void set_will(const std::string& topic, const std::string& payload, bool retain);

    void on_connect(ConnectHandler handler) { connect_handler_ = std::move(handler); }
    void on_message(MessageHandler handler) { message_handler_ = std::move(handler); }

    void connect(const std::string& host, int port);

    // Verwerkt netwerkverkeer en verbindt opnieuw als de verbinding weg is.
    void loop(int timeout_ms);

    void publish(const std::string& topic, const std::string& payload, bool retain = false);
    void subscribe(const std::string& topic);

private:
    static void handle_connect(mosquitto*, void* self, int rc);
    static void handle_message(mosquitto*, void* self, const mosquitto_message* msg);

    mosquitto* mosq_ = nullptr;
    ConnectHandler connect_handler_;
    MessageHandler message_handler_;
    std::chrono::steady_clock::time_point next_reconnect_{};
};
