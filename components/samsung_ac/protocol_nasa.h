#pragma once

#include <vector>
#include <map>
#include "protocol.h"

namespace esphome
{
    namespace samsung_ac
    {
        enum class AddressClass : uint8_t
        {
            Outdoor = 0x10,
            HTU = 0x11,
            Indoor = 0x20,
            ERV = 0x30,
            Diffuser = 0x35,
            MCU = 0x38,
            RMC = 0x40,
            WiredRemote = 0x50,
            PIM = 0x58,
            SIM = 0x59,
            Peak = 0x5A,
            PowerDivider = 0x5B,
            OnOffController = 0x60,
            WiFiKit = 0x62,
            CentralController = 0x65,
            DMS = 0x6A,
            JIGTester = 0x80,
            BroadcastSelfLayer = 0xB0,
            BroadcastControlLayer = 0xB1,
            BroadcastSetLayer = 0xB2,
            BroadcastControlAndSetLayer = 0xB3,
            BroadcastModuleLayer = 0xB4,
            BroadcastCSM = 0xB7,
            BroadcastLocalLayer = 0xB8,
            BroadcastCSML = 0xBF,
            Undefined = 0xFF,
        };

        enum class PacketType : uint8_t
        {
            StandBy = 0,
            Normal = 1,
            Gathering = 2,
            Install = 3,
            Download = 4
        };

        enum class DataType : uint8_t
        {
            Undefined = 0,
            Read = 1,
            Write = 2,
            Request = 3,
            Notification = 4,
            Response = 5,
            Ack = 6,
            Nack = 7
        };

        enum MessageSetType : uint8_t
        {
            Enum = 0,
            Variable = 1,
            LongVariable = 2,
            Structure = 3
        };

        enum class MessageNumber : uint16_t
        {
            // Keep both spellings for backward compatibility
            Undefined = 0x0000,
            Undefiend = Undefined,

            // ===== Existing core mappings =====
            ENUM_in_operation_power = 0x4000,
            ENUM_in_operation_mode = 0x4001,
            ENUM_in_fan_mode = 0x4006, // Did not exists in xml...only in Remocon.dll code
            ENUM_in_fan_mode_real = 0x4007,

            ENUM_in_louver_hl_swing = 0x4011,
            ENUM_in_louver_lr_swing = 0x407e,

            ENUM_in_state_humidity_percent = 0x4038,

            ENUM_in_alt_mode = 0x4060,
            ENUM_in_water_heater_power = 0x4065,
            ENUM_in_water_heater_mode = 0x4066,

            ENUM_in_operation_automatic_cleaning = 0x4111,

            VAR_in_temp_target_f = 0x4201,
            VAR_in_temp_room_f = 0x4204,
            VAR_in_temp_eva_in_f = 0x4205,
            VAR_in_temp_eva_out_f = 0x4206,

            VAR_in_temp_water_heater_target_f = 0x4235,
            VAR_in_temp_water_tank_f = 0x4237,
            VAR_in_temp_water_outlet_target_f = 0x4247,

            VAR_out_sensor_airout = 0x8204,
            VAR_OUT_SENSOR_CT1 = 0x8217,

            VAR_out_error_code = 0x8235,

            VAR_out_load_fanrpm1 = 0x823D,
            VAR_out_load_fanrpm2 = 0x823E,

            // Power/Energy (existing)
            LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM = 0x8413,
            LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM = 0x8414,

            // Voltage (existing)
            LVAR_NM_OUT_SENSOR_VOLTAGE = 0x24fc,

            // --- Debug-only / extra enums used in process_messageset_debug() ---
            ENUM_IN_OPERATION_VENT_POWER = 0x4003,
            ENUM_IN_OPERATION_VENT_MODE = 0x4004,

            ENUM_in_louver_hl_part_swing = 0x4012,

            ENUM_IN_QUIET_MODE = 0x406e,

            ENUM_IN_OPERATION_POWER_ZONE1 = 0x4119,
            ENUM_IN_OPERATION_POWER_ZONE2 = 0x411e,

            ENUM_in_operation_mode_real = 0x4002,
            ENUM_in_fan_vent_mode = 0x4008,

            VAR_in_capacity_request = 0x4211,

            // Outdoor enums used in debug
            ENUM_out_operation_odu_mode = 0x8001,
            ENUM_out_operation_heatcool = 0x8003,
            ENUM_out_load_4way = 0x801a,

