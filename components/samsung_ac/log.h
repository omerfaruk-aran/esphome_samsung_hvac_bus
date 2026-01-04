// log.h
#pragma once

#include "esphome/core/log.h"
#include "util.h"
#include <string>
#include <cstdint>

#define LOGV(...) ESP_LOGV(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGC(...) ESP_LOGCONFIG(TAG, __VA_ARGS__)

#define LOG_STATE(...) LOGI(__VA_ARGS__)
#define LOG_RAW_SEND(inter, ...) ({ if (debug_log_raw_bytes) LOGW("<< +%d: %s", inter, bytes_to_hex(__VA_ARGS__).c_str()); })
#define LOG_RAW(inter, ...) ({ if (debug_log_raw_bytes) LOGD(">> +%d: %s", inter, bytes_to_hex(__VA_ARGS__).c_str()); })
#define LOG_RAW_DISCARDED(inter, ...) ({if (debug_log_raw_bytes) LOGV(">> +%d: %s", inter, bytes_to_hex(__VA_ARGS__).c_str()); })
#define LOG_PACKET_SEND(msg, packet) ({ if (debug_log_messages) LOGI("%s: %s", msg, packet.to_string().c_str()); })
#define LOG_PACKET_RECV(msg, packet) ({ if (debug_log_messages) LOGD("%s: %s", msg, packet.to_string().c_str()); })

namespace esphome
{
    namespace samsung_ac
    {

        extern bool debug_log_messages;
        extern bool debug_log_undefined_messages;
        extern bool debug_log_raw_bytes;

        // NEW:
        extern bool debug_log_messages_on_change;

        // Dedup helpers
        uint32_t log_dedup_key(const std::string &source, const std::string &dest, uint16_t id);
        uint32_t fnv1a_32(const char *data, size_t len);
        uint32_t fnv1a_32(const std::string &s);

        // Returns true if we should print (first time / changed / optional interval)
        bool log_should_print(uint32_t key, double value, double epsilon = 0.0, uint32_t min_interval_ms = 0);

    } // namespace samsung_ac
} // namespace esphome
