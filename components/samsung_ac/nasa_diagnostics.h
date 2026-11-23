#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <variant>

namespace esphome
{
    namespace samsung_ac
    {
        struct Packet;
        struct MessageSet;

        struct NasaServiceValue
        {
            std::variant<float, bool, std::string> state;
        };

        std::optional<NasaServiceValue> decode_nasa_service_telemetry(const Packet &packet, std::size_t index);
        std::string format_nasa_diagnostic_scalar(const Packet &packet, const MessageSet &message, std::size_t index);
        void log_nasa_diagnostics(const Packet &packet);
    } // namespace samsung_ac
} // namespace esphome
