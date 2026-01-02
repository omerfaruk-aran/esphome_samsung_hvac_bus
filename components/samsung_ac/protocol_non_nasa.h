#pragma once

#include <list>
#include <vector>
#include <optional>
#include "protocol.h"
#include "util.h"

namespace esphome
{
    namespace samsung_ac
    {
        enum class NonNasaFanspeed : uint8_t
        {
            Auto = 0,
            Low = 2,
            Medium = 4,
            High = 5,
            Fresh = 6
        };

        enum class NonNasaMode : uint8_t
        {
            Heat = 0x01,
            Cool = 0x02,
            Dry = 0x04,
            Fan = 0x08,
            Auto_Heat = 0x21,
            Auto = 0x22
        };

        enum class NonNasaWindDirection : uint8_t
        {
            Vertical = 26,
            Horizontal = 27,
            FourWay = 28,
            Stop = 31
        };

        struct NonNasaCommand20 // from indoor units
        {
            uint8_t target_temp = 0;
            uint8_t room_temp = 0;
            uint8_t pipe_in = 0;
            uint8_t pipe_out = 0;

            NonNasaFanspeed fanspeed = NonNasaFanspeed::Auto;
            NonNasaMode mode = NonNasaMode::Heat;
            NonNasaWindDirection wind_direction = NonNasaWindDirection::Stop;

            bool power = false;

            std::string to_string();
        };

        struct NonNasaCommandC0 // from outdoor unit
        {
            uint8_t outdoor_unit_operation_mode = 0;
            bool outdoor_unit_4_way_valve = false;
            bool outdoor_unit_hot_gas_bypass = false;
            bool outdoor_unit_compressor = false;
            bool outdoor_unit_ac_fan = false;
            uint8_t outdoor_unit_outdoor_temp_c = 0;
            uint8_t outdoor_unit_discharge_temp_c = 0;
            uint8_t outdoor_unit_condenser_mid_temp_c = 0;

            std::string to_string();
        };

        struct NonNasaCommandC1 // from outdoor unit
        {
            uint8_t outdoor_unit_sump_temp_c = 0;

            std::string to_string();
        };

        struct NonNasaCommandC6
        {
            bool control_status = false;
            std::string to_string()
            {
                return "control_status:" + std::to_string(control_status);
            };
        };

        struct NonNasaCommandF0 // from outdoor unit
        {
            bool outdoor_unit_freeze_protection = false;
            bool outdoor_unit_heating_overload = false;
            bool outdoor_unit_defrost_control = false;
            bool outdoor_unit_discharge_protection = false;
            bool outdoor_unit_current_control = false;
            uint8_t inverter_order_frequency_hz = 0;
            uint8_t inverter_target_frequency_hz = 0;
            uint8_t inverter_current_frequency_hz = 0;
            bool outdoor_unit_bldc_fan = false;
            uint8_t outdoor_unit_error_code = 0;

            std::string to_string();
        };

        struct NonNasaCommand8D // from outdoor unit
        {
            float inverter_current_a = 0;
            float inverter_voltage_v = 0;
            float inverter_power_w = 0;

            std::string to_string();
        };

        struct NonNasaCommandF1 // from outdoor unit
        {
            uint16_t outdoor_unit_EEV_A = 0;
            uint16_t outdoor_unit_EEV_B = 0;
            uint16_t outdoor_unit_EEV_C = 0;
            uint16_t outdoor_unit_EEV_D = 0;

            std::string to_string();
        };

        struct NonNasaCommandF3 // from outdoor unit
        {
            uint8_t inverter_max_frequency_hz = 0;
            float inverter_total_capacity_requirement_kw = 0;
            float inverter_current_a = 0;
            float inverter_voltage_v = 0;
            float inverter_power_w = 0;

            std::string to_string();
        };

        struct NonNasaCommandRaw
        {
            uint8_t length;
            uint8_t data[16 - 4 - 1];

            std::string to_string()
            {
                std::vector<uint8_t> vec(std::begin(data), std::begin(data) + length);
                return bytes_to_hex(vec);
            };
        };

        enum class NonNasaCommand : uint8_t
        {
            Cmd20 = 0x20,
            Cmd54 = 0x54,
            Cmd8D = 0x8d,
            CmdC0 = 0xc0,
            CmdC1 = 0xc1,
            CmdC6 = 0xc6,
            CmdF0 = 0xf0,
            CmdF1 = 0xf1,
            CmdF3 = 0xf3,
            CmdF8 = 0xF8,
        };

        struct NonNasaDataPacket
        {
            std::string src;
            std::string dst;

            NonNasaCommand cmd;

            NonNasaDataPacket()
            {
            }

            union
            {
                NonNasaCommand20 command20;
                NonNasaCommandRaw command54; // Control message ack
                NonNasaCommand8D command8D;
                NonNasaCommandC0 commandC0;
                NonNasaCommandC1 commandC1;
                NonNasaCommandC6 commandC6;
                NonNasaCommandF0 commandF0;
                NonNasaCommandF1 commandF1;
                NonNasaCommandF3 commandF3;
                NonNasaCommandRaw commandF8; // Unknown structure for now
                NonNasaCommandRaw commandRaw;
            };

            DecodeResult decode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct NonNasaRequest
        {
            std::string dst;

            uint8_t room_temp = 0;
            uint8_t target_temp = 0;
            NonNasaFanspeed fanspeed = NonNasaFanspeed::Auto;
            NonNasaMode mode = NonNasaMode::Heat;
            bool power = false;
            NonNasaWindDirection wind_direction = NonNasaWindDirection::Stop;

            std::vector<uint8_t> encode();
            std::string to_string();

            static NonNasaRequest create(std::string dst_address);
        };

        struct NonNasaRequestQueueItem
        {
            NonNasaRequest request;
            uint32_t time;
            uint32_t time_sent;
            uint8_t retry_count;
            uint8_t resend_count;
        };

        extern std::list<NonNasaRequestQueueItem> nonnasa_requests;
        extern bool controller_registered;
        extern bool indoor_unit_awake;

        NonNasaWindDirection swingmode_to_wind_direction(SwingMode swing);
        uint8_t encode_request_wind_direction(NonNasaWindDirection wind_dir);

        DecodeResult try_decode_non_nasa_packet(std::vector<uint8_t> &data);
        void process_non_nasa_packet(MessageTarget *target);

        class NonNasaProtocol : public Protocol
        {
        public:
            NonNasaProtocol() = default;

            void publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request) override;
            void protocol_update(MessageTarget *target) override;
        };
    } // namespace samsung_ac
} // namespace esphome
