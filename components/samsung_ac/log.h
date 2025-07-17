#pragma once

#include "esphome/core/log.h"
#include "util.h"

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

    } // namespace samsung_ac
} // namespace esphome