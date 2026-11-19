#pragma once
// Fake Log for Local Testing

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <string>

namespace esphome
{

inline std::function<void(const char *, const char *, const std::string &)> test_log_sink;

inline void capture_test_log(const char *level, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    const int length = std::vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    if (length >= 0)
    {
        std::string line(static_cast<std::size_t>(length), '\0');
        std::vsnprintf(line.data(), line.size() + 1, format, args);
        va_end(args);
        test_log_sink(level, tag, line);
        return;
    }
    va_end(args);
}

#define ESP_LOG(level, TAG, format, ...)                                    \
    do                                                                     \
    {                                                                      \
        if (::esphome::test_log_sink)                                       \
            ::esphome::capture_test_log(level, TAG, format, ##__VA_ARGS__);  \
        else                                                               \
        {                                                                  \
            std::string str = "[";                                         \
            str += level;                                                  \
            str += "] ";                                                   \
            str += format;                                                 \
            str += "\n";                                                   \
            std::printf(str.c_str(), ##__VA_ARGS__);                        \
        }                                                                  \
    } while (0);

#define ESP_LOGD(TAG, format, ...) ESP_LOG("DEBUG", TAG, format, ##__VA_ARGS__)
#define ESP_LOGE(TAG, format, ...) ESP_LOG("ERROR", TAG, format, ##__VA_ARGS__)
#define ESP_LOGW(TAG, format, ...) ESP_LOG("WARN", TAG, format, ##__VA_ARGS__)
#define ESP_LOGV(TAG, format, ...) ESP_LOG("VERBOSE", TAG, format, ##__VA_ARGS__)
#define ESP_LOGI(TAG, format, ...) ESP_LOG("INFO", TAG, format, ##__VA_ARGS__)

} // namespace esphome
