#pragma once

#include <vector>
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
            Undefiend = 0,
            ENUM_in_operation_power = 0x4000,
            ENUM_in_operation_automatic_cleaning = 0x4111,
            ENUM_in_water_heater_power = 0x4065,
            ENUM_in_operation_mode = 0x4001,
            ENUM_in_water_heater_mode = 0x4066,
            ENUM_in_fan_mode = 0x4006,
            ENUM_in_fan_mode_real = 0x4007,
            ENUM_in_alt_mode = 0x4060,
            ENUM_in_louver_hl_swing = 0x4011,
            ENUM_in_louver_lr_swing = 0x407e,
            ENUM_in_state_humidity_percent = 0x4038,
            VAR_in_temp_room_f = 0x4203,
            VAR_in_temp_target_f = 0x4201,
            VAR_in_temp_water_outlet_target_f = 0x4247,
            VAR_in_temp_water_tank_f = 0x4237,
//            VAR_out_sensor_airout = 0x8204,       // removed 
            VAR_in_temp_water_heater_target_f = 0x4235,
            VAR_in_temp_eva_in_f = 0x4205,
            VAR_in_temp_eva_out_f = 0x4206,
            VAR_out_error_code = 0x8235,
            
            // New mappings for undefined messages
            VAR_out_power_status = 0x823f,           // Power status
            VAR_out_operation_mode = 0x8248,         // Operation mode
            VAR_out_operation_state = 0x8249,        // Operation state
            ENUM_out_operation_mode = 0x8001,        // Operation mode enum
            ENUM_out_heat_cool_mode = 0x8003,        // Heat/Cool mode
            VAR_out_compressor_freq = 0x8244,        // Compressor frequency
            VAR_out_fan_speed = 0x8247,              // Fan speed
            ENUM_out_fan_mode = 0x8061,              // Fan mode
            ENUM_out_compressor_state = 0x8062,      // Compressor state
            ENUM_out_defrost_mode = 0x8075,          // Defrost mode
            ENUM_out_4way_valve = 0x805e,            // 4-way valve state
            VAR_out_pipe_temp = 0x8204,              // Pipe temperature
            VAR_out_discharge_temp = 0x820a,         // Discharge line temperature (0.1°C)
            VAR_out_suction_temp = 0x821a,           // Suction line refrigerant temperature (0.1°C)
            VAR_out_ambient_temp = 0x821c,           // Ambient temperature
            VAR_out_condenser_temp = 0x821e,         // Condenser temperature
            VAR_out_evaporator_temp = 0x8220,        // Evaporator temperature
            VAR_out_compressor_current = 0x8217,     // Compressor electrical current (0.1A)
            //VAR_out_compressor_power = 0x8223,       // Compressor power consumption
            VAR_out_fan_current = 0x8225,            // Fan current
            VAR_out_fan_power = 0x8227,              // Fan power consumption
            VAR_out_total_power = 0x8228,            // Total power consumption
            VAR_out_energy_accumulated = 0x8414,     // Accumulated energy consumption (kWh)
            VAR_out_operation_time = 0x822a,         // Operation time
            VAR_out_error_count = 0x822b,            // Error count
            VAR_out_system_pressure = 0x8224,        // System pressure
            VAR_out_phase_current = 0x82db,          // Phase current
            VAR_out_phase_voltage = 0x82de,          // Phase voltage
            VAR_out_phase_power = 0x82df,            // Phase power
            VAR_out_compressor_speed = 0x82e0,       // Compressor speed
            VAR_out_fan_speed_rpm = 0x82e3,          // Fan speed in RPM
            VAR_out_power_factor = 0x82e8,           // Power factor
            VAR_out_efficiency = 0x82ed,             // System efficiency
            VAR_out_capacity = 0x82f6,               // System capacity
            VAR_out_operation_count = 0x822c,        // Operation count
            LVAR_out_energy_total = 0x8408,          // Total energy consumption
            LVAR_out_operation_hours = 0x840b,       // Total operation hours
            LVAR_out_compressor_hours = 0x840c,      // Compressor operation hours
            LVAR_out_fan_hours = 0x841a,             // Fan operation hours
            
            // Indoor unit mappings
            VAR_in_pipe_temp = 0x4204,               // Indoor pipe temperature
            VAR_in_ambient_temp = 0x420c,            // Indoor ambient temperature
            VAR_in_operation_state = 0x4217,         // Indoor operation state
            VAR_in_water_temp = 0x4236,              // Water temperature
            VAR_in_water_flow = 0x4238,              // Water flow
            VAR_in_water_pressure = 0x4239,          // Water pressure
            VAR_in_water_valve = 0x423e,             // Water valve state
            VAR_in_power_consumption = 0x427f,       // Power consumption
            VAR_in_water_level = 0x428c,             // Water level
            VAR_in_water_quality = 0x42d4,           // Water quality
            VAR_in_water_filter = 0x42d8,            // Water filter status
            VAR_in_water_pump = 0x42d9,              // Water pump status
            VAR_in_water_heater = 0x42e8,            // Water heater status
            VAR_in_water_heater_temp = 0x42e9,       // Water heater temperature
            LVAR_in_energy_total = 0x4401,           // Total energy consumption
            LVAR_in_operation_hours = 0x4426,        // Total operation hours
            LVAR_in_water_heater_hours = 0x4427,     // Water heater operation hours
            LVAR_NM_OUT_SENSOR_VOLTAGE = 0x24fc,
            LVAR_OUT_CONTROL_WATTMETER_1UNIT = 0x8411,
            LVAR_OUT_CONTROL_WATTMETER_TOTAL_SUM = 0x8415,
            LVAR_OUT_CONTROL_WATTMETER_TOTAL_SUM_ACCUM = 0x8416,
            LVAR_OUT_CONTROL_WATTMETER_TOTAL_PRODUCED = 0x8427,
            VAR_OUT_SENSOR_PIPEIN3 = 0x8261,
            VAR_OUT_SENSOR_PIPEIN4 = 0x8262,
            VAR_OUT_SENSOR_PIPEIN5 = 0x8263,
            VAR_OUT_SENSOR_PIPEOUT1 = 0x8264,
            ENUM_IN_OPERATION_VENT_POWER = 0x4003,
            ENUM_IN_OPERATION_VENT_MODE = 0x4004,
            ENUM_IN_LOUVER_HL_PART_SWING = 0x4012,
            ENUM_IN_QUIET_MODE = 0x406E,
            ENUM_IN_OPERATION_POWER_ZONE1 = 0x4119,
            ENUM_IN_OPERATION_POWER_ZONE2 = 0x411E,
            ENUM_IN_OPERATION_MODE_REAL = 0x4002,
            ENUM_IN_FAN_VENT_MODE = 0x4008,
            ENUM_OUT_LOAD_4WAY = 0x801a,
            VAR_IN_CAPACITY_REQUEST = 0x4211,
            // --- Unmapped/Unknown message numbers from logs ---
            VAR_out_high_pressure = 0x8206,          // High pressure (0.1 bar)
            VAR_out_low_pressure = 0x8208,           // Low pressure (0.1 bar)
            VAR_out_heat_exchanger_outlet = 0x8218,  // Heat exchanger outlet temperature (0.1°C)
            VAR_out_ipm_temperature = 0x8254,       // IPM temperature
            VAR_out_compressor_top_temp = 0x8280,   // Compressor top temperature (0.1°C)
            VAR_out_high_side_sat_temp = 0x829f,    // High side saturation temperature (0.1°C)
            VAR_out_low_side_sat_temp = 0x82a0,     // Low side saturation temperature (0.1°C)
            VAR_out_power = 0x8413,                 // Outdoor unit power consumption (W)
            
            // Still unknown messages
            VAR_unknown_8245 = 0x8245,
            VAR_unknown_825a = 0x825a,
            VAR_unknown_825b = 0x825b,
            VAR_unknown_825d = 0x825d,
            VAR_unknown_827a = 0x827a,
            VAR_unknown_829a = 0x829a,
            VAR_unknown_82b2 = 0x82b2,
            VAR_unknown_82b5 = 0x82b5,
            VAR_unknown_82b8 = 0x82b8,
            VAR_unknown_82bc = 0x82bc,
            VAR_unknown_82d1 = 0x82d1,
            VAR_unknown_8298 = 0x8298,
            VAR_unknown_82a8 = 0x82a8,
            VAR_unknown_82a9 = 0x82a9,
            VAR_unknown_82aa = 0x82aa,
            VAR_unknown_8243 = 0x8243,
            VAR_unknown_8233 = 0x8233,
            ENUM_unknown_808d = 0x808d,             // TODO: Identify
            ENUM_unknown_803f = 0x803f,             // TODO: Identify
            ENUM_unknown_80bf = 0x80bf,             // TODO: Identify
            LONGVAR_unknown_8404 = 0x8404,          // TODO: Identify
            // Additional undefined messages from new logs
            ENUM_unknown_8031 = 0x8031,             // TODO: Identify
            ENUM_unknown_8000 = 0x8000,             // TODO: Identify
            ENUM_unknown_8005 = 0x8005,             // TODO: Identify
            ENUM_unknown_8017 = 0x8017,             // TODO: Identify
            ENUM_unknown_8019 = 0x8019,             // TODO: Identify
            ENUM_unknown_8021 = 0x8021,             // TODO: Identify
            ENUM_unknown_8032 = 0x8032,             // TODO: Identify
            ENUM_unknown_8033 = 0x8033,             // TODO: Identify
            ENUM_unknown_803c = 0x803c,             // TODO: Identify
            ENUM_unknown_8045 = 0x8045,             // TODO: Identify
            ENUM_unknown_8063 = 0x8063,             // TODO: Identify
            ENUM_unknown_8066 = 0x8066,             // TODO: Identify
            ENUM_unknown_8077 = 0x8077,             // TODO: Identify
            ENUM_unknown_8078 = 0x8078,             // TODO: Identify
            ENUM_unknown_8079 = 0x8079,             // TODO: Identify
            ENUM_unknown_807a = 0x807a,             // TODO: Identify
            ENUM_unknown_807b = 0x807b,             // TODO: Identify
            ENUM_unknown_807c = 0x807c,             // TODO: Identify
            ENUM_unknown_807d = 0x807d,             // TODO: Identify
            ENUM_unknown_807e = 0x807e,             // TODO: Identify
            ENUM_unknown_807f = 0x807f,             // TODO: Identify
            ENUM_unknown_8083 = 0x8083,             // TODO: Identify
            ENUM_unknown_808e = 0x808e,             // TODO: Identify
            ENUM_unknown_80a9 = 0x80a9,             // TODO: Identify
            ENUM_unknown_80af = 0x80af,             // TODO: Identify
            ENUM_unknown_80b1 = 0x80b1,             // TODO: Identify
            ENUM_unknown_80ce = 0x80ce,             // TODO: Identify
            ENUM_unknown_80d7 = 0x80d7,             // TODO: Identify
            VAR_unknown_202 = 0x202,                // TODO: Identify
            LONGVAR_unknown_410 = 0x410,            // TODO: Identify
            LONGVAR_unknown_41b = 0x41b,            // TODO: Identify
            // New messages from latest logs
            STRUCT_unknown_860d = 0x860d,           // TODO: Identify
            LONGVAR_unknown_8409 = 0x8409,          // TODO: Identify
            ENUM_in_backup_heater_status = 0x406C, // Backup Heater Status
            ENUM_in_booster_heater_status = 0x4087, // Booster Heater Status
            ENUM_in_3way_valve_position = 0x4067, // 3-Way Diverter Valve Position
            VAR_in_circulation_pump_speed = 0x40C4, // Circulation Pump Speed (DHW Pump)
            // --- Outdoor Unit Power and Energy Messages (corrected mappings) ---
            Compressor_Power = 0x8237,                      // VAR, W, Outdoor unit (was incorrectly mapped as target frequency)
            Fan_Power = 0x8239,                            // VAR, W, Outdoor unit
            Total_Power = 0x823d,                          // VAR, W, Outdoor unit
            Compressor_Current = 0x8236,                   // VAR, A, Outdoor unit (was incorrectly mapped as commanded frequency)
            Fan_Compressor_Frequency = 0x8238,             // VAR, Hz or step, Outdoor unit (was incorrectly mapped as current frequency)
            Phase_Current = 0x82db,              // VAR, A, Outdoor unit
            Phase_Voltage = 0x82de,              // VAR, V, Outdoor unit
            Phase_Power = 0x82df,                // VAR, W, Outdoor unit
            Power_Factor = 0x82e8,               // VAR, x0.01, e.g., 100 = PF 1.0
            System_Efficiency = 0x82ed,          // VAR, x0.1, May represent COP
            System_Capacity = 0x82f6,            // VAR, kW or BTU/h, Possibly x0.1
            Total_Energy_Consumption = 0x8408,   // LVAR, kWh, Outdoor cumulative
            Operation_Hours = 0x840b,            // LVAR, hours, Total run time
            Compressor_Hours = 0x840c,           // LVAR, hours, Comp. runtime
            Fan_Hours = 0x841a,                  // LVAR, hours, Outdoor fan runtime
            Control_Wattmeter_Unit = 0x8411,     // LVAR, kWh, Outdoor unit (main)
            Wattmeter_Total_Sum = 0x8415,        // LVAR, kWh, Cumulative from unit start
            Wattmeter_Accumulated = 0x8416,      // LVAR, kWh, Accumulative sum over session
            Produced_Energy_Actual = 0x8426,     // LVAR, kWh, e.g., heating output
            Produced_Energy_Total = 0x8404,      // LVAR, kWh, Output since install
            
            // --- Missing Outdoor Unit Messages from Reference Tables ---
            ENUM_out_compressor_running_status = 0x8010,         // Compressor Running Status (bool)
            ENUM_out_hot_gas_valve_status = 0x8017,              // Hot Gas Valve Status (bool)
            ENUM_out_liquid_line_valve_status = 0x8019,          // Liquid Line Valve Status (bool)
            ENUM_out_4way_reversing_valve_status = 0x801A,       // 4-Way Reversing Valve Status (bool)
            ENUM_out_evi_bypass_valve_status = 0x8021,           // EVI Bypass Valve Status (bool)
            ENUM_out_base_pan_heater = 0x80AF,                   // Base Pan Heater (bool)
            ENUM_out_phe_heater = 0x80D7,                        // PHE (plate HEX) Heater (bool)
            VAR_out_ambient_temperature = 0x8204,                // Outdoor (ambient) Temperature (°C)
            VAR_out_condenser_outlet_temp = 0x8218,              // Condenser Outlet (HEX out) Temp (°C)
            VAR_out_suction_temperature = 0x821A,                // Suction Temperature (°C)
            VAR_out_evi_inlet_temp = 0x821E,                     // EVI Inlet Temp (°C)
            VAR_out_evi_outlet_temp = 0x8220,                    // EVI Outlet Temp (°C)
            VAR_out_fan_step = 0x8226,                           // Outdoor Fan Step (step)
            VAR_out_eev_steps = 0x8229,                          // Main EEV Steps (steps)
            VAR_out_dc_link_voltage = 0x823B,                    // DC Link Voltage (V)
            VAR_out_high_pressure_sat_temp = 0x829F,             // High-Pressure Sat. Temp (°C)
            VAR_out_low_pressure_sat_temp = 0x82A0,              // Low-Pressure Sat. Temp (°C)
            
            // --- Missing Indoor Unit Messages from Reference Tables ---
            VAR_in_zone2_room_setpoint = 0x42D6,                 // Zone 2 Room Setpoint (°C)
            VAR_in_zone2_water_setpoint = 0x42D7,                // Zone 2 Water Setpoint (°C)
            VAR_in_water_outlet_zone1 = 0x42D8,                  // Water Outlet (Zone 1) (°C)
            VAR_in_water_outlet_zone2 = 0x42D9,                  // Water Outlet (Zone 2) (°C)
            VAR_in_water_flow_rate = 0x42E9,                     // Water Flow Rate (L/min x0.1)
            VAR_in_flow_rate_control = 0x42F1,                   // Flow Rate Control % (%)
            
            // --- Missing Energy Messages from Reference Tables ---
            LVAR_out_wattmeter_total_sum = 0x8415,               // Wattmeter Total Sum (kWh)
            LVAR_out_produced_energy_total = 0x8427,             // Produced Energy (Total) (kWh)
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

        DecodeResult try_decode_nasa_packet(std::vector<uint8_t> data);
        void process_nasa_packet(MessageTarget *target);

        class NasaProtocol : public Protocol
        {
        public:
            NasaProtocol() = default;

            void publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request) override;
            void protocol_update(MessageTarget *target) override;
        };

    } // namespace samsung_ac
} // namespace esphome
