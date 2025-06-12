#pragma once

#include <vector>
#include <iostream>
#include <vector>
#include <bitset>
#include <optional>
#include <functional>

namespace esphome
{
    namespace samsung_ac
    {
        static const char *TAG = "samsung_ac";

        std::string long_to_hex(long number);
        int hex_to_int(const std::string &hex);
        std::string bytes_to_hex(const std::vector<uint8_t> &data);
        std::vector<uint8_t> hex_to_bytes(const std::string &hex);
        void print_bits_8(uint8_t value);
    } // namespace samsung_ac
} // namespace esphome
