#include "esphome/core/log.h"
#include "debug_mqtt.h"

#if defined(USE_ESP8266)
#include <AsyncMqttClient.h>
static AsyncMqttClient *mqtt_client{nullptr};
#endif

#if defined(USE_ESP32) && defined(SAMSUNG_AC_DEBUG_MQTT)
#include <mqtt_client.h>
static esp_mqtt_client_handle_t mqtt_client{nullptr};
#endif

namespace esphome
{
namespace samsung_ac
{
    bool debug_mqtt_connected()
    {
#if defined(USE_ESP8266)
        if (mqtt_client == nullptr)
            return false;
        return mqtt_client->connected();

#elif defined(USE_ESP32) && defined(SAMSUNG_AC_DEBUG_MQTT)
        if (mqtt_client == nullptr)
            return false;

        return true;

#else
        return false;
#endif
    }

    void debug_mqtt_connect(const std::string &host, const uint16_t port,
                            const std::string &username, const std::string &password)
    {
        if (host.empty())
            return;

#if defined(USE_ESP8266)
        if (mqtt_client == nullptr)
        {
            mqtt_client = new AsyncMqttClient();
            mqtt_client->setServer(host.c_str(), port);
            if (!username.empty())
                mqtt_client->setCredentials(username.c_str(), password.c_str());
        }

        if (!mqtt_client->connected())
            mqtt_client->connect();

#elif defined(USE_ESP32) && defined(SAMSUNG_AC_DEBUG_MQTT)
        if (mqtt_client == nullptr)
        {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
            std::string uri = "mqtt://" + host + ":" + std::to_string(port);
            esp_mqtt_client_config_t mqtt_cfg = {};
            mqtt_cfg.broker.address.uri = uri.c_str();
            if (!username.empty())
            {
                mqtt_cfg.credentials.username = username.c_str();
                mqtt_cfg.credentials.authentication.password = password.c_str();
            }
#else
            esp_mqtt_client_config_t mqtt_cfg = {};
            mqtt_cfg.host = host.c_str();
            mqtt_cfg.port = port;
            if (!username.empty())
            {
                mqtt_cfg.username = username.c_str();
                mqtt_cfg.password = password.c_str();
            }
#endif
            mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_start(mqtt_client);
        }
#else
        (void)port;
        (void)username;
        (void)password;
#endif
    }

    bool debug_mqtt_publish(const std::string &topic, const std::string &payload)
    {
#if defined(USE_ESP8266)
        if (mqtt_client == nullptr)
            return false;
        return mqtt_client->publish(topic.c_str(), 0, false, payload.c_str()) != 0;

#elif defined(USE_ESP32) && defined(SAMSUNG_AC_DEBUG_MQTT)
        if (mqtt_client == nullptr)
            return false;
        return esp_mqtt_client_publish(mqtt_client, topic.c_str(), payload.c_str(),
                                       payload.length(), 0, false) != -1;
#else
        return false;
#endif
    }

} // namespace samsung_ac
} // namespace esphome
