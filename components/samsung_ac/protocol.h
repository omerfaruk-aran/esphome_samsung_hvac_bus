#pragma once

#include <set>
#include "esphome/core/optional.h"
#include "util.h"

namespace esphome
{
    namespace samsung_ac
    {
        extern bool non_nasa_keepalive;

        enum class DecodeResultType
        {
            Fill = 1,
            Discard = 2,
            Processed = 3
        };

        struct DecodeResult
        {
            DecodeResultType type;
            uint16_t bytes; // when Processed
        };

        enum class Mode
        {
            Unknown = -1,
            Auto = 0,
            Cool = 1,
            Dry = 2,
            Fan = 3,
            Heat = 4,
        };

        enum class WaterHeaterMode
        {
            Unknown = -1,
            Eco = 0,
            Standard = 1,
            Power = 2,
            Force = 3,
        };

        enum class FanMode
        {
            Unknown = -1,
            Auto = 0,
            Low = 1,
            Mid = 2,
            High = 3,
            Turbo = 4,
            Off = 5
        };

        typedef std::string AltModeName;
        typedef uint8_t AltMode;

        struct AltModeDesc
        {
            AltModeName name;
            AltMode value;
        };

        enum class SwingMode : uint8_t
        {
            Fix = 0,
            Vertical = 1,
            Horizontal = 2,
            All = 3
        };

        class MessageTarget
        {
        public:
            virtual uint32_t get_miliseconds() = 0;
            virtual void publish_data(uint8_t id, std::vector<uint8_t> &&data) = 0;
            virtual void ack_data(uint8_t id) = 0;
            virtual void register_address(const std::string address) = 0;
            virtual void set_power(const std::string address, bool value) = 0;
            virtual void set_automatic_cleaning(const std::string address, bool value) = 0;
            virtual void set_water_heater_power(const std::string address, bool value) = 0;
            virtual void set_room_temperature(const std::string address, float value) = 0;
            virtual void set_target_temperature(const std::string address, float value) = 0;
            virtual void set_water_outlet_target(const std::string address, float value) = 0;
            virtual void set_outdoor_temperature(const std::string address, float value) = 0;
            virtual void set_indoor_eva_in_temperature(const std::string address, float value) = 0;
            virtual void set_indoor_eva_out_temperature(const std::string address, float value) = 0;
            virtual void set_target_water_temperature(const std::string address, float value) = 0;
            virtual void set_mode(const std::string address, Mode mode) = 0;
            virtual void set_water_heater_mode(const std::string address, WaterHeaterMode waterheatermode) = 0;
            virtual void set_fanmode(const std::string address, FanMode fanmode) = 0;
            virtual void set_altmode(const std::string address, AltMode altmode) = 0;
            virtual void set_swing_vertical(const std::string address, bool vertical) = 0;
            virtual void set_swing_horizontal(const std::string address, bool horizontal) = 0;
            virtual void set_custom_sensor(const std::string address, uint16_t message_number, float value) = 0;
            virtual void set_error_code(const std::string address, int error_code) = 0;
            virtual void set_outdoor_instantaneous_power(const std::string &address, float value) = 0;
            virtual void set_outdoor_cumulative_energy(const std::string &address, float value) = 0;
            virtual void set_outdoor_current(const std::string &address, float value) = 0;
            virtual void set_outdoor_voltage(const std::string &address, float value) = 0;
        };

        struct ProtocolRequest
        {
        public:
            optional<bool> power;
            optional<bool> automatic_cleaning;
            optional<bool> water_heater_power;
            optional<Mode> mode;
            optional<WaterHeaterMode> waterheatermode;
            optional<float> target_temp;
            optional<float> water_outlet_target;
            optional<float> target_water_temp;
            optional<FanMode> fan_mode;
            optional<SwingMode> swing_mode;
            optional<AltMode> alt_mode;
        };

        class Protocol
        {
        public:
            virtual void publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request) = 0;
            virtual void protocol_update(MessageTarget *target) = 0;
        };

        enum class ProtocolProcessing
        {
            Auto = 0,
            NASA = 1,
            NonNASA = 2
        };

        extern ProtocolProcessing protocol_processing;

        DecodeResult process_data(std::vector<uint8_t> &data, MessageTarget *target);

        Protocol *get_protocol(const std::string &address);

        bool is_nasa_address(const std::string &address);

        enum class AddressType
        {
            Outdoor = 0,
            Indoor = 1,
            Other = 2
        };

        AddressType get_address_type(const std::string &address);

    } // namespace samsung_ac
} // namespace esphome
