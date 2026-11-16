#pragma once

#include "MqttClient.h"
#include "KakuTransmitter.h"

class MqttKaku : public MqttClient
{
public:
    MqttKaku(const std::string &client_id, KakuTransmitter &kaku);

    void publish_discovery();

protected:
    void connected() override;
    void message_received(const std::string &topic, const std::string &payload) override;

private:
    KakuTransmitter &kaku;

    std::string button_config(const std::string &id, const std::string &name, const std::string &icon);
    std::string switch_config(int channel);
    std::string switch_topic(int channel);
};