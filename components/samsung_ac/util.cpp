#include "util.h"

namespace esphome
{
    namespace samsung_ac
    {
        std::string long_to_hex(long number)
        {
            char str[10];
            snprintf(str, sizeof(str), "%02lx", number); // Use of snprintf for security reasons.
            return std::string(str);
        }

        int hex_to_int(const std::string &hex)
        {
            return (int)strtol(hex.c_str(), NULL, 16);
        }

        std::string bytes_to_hex(const std::vector<uint8_t> &data)
        {
            return bytes_to_hex(data, 0, data.size());
        }

        std::string bytes_to_hex(const std::vector<uint8_t> &data, uint16_t start, uint16_t end)
        {

            std::string str;
            str.reserve((end - start) * 2); // Memory reservations are made to increase efficiency.
            for (int i = start; i < end; i++)
            {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02x", data[i]);
                str += buf;
            }
            return str;
        }

        std::vector<uint8_t> hex_to_bytes(const std::string &hex)
        {
            std::vector<uint8_t> bytes;
            bytes.reserve(hex.length() / 2);
            for (size_t i = 0; i < hex.length(); i += 2)
            {
                uint8_t byte = (uint8_t)strtol(hex.substr(i, 2).c_str(), nullptr, 16);
                bytes.push_back(byte);
            }
            return bytes;
        }

        void print_bits_8(uint8_t value)
        {
            std::cout << std::bitset<8>(value) << std::endl;
        }
    } // namespace samsung_ac
} // namespace esphome
