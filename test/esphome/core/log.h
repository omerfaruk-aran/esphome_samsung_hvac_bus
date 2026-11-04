#pragma once
// Fake Log for Local Testing

namespace esphome
{

#define ESP_LOG(level, TAG, format, ...)      \
    do                                        \
    {                                         \
        std::string str = "[";                \
        str += level;                         \
        str += "] ";                          \
        str += format;                        \
        str += "\n";                          \
        printf((str.c_str()), ##__VA_ARGS__); \
    } while (0);

#define ESP_LOGD(TAG, format, ...) ESP_LOG("DEBUG", TAG, format, ##__VA_ARGS__)
#define ESP_LOGE(TAG, format, ...) ESP_LOG("ERROR", TAG, format, ##__VA_ARGS__)
#define ESP_LOGW(TAG, format, ...) ESP_LOG("WARN", TAG, format, ##__VA_ARGS__)
#define ESP_LOGV(TAG, format, ...) ESP_LOG("VERBOSE", TAG, format, ##__VA_ARGS__)
#define ESP_LOGVV(TAG, format, ...) ESP_LOG("VERY_VERBOSE", TAG, format, ##__VA_ARGS__)
#define ESP_LOGI(TAG, format, ...) ESP_LOG("INFO", TAG, format, ##__VA_ARGS__)

} // namespace esphome
