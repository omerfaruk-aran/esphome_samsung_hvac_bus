#pragma once

#include <cstdint>

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
            ENUM_in_operation_power = 0x4000,  // [RW] Normal Operation / Power / Zone1 Normal Power | 0..2 | 0=Off; 1=On; 2=On
            ENUM_in_operation_mode = 0x4001,  // [W] 0..25 | 0=Auto; 1=Cool; 2=Dry; 3=Fan; 4=Heat; 21=Cool Storage; 24=Hot Water
            ENUM_in_fan_mode = 0x4006,  // Did not exists in xml...only in Remocon.dll code | [W] fan speed (set) | 0=Auto; 1=Low; 2=Mid; 3=High; 4=Turbo; 254=Off | read back on 0x4007
            ENUM_in_fan_mode_real = 0x4007,  // [R] Fan Speed | 0..254 | 1=Low; 2=Mid; 3=High; 4=Turbo; 10=AutoLow; 11=AutoMid; 12=AutoHigh; 13=UL; 14=LL; 15=HH; 16=Spee...

            ENUM_in_louver_hl_swing = 0x4011,  // [RW] Wind direction(Vertical) | 0=Off; 1=On
            ENUM_in_louver_lr_swing = 0x407e,  // [RW] Wind direction(Horizontal) | 0=Off; 1=On

            ENUM_in_state_humidity_percent = 0x4038,  // [R] Humidity | 0..100

            ENUM_in_alt_mode = 0x4060,  // [R] Wind Free | 0=OFF; 9=ON | also known as ENUM_IN_ALTERNATIVE_MODE
            ENUM_in_water_heater_power = 0x4065,  // [RW] DHW Power / WaterHeaterPower | 0=Off; 1=On
            ENUM_in_water_heater_mode = 0x4066,  // [RW] DHW Mode / WaterHeaterMode | 0..3 | 0=Eco; 1=Standard; 2=Power; 3=Force

            ENUM_in_operation_automatic_cleaning = 0x4111,  // no vendor definition (community-sourced)

            VAR_in_temp_target_f = 0x4201,  // [RW] Zone1 Room Set Temp. / Set Temperature | Celsius | /10 | signed | -41..150
            // Room temperature arrives as two separate messages - keep the pair together.
            //   0x4203 NASA_CURRENT_TEMP          - raw sensor reading. Drives the climate
            //          entity's current temperature and the `room_temperature` sensor.
            //   0x4204 NASA_MODIFIED_CURRENT_TEMP - compensated value. Exposed as the
            //          optional `room_temperature_modified` sensor, wired by message number
            //          in __init__.py rather than by a case in process_messageset().
            // Which one matches the wall remote is not confirmed on real hardware;
            // enable both sensors and compare.
            VAR_in_temp_room_f = 0x4203,  // [R] Room Temp. / Temp. Sensor | Celsius | /10 | signed | -41..150
            VAR_in_temp_room_modified_f = 0x4204,  // [R] Temperature (NASA_MODIFIED_CURRENT_TEMP) | Celsius | /10 | signed
            VAR_in_temp_eva_in_f = 0x4205,  // [R] Eva In1 / Eva In / T_PI / Water In | Celsius | /10 | signed | -41..150
            VAR_in_temp_eva_out_f = 0x4206,  // [R] Eva Out1 / Eva Out / Water Out | Celsius | /10 | signed | -41..150
            VAR_in_model_information = 0x4229,  // [R] Model | 12=Master-N; 31=Slim 1Way; 32=Big Slim 1Way; 51=Global 4Way; 52=Global Mini4Way; 53=Mini 4Way; 6...

            VAR_in_temp_water_heater_target_f = 0x4235,  // [RW] DHW Set Temp. / WaterHeater Set Temp. | Celsius | /10 | signed | -41..150
            VAR_in_temp_water_tank_f = 0x4237,  // [R] DHW Tank / Water Tank | Celsius | /10 | signed | -41..150
            VAR_in_temp_water_outlet_target_f = 0x4247,  // [RW] Water Outlet Set Temp. | Celsius | /10 | signed | -41..150

            VAR_out_sensor_airout = 0x8204,  // [R] Outdoor Temp. / T_A | Celsius | /10 | signed | -41..150
            VAR_OUT_SENSOR_CT1 = 0x8217,  // [R] Compressor Current1 | /10 | 0..10000

            VAR_out_error_code = 0x8235,  // [R] Error Code

            // Power/Energy (existing)
            LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM = 0x8413,  // [unused] kW | /1000.0
            LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM = 0x8414,  // no vendor definition (community-sourced)

            // Voltage (existing)
            LVAR_NM_OUT_SENSOR_VOLTAGE = 0x24fc,  // no vendor definition (community-sourced)

            // --- Known addresses with no handler of their own. Reachable as custom_sensor. ---
            ENUM_IN_OPERATION_VENT_POWER = 0x4003,  // [R] Power / Ventilator Power | 0..1 | 0=Off; 1=On
            ENUM_IN_OPERATION_VENT_MODE = 0x4004,  // [R] Mode / Ventilator Mode | 0..25 | 0=Normal; 1=HeatEx; 2=Bypass; 3=Normal+Purify; 4=HeatEx+Purify; 5=Purify; 6=Sleep; 7=Bypass+Puri...

            ENUM_in_louver_hl_part_swing = 0x4012,  // [unused] 0=Sing Off; 1=LOUVER 1; 2=LOUVER 2; 3=LOUVER 1,2; 4=LOUVER 3; 5=LOUVER 1,3; 6=LOUVER 2,3; 7=LOUV...

            ENUM_IN_QUIET_MODE = 0x406e,  // [unused] no vendor label

            ENUM_IN_OPERATION_POWER_ZONE1 = 0x4119,  // [unused] 0..1
            ENUM_IN_OPERATION_POWER_ZONE2 = 0x411e,  // [RW] Zone2 Normal Power | 0..1

            ENUM_in_operation_mode_real = 0x4002,  // [R] Mode / Operation Mode | 0..24 | 0=Unknown; 1=Cool; 2=Dry; 3=Fan; 4=Heat; 11=Auto Cool; 12=Auto Dry; 13=Auto Fan; 14=Auto Heat; 2...
            ENUM_in_fan_vent_mode = 0x4008,  // [R] Fan Speed | 0..254 | 0=Auto; 1=Low; 2=Mid; 3=High; 4=Turbo

            VAR_in_capacity_request = 0x4211,  // [R] Capacity | kW | /8.6 | 0..7460

            // Outdoor enums used in debug
            ENUM_out_operation_odu_mode = 0x8001,  // [R] Operation Mode | 0..254 | 0=OP_STOP; 1=OP_SAFETY; 2=OP_NORMAL; 3=OP_BALANCE; 4=OP_RECOVERY; 5=OP_DEICE; 6=OP_COMPDOWN; 7=O...
            ENUM_out_operation_heatcool = 0x8003,  // [R] Operation Status | 0..4 | 0=Undefined; 1=Cool; 2=Heat; 3=CoolMain; 4=HeatMain
            ENUM_out_load_4way = 0x801a,  // [R] 4Way Valve / V_4W / 4Way

            // --- Pipe sensors & outdoor misc used in debug ---
            VAR_OUT_SENSOR_PIPEIN3 = 0x8261,  // [R] Outdoor Pipe IN 3 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEIN4 = 0x8262,  // [R] Outdoor Pipe IN 4 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEIN5 = 0x8263,  // [R] Outdoor Pipe IN 5 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEOUT1 = 0x8264,  // [R] Outdoor Pipe OUT 1 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEOUT2 = 0x8265,  // [R] Outdoor Pipe OUT 2 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEOUT3 = 0x8266,  // [R] Outdoor Pipe OUT 3 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEOUT4 = 0x8267,  // [R] Outdoor Pipe OUT 4 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEOUT5 = 0x8268,  // [R] Outdoor Pipe OUT 5 | Celsius | /10 | signed | -50..94

            VAR_out_control_order_cfreq_comp2 = 0x8274,  // [R] Order Frequency2 | 0..1000
            VAR_out_control_target_cfreq_comp2 = 0x8275,  // [R] Target Frequency2 | 0..1000

            VAR_out_sensor_top1 = 0x8280,  // [R] Comp Top1 / T_CT / T_CT1 | Celsius | /10 | signed | -41..150
            VAR_OUT_PHASE_CURRENT = 0x82db,  // [R] Phase Current

            VAR_OUT_PROJECT_CODE = 0x82bc,  // [W] signed
            VAR_OUT_PRODUCT_OPTION_CAPA = 0x82e3,  // [R] Capacity | kW | /10.0

            // --- Your “FSV” variables (used in process_messageset default switch) ---
            VAR_IN_FSV_3021 = 0x4260,  // [RW] 3021:Max.- Heat pump - DHW | Celsius | /10 | signed
            VAR_IN_FSV_3022 = 0x4261,  // [RW] 3022:Stop- Heat pump - DHW | Celsius | /10 | signed
            VAR_IN_FSV_3023 = 0x4262,  // [RW] 3023:Start- Heat pump - DHW | Celsius | /10 | signed

            // --- Additional wattmeter/energy ids (used in process_messageset default switch) ---
            NASA_OUTDOOR_CONTROL_WATTMETER_1UNIT = 0x8411,  // no vendor definition (community-sourced)
            NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM = 0x8415,  // no vendor definition (community-sourced)
            NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM_ACCUM = 0x8416,  // no vendor definition (community-sourced)

            actual_produced_energy = 0x8426,  // no vendor definition (community-sourced)
            total_produced_energy = 0x8427,  // no vendor definition (community-sourced)

            VAR_IN_DUST_SENSOR_PM10_0_VALUE = 0x42d1,  // [R] PM 10.0 | -1..999
            VAR_IN_DUST_SENSOR_PM2_5_VALUE = 0x42d2,  // [R] PM 2.5 | -1..999
            VAR_IN_DUST_SENSOR_PM1_0_VALUE = 0x42d3,  // [R] PM 1.0 | -1..999

            // =============================================================
            // Message numbers, labels, units, scaling and enum values below were
            // compiled from vendor protocol definitions and from the observed
            // behaviour of vendor commissioning tooling.
            //
            // Scope: air-to-air air conditioners only (CAC, CAC Inverter, DVM S,
            // DVM S Eco 3HP/789HP/BIG/HPHR, DVM S Water(+mini), DVM Slim, FJM,
            // Home DVM). EHS / DVM Chiller / GHP / ERV / FCU were excluded.
            //
            // Access tag in each comment:
            //   [RW]     both read back and written by vendor tooling
            //   [W]      only ever written, never read back
            //   [R]      read back only - no writer observed
            //   [unused] defined upstream but neither read nor written by any
            //            tooling seen, so its values are unverified
            // [R] is a lower bound: it means no writer was observed, not that
            // the bus refuses a write.
            //
            // ENUM_IN_FSV_* / VAR_IN_FSV_* are installer Field Setting Values
            // (commissioning-time settings), not runtime controls.
            // =============================================================

            // ---------- READ / WRITE  (115) ----------

            // Enum
            ENUM_IN_REFERENCE_EHS_TEMP = 0x406f,  // [RW] Ref. Temp. | 0=Room; 1=Water Out
            ENUM_IN_FSV_2041 = 0x4093,  // [RW] 2041:WL type- Auto heating of wired remote controller - Water Law | 1=1:Floor; 2=2:FCU
            ENUM_IN_FSV_2081 = 0x4094,  // [RW] 2081:WL type- Auto cooling of wired remote controller - Water Law | 1=1:Floor; 2=2:FCU
            ENUM_IN_FSV_2091 = 0x4095,  // [RW] 2091:#1(Floor)- Use of thermostat - Water Law | 0=No; 1=1; 2=2; 3=3; 4=4
            ENUM_IN_FSV_2092 = 0x4096,  // [RW] 2092:#2(FCU)- Use of thermostat - Water Law | 0=No; 1=1; 2=2; 3=3; 4=4
            ENUM_IN_FSV_3011 = 0x4097,  // [RW] 3011:DHW application- Activating hot water function - DHW | 0=No; 1=1; 2=2
            ENUM_IN_FSV_3031 = 0x4098,  // [RW] 3031:Operation- Booster heater - DHW | 0=No; 1=Yes
            ENUM_IN_FSV_3041 = 0x4099,  // [RW] 3041:Operation- Disinfection - DHW | 0=No; 1=Yes
            ENUM_IN_FSV_3042 = 0x409a,  // [RW] 3042:Operation interval- Disinfection - DHW | 0=Sunday; 1=Monday; 2=Tuesday; 3=Wednesday; 4=Thursday; 5=Friday; 6=Saturday; 7=Everyday
            ENUM_IN_FSV_3051 = 0x409b,  // [RW] 3051:Timer OFF Function- Forced DHW Opreration - DHW | 0=No; 1=Yes
            ENUM_IN_FSV_3061 = 0x409c,  // [RW] 3061:H/P interlocking- Solar heat panel/DHW thermostat - DHW | 0=No; 1=1; 2=2; 3=3
            ENUM_IN_FSV_3071 = 0x409d,  // [RW] 3071:Basic valve direction - 3-way/DHW Valve - DHW | 0=Room; 1=Tank
            ENUM_IN_FSV_4011 = 0x409e,  // [RW] 4011:Heating/hot water priority- Heat pump - Heating | 0=DHW; 1=Heating
            ENUM_IN_FSV_4021 = 0x409f,  // [RW] 4021:Application- Backup Heater - Heating | 0=0; 1=1; 2=2
            ENUM_IN_FSV_4022 = 0x40a0,  // [RW] 4022:BUH/BSH priority- Backup Heater - Heating | 0=BUH/BSH Both; 1=BUH; 2=BSH
            ENUM_IN_FSV_4023 = 0x40a1,  // [RW] 4023:Cold Weather compensation- Backup Heater - Heating | 0=No; 1=Yes
            ENUM_IN_FSV_4031 = 0x40a2,  // [RW] 4031:Application- Backup Boiler - Heating | 0=No; 1=Yes
            ENUM_IN_FSV_4032 = 0x40a3,  // [RW] 4032:Boiler priority- Backup Boiler - Heating | 0=No; 1=Yes
            ENUM_IN_FSV_5041 = 0x40a4,  // [RW] 5041:Operation- Benefit kWh (Power Peak control) - Others | 0=No; 1=Yes
            ENUM_IN_FSV_5042 = 0x40a5,  // [RW] 5042:Heat source for operation limit- Benefit kWh (Power Peak control) - Others | 0=All; 1=1; 2=2; 3=3
            ENUM_IN_FSV_5043 = 0x40a6,  // [RW] 5043:Contact Logic- Benefit kWh (Power Peak control) - Others | 0=Low; 1=High
            ENUM_IN_FSV_5051 = 0x40a7,  // [RW] 5051:Operation - Frequency Ratio Control - Others | 0=No; 1=Yes
            ENUM_IN_FSV_5061 = 0x40b4,  // [RW] 5061:Ratio of hot water supply compare to heating  - Others
            ENUM_IN_EMPTY_ROOM_CONTROL_USED = 0x40bd,  // [RW] Unoccupied room setting | 0..1 | 0=Disable; 1=Enable
            ENUM_IN_FSV_4041 = 0x40c0,  // [RW] 4041:Application- Mixing valve - Heating | 0..2 | 0=No; 1=1; 2=2
            ENUM_IN_FSV_4044 = 0x40c1,  // [RW] 4044:Control Factor- Mixing valve - Heating | 0..5
            ENUM_IN_FSV_4051 = 0x40c2,  // [RW] 4051:Application- Inverter Pump - Heating | 0..2
            ENUM_IN_FSV_4053 = 0x40c3,  // [RW] 4053:Control Factor- Inverter Pump - Heating | 0..3
            ENUM_IN_ENTER_ROOM_CONTROL_USED = 0x40d5,  // [RW] Entering room setting | 0..1 | 0=Disable; 1=Enable
            ENUM_IN_FSV_5033 = 0x4107,  // [RW] 5033:A2A/DHW Priority- TDM Operation Time - Others | 0=A2A; 1=DHW
            ENUM_IN_FSV_4061 = 0x411a,  // [RW] 4061:Zone Control-Application | 0=0; 1=1
            ENUM_IN_FSV_5081 = 0x411b,  // [RW] 5081:PV Control-Application | 0=0; 1=1
            ENUM_IN_FSV_5091 = 0x411c,  // [RW] 5091:Smart Grid Control-Application | 0=0; 1=1
            ENUM_IN_FSV_5094 = 0x411d,  // [RW] 5094:Smart Grid Control-DHW Mode | 0=0; 1=1
            ENUM_IN_FSV_2093 = 0x4127,  // [RW] 2093:Remote Controller Auto mode | 1=1; 2=2; 3=3; 4=4
            ENUM_IN_FSV_5022 = 0x4128,  // [RW] 5022:Saving hot water-DHW Saving Mode | 0=0; 1=1
            ENUM_IN_FSV_2094 = 0x412a,  // [RW] 0=No; 1=1; 2=2; 3=3; 4=4
            ENUM_OUT_CHECK_REF_RESULT = 0x809c,  // [RW] Refrigerant Check Result | 0..7 | 0=RefResult_NotInspect; 1=completed normally; 2=RefResult_NotJudgment; 3=subcooling not achievable; 4=RefResult_Normal; 5=RefRe...

            // Variable
            VAR_in_eev_value_real_1 = 0x4217,  // [RW] EEV1 | 0..3000
            VAR_IN_FSV_1011 = 0x424a,  // [RW] 1011:Max.- Temperature of general cooling discharge water - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1012 = 0x424b,  // [RW] 1012:Min.- Temperature of general cooling discharge water - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1021 = 0x424c,  // [RW] 1021:Max.- General Indoor cooling temperature - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1022 = 0x424d,  // [RW] 1022:Min.- General Indoor cooling temperature - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1031 = 0x424e,  // [RW] 1031:Max.- Temperature of general heating discharge water - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1032 = 0x424f,  // [RW] 1032:Min.- Temperature of general heating discharge water - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1041 = 0x4250,  // [RW] 1041:Max.- General indoor heating temperature - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1042 = 0x4251,  // [RW] 1042:Min.- General indoor heating temperature - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1051 = 0x4252,  // [RW] 1051:Max.- Temperature of hot water tank - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_1052 = 0x4253,  // [RW] 1052:Min.- Temperature of hot water tank - Remote Controller | Celsius | /10 | signed
            VAR_IN_FSV_2011 = 0x4254,  // [RW] 2011:Max.- Auto heating ambient temperature - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2012 = 0x4255,  // [RW] 2012:Min.- Auto heating ambient temperature - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2021 = 0x4256,  // [RW] 2021:Max.- Temperature of auto heating discharge water (WL1-Floor) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2022 = 0x4257,  // [RW] 2022:Min.- Temperature of auto heating discharge water ((WL1-Floor) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2031 = 0x4258,  // [RW] 2031:Max.- Temperature of auto heating discharge water (WL2-FCU) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2032 = 0x4259,  // [RW] 2032:Min.- Temperature of auto heating discharge water (WL2-FCU) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2051 = 0x425a,  // [RW] 2051:Max.- Auto cooling ambient temperature - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2052 = 0x425b,  // [RW] 2052:Min.- Auto cooling ambient temperature - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2061 = 0x425c,  // [RW] 2061:Max.- Temperature of auto cooling discharge water (WL1-Floor) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2062 = 0x425d,  // [RW] 2062:Min.- Temperature of auto cooling discharge water ((WL1-Floor) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2071 = 0x425e,  // [RW] 2071:Max.- Temperature of auto cooling discharge water (WL2-FCU) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_2072 = 0x425f,  // [RW] 2072:Min.- Temperature of auto cooling discharge water (WL2-FCU) - Water Law | Celsius | /10 | signed
            VAR_IN_FSV_3024 = 0x4263,  // [RW] 3024:Min. hour- Heat pump - DHW
            VAR_IN_FSV_3025 = 0x4264,  // [RW] 3025:Max. hour- Heat pump - DHW
            VAR_IN_FSV_3026 = 0x4265,  // [RW] 3026:Operation interval- Heat pump - DHW
            VAR_IN_FSV_3032 = 0x4266,  // [RW] 3032:Delayed time- Booster heater - DHW
            VAR_IN_FSV_3033 = 0x4267,  // [RW] 3033:Overshoot- Booster heater - DHW | Celsius | /10 | signed
            VAR_IN_FSV_3043 = 0x4269,  // [RW] 3043:Start time- Disinfection - DHW
            VAR_IN_FSV_3044 = 0x426a,  // [RW] 3044:Target temp.- Disinfection - DHW | Celsius | /10 | signed
            VAR_IN_FSV_3045 = 0x426b,  // [RW] 3045:Holding time- Disinfection - DHW | signed
            VAR_IN_FSV_3052 = 0x426c,  // [RW] 3052:Timer Duration(per 10min)- Forced DHW Opreration - DHW | /0.1 | signed
            VAR_IN_FSV_4012 = 0x426d,  // [RW] 4012:Heating priority- Heat pump - Heating | Celsius | /10 | signed
            VAR_IN_FSV_4013 = 0x426e,  // [RW] 4013:Heating Off- Heat pump - Heating | Celsius | /10 | signed
            VAR_IN_FSV_4024 = 0x4270,  // [RW] 4024:Threshold Temp.- Backup Heater - Heating | Celsius | /10 | signed
            VAR_IN_FSV_4025 = 0x4271,  // [RW] 4025:Defrost Backup Temp.- Backup Heater - Heating | Celsius | /10 | signed
            VAR_IN_FSV_4033 = 0x4272,  // [RW] 4033:Threshold Temp.- Backup Boiler - Heating | Celsius | /10 | signed
            VAR_IN_FSV_5011 = 0x4273,  // [RW] 5011:Temperature of cooling water oulet- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5012 = 0x4274,  // [RW] 5012:Room Temperature of cooling Mode- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5013 = 0x4275,  // [RW] 5013:Temperature of heating discharge water- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5014 = 0x4276,  // [RW] 5014:Indoor heating temperature- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5015 = 0x4277,  // [RW] 5015:Temperature of auto cooling WL1 water- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5016 = 0x4278,  // [RW] 5016:Temperature of auto cooling WL2 water- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5017 = 0x4279,  // [RW] 5017:Temperature of auto heating WL1 water- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5018 = 0x427a,  // [RW] 5018:Temperature of auto heating WL2 water- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5019 = 0x427b,  // [RW] 5019:Temperature of hot water Tank- Outing mode - Others | Celsius | /10 | signed
            VAR_IN_FSV_5021 = 0x427c,  // [RW] 5021:Temperature of hot water Tank- Saving hot water - Others | Celsius | /10 | signed
            VAR_IN_FSV_5031 = 0x427d,  // [RW] 5031:Priority Max. Operation Time- TDM Operation Time - Others
            VAR_IN_FSV_5032 = 0x427e,  // [RW] 5032:Non Priority Min. Operation Time- TDM Operation Time - Others
            VAR_IN_FSV_4042 = 0x4286,  // [RW] 4042:Target ΔT(Heating)- Mixing valve - Heating | Celsius | /10
            VAR_IN_FSV_4043 = 0x4287,  // [RW] 4043:Target ΔT(Cooling)- Mixing valve - Heating | Celsius | /10
            VAR_IN_FSV_4045 = 0x4288,  // [RW] 4045:Control Interval- Mixing valve - Heating
            VAR_IN_FSV_4046 = 0x4289,  // [RW] 4046:Running Time(per 10 sec.)- Mixing valve - Heating | /0.1
            VAR_IN_FSV_4052 = 0x428a,  // [RW] 4052:Target ΔT- Inverter Pump - Heating | Celsius | /10
            VAR_IN_FSV_3046 = 0x42ce,  // [RW] 3046:Max. Operation Time-Disinfection-DHW
            VAR_IN_TEMP_TARGET_ZONE2_F = 0x42d6,  // [RW] Zone2 Room Set Temp. | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_OUTLET_TARGET_ZONE2_F = 0x42d7,  // [RW] Water Outlet2 Set Temp. | Celsius | /10 | signed | -41..150
            VAR_IN_FSV_5082 = 0x42db,  // [RW] 5082:PV Control-Setting Temp Shift Value(Cooling) | Celsius | /10
            VAR_IN_FSV_5083 = 0x42dc,  // [RW] 5083:PV Control-Setting Temp Shift Value(Heating) | Celsius | /10
            VAR_IN_FSV_5092 = 0x42dd,  // [RW] 5092:Smart Grid Control-Setting Temp Shift Value(Heating) | Celsius | /10
            VAR_IN_FSV_5093 = 0x42de,  // [RW] 5093:Smart Grid Control-Setting Temp Shift Value(DHW) | Celsius | /10
            VAR_IN_FSV_3081 = 0x42ed,  // [RW] 3081:Energy metering-BUH 1step capacity | signed
            VAR_IN_FSV_3082 = 0x42ee,  // [RW] 3082:Energy metering-BUH 2step capacity | signed
            VAR_IN_FSV_3083 = 0x42ef,  // [RW] 3083:Energy metering-BSH capacity | signed
            VAR_IN_FSV_5023 = 0x42f0,  // [RW] 5023:Saving hot water-Thermo On | Celsius | /10 | signed
            VAR_out_install_Comp_Num = 0x8202,  // [RW] Total Comp | 0..100
            VAR_out_load_outeev1 = 0x8229,  // [RW] Main EEV1 / E_M / E_M1 / EEV 1 | 0..10000
            VAR_out_load_outeev2 = 0x822a,  // [RW] Main EEV2 / E_M2 / EEV 2 | 0..10000

            // LongVariable
            LVAR_ad_address_rmc = 0x0402,  // [RW] RMC | & 0xFF

            // Structure
            STR_ad_option_basic = 0x0600,  // [RW] Product Option
            STR_ad_option_install = 0x0601,  // [RW] Installation Option
            STR_ad_option_install_2 = 0x0602,  // [RW] Installation Option2
            STR_ad_info_equip_position = 0x0605,  // [RW] Location
            STR_ad_id_serial_number = 0x0607,  // [RW] Serial Number
            STR_ad_dbcode_micom_main = 0x0608,  // [RW] Main Micom
            STR_ad_dbcode_eeprom = 0x060c,  // [RW] EEPROM Version
            STR_AD_PRODUCT_MODEL_NAME = 0x061a,  // [RW] Model

            // ---------- WRITE ONLY  (2) ----------

            // Variable
            VAR_OUT_HIGH_OVERLOAD_DETECT = 0x82f5,  // [W]

            // LongVariable
            LVAR_IN_AUTO_STATIC_PRESSURE = 0x4415,  // [W]

            // ---------- READ ONLY (sensors / status)  (131) ----------

            // Enum
            ENUM_IN_STATE_THERMO = 0x4028,  // [R] Thermo / Themo on/off | 0=Off; 1=On
            ENUM_in_state_defrost_mode = 0x402e,  // [R] Defrost Valve
            ENUM_IN_MTFC = 0x402f,  // [R] MTFC Status
            ENUM_IN_SILENCE = 0x4046,  // [R] Silence Mode
            ENUM_IN_3WAY_VALVE = 0x4067,  // [R] 3Way Valve | 0..1 | 0=Room; 1=Tank
            ENUM_IN_SOLAR_PUMP = 0x4068,  // [R] Solar Pump
            ENUM_IN_THERMOSTAT1 = 0x4069,  // [R] Thermostat1 / Cooling Thermostat | 0..2 | 0=Off; 1=Cool; 2=Heat
            ENUM_IN_THERMOSTAT2 = 0x406a,  // [R] Thermostat2 / Heating Thermostat | 0..2 | 0=Off; 1=Cool; 2=Heat
            ENUM_IN_BACKUP_HEATER = 0x406c,  // [R] Backup Heater | 0..2 | 0=OFF; 1=STEP 1; 2=STEP 2
            ENUM_IN_OUTING_MODE = 0x406d,  // [R] Outing Operation
            ENUM_IN_DISCHAGE_TEMP_CONTROL = 0x4070,  // [R] Discharge control | 0=Off; 1=On
            ENUM_IN_BOOSTER_HEATER = 0x4087,  // [R] Boost Heater | 0..1
            ENUM_IN_STATE_WATER_PUMP = 0x4089,  // [R] Water Pump | 0=Off; 1=On
            ENUM_IN_2WAY_VALVE = 0x408a,  // [R] 2Way Valve
            ENUM_IN_STATE_AUTO_STATIC_PRESSURE_RUNNING = 0x40bb,  // [R] Auto ESP | 0=OFF; 1=Complete; 2=Running
            ENUM_IN_STATE_KEY_TAG = 0x40bc,  // [R] Key Tag Status | 0..1 | 0=UNOCCUPIED; 1=OCCUPIED
            ENUM_IN_WATERPUMP_PWM_VALUE = 0x40c4,  // [R] Inverter Pump PWM | 0..100
            ENUM_IN_THERMOSTAT_WATER_HEATER = 0x40c5,  // [R] DHW Thermostat
            ENUM_IN_ENTHALPY_CONTROL_STATE = 0x4105,  // [R] Enthalpy contorl entry
            ENUM_IN_FREE_COOLING_STATE = 0x410d,  // [R] Free cooling operation
            ENUM_IN_PV_CONTACT_STATE = 0x4123,  // [R] PV Control | 0=Disable; 1=Enable
            ENUM_IN_SG_READY_MODE_STATE = 0x4124,  // [R] Smart Grid
            ENUM_out_load_comp1 = 0x8010,  // [R] Comp1 / Compressor
            ENUM_out_load_comp2 = 0x8011,  // [R] Comp2
            ENUM_out_load_cch1 = 0x8013,  // [R] CCH1
            ENUM_out_load_cch2 = 0x8014,  // [R] CCH2
            ENUM_out_load_hotgas = 0x8017,  // [R] Hot Gas Valve / V_HG / Hot Gas1
            ENUM_out_load_hotgas2 = 0x8018,  // [R] Hot Gas2 / V_HG2
            ENUM_out_load_maincool = 0x801f,  // [R] Main Cooling / V_MC
            ENUM_out_load_outeev = 0x8020,  // [R] EEV Valve
            ENUM_out_load_evi_bypass = 0x8021,  // [R] EVI Bypass / V_EB
            ENUM_out_load_evi_sol1 = 0x8022,  // [R] EVI Sol1 / V_EV / V_EV1
            ENUM_out_load_evi_sol2 = 0x8023,  // [R] EVI Sol2 / V_EV2
            ENUM_out_load_gascharge = 0x8025,  // [R] Hot Gas Charging / V_HC
            ENUM_out_load_water = 0x8026,  // [R] 2Way
            ENUM_out_load_pumpout = 0x8027,  // [R] Pump Out
            ENUM_OUT_LOAD_4WAY2 = 0x802a,  // [R] Sum-4way
            ENUM_out_load_liquidtube = 0x8034,  // [R] Liquid tube / V_LT / V_L
            ENUM_out_load_flow_switch = 0x803b,  // [R] Flow Switch
            ENUM_out_op_test_op_complete = 0x8046,  // [R] TestOperation(UP) | 0=Not Completed; 1=Completed
            ENUM_out_mcu_load_cool_a = 0x8049,  // [R] A-Cool
            ENUM_out_mcu_load_heat_a = 0x804a,  // [R] A-Heat
            ENUM_out_mcu_load_cool_b = 0x804b,  // [R] B-Cool
            ENUM_out_mcu_load_heat_b = 0x804c,  // [R] B-Heat
            ENUM_out_mcu_load_cool_c = 0x804d,  // [R] C-Cool
            ENUM_out_mcu_load_heat_c = 0x804e,  // [R] C-Heat
            ENUM_out_mcu_load_cool_d = 0x804f,  // [R] D-Cool
            ENUM_out_mcu_load_heat_d = 0x8050,  // [R] D-Heat
            ENUM_out_mcu_load_cool_e = 0x8051,  // [R] E-Cool
            ENUM_out_mcu_load_heat_e = 0x8052,  // [R] E-Heat
            ENUM_out_mcu_load_cool_f = 0x8053,  // [R] F-Cool
            ENUM_out_mcu_load_heat_f = 0x8054,  // [R] F-Heat
            ENUM_out_mcu_load_liquid = 0x8055,  // [R] LBV
            ENUM_out_mcu_port0_indoor_addr = 0x8058,  // [R] A-Address | 0..254
            ENUM_out_mcu_port1_indoor_addr = 0x8059,  // [R] B-Address | 0..254
            ENUM_out_mcu_port2_indoor_addr = 0x805a,  // [R] C-Address | 0..254
            ENUM_out_mcu_port3_indoor_addr = 0x805b,  // [R] D-Address | 0..254
            ENUM_out_mcu_port4_indoor_addr = 0x805c,  // [R] E-Address | 0..254
            ENUM_out_mcu_port5_indoor_addr = 0x805d,  // [R] F-Address | 0..254
            ENUM_OUT_CONTROL_FAN_NUM = 0x8099,  // [R]
            ENUM_OUT_LOAD_CBOX_COOLING_FAN = 0x809e,  // [R] DC Fan
            ENUM_OUT_LOAD_BASEHEATER = 0x80af,  // [R] Base Heater

            // Variable
            VAR_ad_error_code1 = 0x0202,  // [R] Error Code

            VAR_in_temp_discharge = 0x420b,  // [R] Discharge(Duct) | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_DISCHARGE_COOL_TARGET_F = 0x422a,  // [R] Disc. Set temp.(Cool) | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_DISCHARGE_HEAT_TARGET_F = 0x422b,  // [R] Disc. Set temp.(Heat) | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_IN_F = 0x4236,  // [R] Water In / T_WI / Water In1 | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_OUT_F = 0x4238,  // [R] Water Out / T_WO | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_OUT2_F = 0x4239,  // [R] Heater Out(Tw3) / T_W3 | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_LAW_TARGET_F = 0x4248,  // [R] Water Law Set Temp. / Water Law Target Value | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_MIXING_VALVE_F = 0x428c,  // [R] Mixing Valve Temp(Tw4) | Celsius | /10 | signed | -41..150
            VAR_IN_FAN_CURRENT_RPM_SUCTION1 = 0x429f,  // [R] Booster Fan1
            VAR_IN_FAN_CURRENT_RPM_SUCTION2 = 0x42a1,  // [R] Booster Fan2
            VAR_IN_FAN_CURRENT_RPM_SUCTION3 = 0x42a3,  // [R] Booster Fan3
            VAR_IN_ENTHALPY_SENSOR_OUTPUT = 0x42cf,  // [R] Enthalpy value | Enthalpy | /10
            VAR_IN_EXT_VARIABLE_DAMPER_OUTPUT = 0x42d0,  // [R] Damper output
            VAR_IN_TEMP_ZONE2_F = 0x42d4,  // [R] Zone2 Room Temp. | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_OUTLET_ZONE1_F = 0x42d8,  // [R] Zone1 WaterOut Temp | Celsius | /10 | signed | -41..150
            VAR_IN_TEMP_WATER_OUTLET_ZONE2_F = 0x42d9,  // [R] Zone2 WaterOut Temp | Celsius | /10 | signed | -41..150
            VAR_IN_FLOW_SENSOR_CALC = 0x42e9,  // [R] Flow Sensor | /10 | signed | 0..80
            VAR_out_sensor_highpress = 0x8206,  // [R] High Pressure / PS_H / HPS | kgfcm2 | /10 | signed
            VAR_out_sensor_lowpress = 0x8208,  // [R] Low Pressure / PS_L | kgfcm2 | /10 | signed
            VAR_out_sensor_discharge1 = 0x820a,  // [R] Discharge1 / T_D / T_D1 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_discharge2 = 0x820c,  // [R] Discharge2 / T_D2 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_condout = 0x8218,  // [R] CondOut Temp. / T_CO | Celsius | /10 | signed | -41..150
            VAR_out_sensor_suction = 0x821a,  // [R] Suction1 Temp. / T_S / T_S1 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_doubletube = 0x821c,  // [R] Liquid Tube Temp. / T_L | Celsius | /10 | signed | -41..150
            VAR_outcd__sensor_eviin = 0x821e,  // [R] ESC IN / EVI IN / T_EI / ESC In | Celsius | /10 | signed | -41..150
            VAR_out_sensor_eviout = 0x8220,  // [R] ESC OUT / EVI OUT / T_EO / T_EC / ESC Out | Celsius | /10 | signed | -41..150
            VAR_out_control_target_discharge = 0x8223,  // [R] Target Discharge | Celsius | /10 | signed | -41..150
            VAR_out_load_fanstep1 = 0x8226,  // [R] Outdoor Fan Step | 0..10000
            VAR_OUT_LOAD_OUTEEV3 = 0x822b,  // [R] EEV 3 | 0..10000
            VAR_OUT_LOAD_OUTEEV4 = 0x822c,  // [R] EEV 4 | 0..10000
            VAR_OUT_LOAD_OUTEEV5 = 0x822d,  // [R] EEV 5 | 0..10000
            VAR_out_load_evieev = 0x822e,  // [R] ESC EEV / EVI EEV / E_EV | 0..10000
            VAR_out_control_order_cfreq_comp1 = 0x8236,  // [R] Order Frequency1 | 0..1000
            VAR_out_control_target_cfreq_comp1 = 0x8237,  // [R] Target Frequency1 | 0..1000
            VAR_out_control_cfreq_comp1 = 0x8238,  // [R] Current Frequency1 / current freq / current freq 1 | 0..1000
            VAR_out_sensor_dclink_voltage = 0x823b,  // [R] DC Link1
            VAR_out_load_fanrpm1 = 0x823d,  // [R] Fan RPM / Fan1 RPM
            VAR_out_sensor_IPM1 = 0x8254,  // [R] IPM1 Temp / IPMC1 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_IPM2 = 0x8255,  // [R] IPM2 Temp / IPMC2 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_temp_water = 0x825e,  // [R] Water Temp. / T_W | Celsius | /10 | signed | -41..150
            VAR_OUT_SENSOR_PIPEIN1 = 0x825f,  // [R] Outdoor Pipe IN 1 | Celsius | /10 | signed | -50..94
            VAR_OUT_SENSOR_PIPEIN2 = 0x8260,  // [R] Outdoor Pipe IN 2 | Celsius | /10 | signed | -50..94
            VAR_out_mcu_sensor_subcooler_in = 0x826b,  // [R] SubCooler In Temp. | Celsius | /10 | signed | -41..150
            VAR_out_mcu_sensor_subcooler_out = 0x826c,  // [R] SubCooler Out Temp. | Celsius | /10 | signed | -41..150
            VAR_out_mcu_subcooler_eev = 0x826d,  // [R] SubCool EEV
            VAR_out_mcu_change_over_eev1 = 0x826e,  // [R] A-EEV
            VAR_out_mcu_change_over_eev2 = 0x826f,  // [R] B-EEV
            VAR_out_mcu_change_over_eev3 = 0x8270,  // [R] C-EEV
            VAR_out_mcu_change_over_eev4 = 0x8271,  // [R] D-EEV
            VAR_out_mcu_change_over_eev5 = 0x8272,  // [R] E-EEV
            VAR_out_mcu_change_over_eev6 = 0x8273,  // [R] F-EEV
            VAR_out_control_cfreq_comp2 = 0x8276,  // [R] Current Frequency2 / current freq 2 | 0..1000
            VAR_out_sensor_ct2 = 0x8277,  // [R] Compressor Current2 | /10 | 0..10000
            VAR_out_control_dsh1 = 0x827a,  // [R] DSH | Celsius | /10 | signed | -41..150
            VAR_out_sensor_top2 = 0x8281,  // [R] Comp Top2 / T_CT2 | Celsius | /10 | signed | -41..150
            VAR_out_install_capa = 0x8287,  // [R] HP / Capacity | HP
            VAR_out_sensor_suction2_1sec = 0x829a,  // [R] Suction2 Temp. / T_S2 | Celsius | /10 | signed | -41..150
            VAR_out_sensor_sat_temp_high_pressure = 0x829f,  // [R] Saturated T_Pd | Celsius | /10 | signed | -41..150
            VAR_out_sensor_sat_temp_low_pressure = 0x82a0,  // [R] Saturated T_Ps | Celsius | /10 | signed | -41..150
            VAR_out_sensor_midpress = 0x82b8,  // [R] Mid Pressure | kgfcm2 | /10 | signed
            VAR_OUT_LOAD_FLUX_VARIABLE_VALVE = 0x82bd,  // [R] Flow control | /10 | signed
            VAR_OUT_SENSOR_CONTROL_BOX = 0x82be,  // [R] Control Box Temp. / T_C/box / T_CB | Celsius | /10 | signed
            VAR_out_sensor_condout2 = 0x82bf,  // [R] CondOut2 Temp. | Celsius | /10 | signed | -41..150
            VAR_OUT_LOAD_MCU_HR_BYPASS_EEV = 0x82e8,  // [R] MCU Bypass EEV
            VAR_OUT_SENSOR_PFCM1 = 0x82e9,  // [R] PFCM Temperature | Celsius | /10 | signed | -54..3000
            VAR_OUT_SENSOR_SUCTION3_1SEC = 0x82f9,  // [R] Suction3 Temp. | Celsius | /10 | signed | -41..150

            // LongVariable
            LVAR_In_Device_staus_Heatpump_Boiler = 0x440a,  // [R] Flow Switch | & 0x00000002


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
