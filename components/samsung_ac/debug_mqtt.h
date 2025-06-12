#pragma once

#include <string>

namespace esphome
{
    namespace samsung_ac
    {
        bool debug_mqtt_connected();
        void debug_mqtt_connect(const std::string &host, const uint16_t port, const std::string &username, const std::string &password);
        bool debug_mqtt_publish(const std::string &topic, const std::string &payload);
    } // namespace samsung_ac
} // namespace esphome
