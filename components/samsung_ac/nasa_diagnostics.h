#pragma once

#include <cstddef>
#include <string>

namespace esphome
{
    namespace samsung_ac
    {
        struct Packet;
        struct MessageSet;

        std::string format_nasa_diagnostic_scalar(const Packet &packet, const MessageSet &message, std::size_t index);
        void log_nasa_diagnostics(const Packet &packet);
    } // namespace samsung_ac
} // namespace esphome