            // --- Pipe sensors & outdoor misc used in debug ---
            VAR_OUT_SENSOR_PIPEIN3 = 0x8261,
            VAR_OUT_SENSOR_PIPEIN4 = 0x8262,
            VAR_OUT_SENSOR_PIPEIN5 = 0x8263,
            VAR_OUT_SENSOR_PIPEOUT1 = 0x8264,
            VAR_OUT_SENSOR_PIPEOUT2 = 0x8265,
            VAR_OUT_SENSOR_PIPEOUT3 = 0x8266,
            VAR_OUT_SENSOR_PIPEOUT4 = 0x8267,
            VAR_OUT_SENSOR_PIPEOUT5 = 0x8268,

            VAR_out_control_order_cfreq_comp2 = 0x8274,
            VAR_out_control_target_cfreq_comp2 = 0x8275,

            VAR_out_sensor_top1 = 0x8280,
            VAR_OUT_PHASE_CURRENT = 0x82db,

            VAR_OUT_PROJECT_CODE = 0x82bc,
            VAR_OUT_PRODUCT_OPTION_CAPA = 0x82e3,

            // --- Your “FSV” variables (used in process_messageset default switch) ---
            VAR_IN_FSV_3021 = 0x4260,
            VAR_IN_FSV_3022 = 0x4261,
            VAR_IN_FSV_3023 = 0x4262,

            // --- Additional wattmeter/energy ids (used in process_messageset default switch) ---
            NASA_OUTDOOR_CONTROL_WATTMETER_1UNIT = 0x8411,
            NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM = 0x8415,
            NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM_ACCUM = 0x8416,

            actual_produced_energy = 0x8426,
            total_produced_energy = 0x8427,

            VAR_IN_DUST_SENSOR_PM10_0_VALUE = 0x42d1,
            VAR_IN_DUST_SENSOR_PM2_5_VALUE = 0x42d2,
            VAR_IN_DUST_SENSOR_PM1_0_VALUE = 0x42d3,

        };

        struct Address
        {
            AddressClass klass;
            uint8_t channel;
            uint8_t address;
            uint8_t size = 3;

            static Address parse(const std::string &str);
            static Address get_my_address();

            void decode(std::vector<uint8_t> &data, unsigned int index);
            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Command
        {
            bool packetInformation = true;
            uint8_t protocolVersion = 2;
            uint8_t retryCount = 0;
            PacketType packetType = PacketType::StandBy;
            DataType dataType = DataType::Undefined;
            uint8_t packetNumber = 0;

            uint8_t size = 3;

            void decode(std::vector<uint8_t> &data, unsigned int index);
            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Buffer
        {
            uint8_t size;
            uint8_t data[255];
        };

        struct MessageSet
        {
            MessageNumber messageNumber = MessageNumber::Undefiend;
            MessageSetType type = Enum;
            union
            {
                long value;
                Buffer structure;
            };
            uint16_t size = 2;

            MessageSet(MessageNumber messageNumber)
            {
                this->messageNumber = messageNumber;
                // this->deviceType = (NMessageSet.DeviceType) (((int) messageNumber & 57344) >> 13);
                this->type = (MessageSetType)(((uint32_t)messageNumber & 1536) >> 9);
                // this->_msgIndex = (ushort) ((uint) messageNumber & 511U);
            }

            static MessageSet decode(std::vector<uint8_t> &data, unsigned int index, int capacity);

            void encode(std::vector<uint8_t> &data);
            std::string to_string();
        };

        struct Packet
        {
            Address sa;
            Address da;
            Command command;
            std::vector<MessageSet> messages;

            static Packet create(Address da, DataType dataType, MessageNumber messageNumber, int value);
            static Packet createa_partial(Address da, DataType dataType);

            DecodeResult decode(std::vector<uint8_t> &data);
            std::vector<uint8_t> encode();
            std::string to_string();
        };

        DecodeResult try_decode_nasa_packet(std::vector<uint8_t> &data);
        void process_nasa_packet(MessageTarget *target);

        class NasaProtocol : public Protocol
        {
        public:
            NasaProtocol() = default;

            void publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request) override;
            void protocol_update(MessageTarget *target) override;

        protected:
            std::map<std::string, ProtocolRequest> outgoing_queue_; // std::string address -> ProtocolRequest
        };

    } // namespace samsung_ac
} // namespace esphome
