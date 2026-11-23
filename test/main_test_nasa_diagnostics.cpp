#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

#include "esphome/core/log.h"
#include "test_stuff.h"
#include "../components/samsung_ac/log.h"
#include "../components/samsung_ac/nasa_diagnostics.h"
#include "../components/samsung_ac/protocol_nasa.h"

using namespace esphome::samsung_ac;

namespace
{
    Packet make_packet(DataType type = DataType::Notification)
    {
        Packet packet{};
        packet.sa = {AddressClass::Indoor, 0, 1};
        packet.da = {AddressClass::BroadcastControlAndSetLayer, 0, 0xFF};
        packet.command.dataType = type;
        packet.command.packetNumber = 1;
        return packet;
    }

    MessageSet scalar(uint16_t code, MessageSetType type, long value)
    {
        MessageSet message(static_cast<MessageNumber>(code));
        message.type = type;
        message.value = value;
        return message;
    }

    void expect_line(const std::string &actual, const std::string &expected)
    {
        if (actual != expected)
            std::cerr << "Expected: " << expected << "\nActual:   " << actual << '\n';
        assert(actual == expected);
    }

    void check_line_bound(const std::string &line)
    {
        assert(!line.empty() && line.size() <= 384);
        assert(std::all_of(line.begin(), line.end(), [](unsigned char byte) { return byte >= 0x20 && byte <= 0x7E; }));
    }

    void test_scalar_contract()
    {
        const auto packet = make_packet();
        expect_line(format_nasa_diagnostic_scalar(packet, scalar(0x4203, Variable, 233), 0),
                    "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x4203 type=variable "
                    "name=indoor_room_temperature raw=233 raw_hex=0x00E9 value=23.3 unit=C status=decoded confidence=documented");
        expect_line(format_nasa_diagnostic_scalar(packet, scalar(0x8204, Variable, 0xFFF6), 2),
                    "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=2 code=0x8204 type=variable "
                    "name=outdoor_air_temperature raw=65526 raw_hex=0xFFF6 value=-1.0 unit=C status=decoded confidence=documented");
        expect_line(format_nasa_diagnostic_scalar(packet, scalar(0x8414, LongVariable, 16777217), 3),
                    "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=3 code=0x8414 type=long_variable "
                    "name=outdoor_electrical_energy_total raw=16777217 raw_hex=0x01000001 value=16777.217 unit=kWh "
                    "status=decoded confidence=documented");
        expect_line(format_nasa_diagnostic_scalar(make_packet(DataType::Read), scalar(0x4201, Variable, 280), 0),
                    "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=read seq=1 index=0 code=0x4201 type=variable "
                    "name=indoor_setpoint raw=280 raw_hex=0x0118 value=na unit=na status=read_request confidence=documented");
    }

    void test_scalar_conversions_and_fallbacks()
    {
        const auto packet = make_packet();
        const struct
        {
            uint16_t code;
            MessageSetType type;
            long value;
            const char *expected;
        } cases[] = {
            {0x4002, Enum, 0, "name=indoor_mode_reported raw=0 raw_hex=0x00 value=UNKNOWN unit=na status=decoded confidence=documented"},
            {0x8238, Variable, 30, "name=compressor_1_frequency_actual raw=30 raw_hex=0x001E value=30 unit=Hz status=decoded confidence=documented"},
            {0x4204, Variable, -1, "name=indoor_temperature_0x4204 raw=65535 raw_hex=0xFFFF value=-0.1 unit=C status=decoded confidence=provisional"},
            {0x4205, Variable, 0xFE0C, "name=indoor_evaporator_inlet_temperature raw=65036 raw_hex=0xFE0C value=-50.0 unit=C status=decoded confidence=documented"},
            {0x4206, Variable, 0xFE0C, "name=indoor_evaporator_outlet_temperature raw=65036 raw_hex=0xFE0C value=-50.0 unit=C status=decoded confidence=documented"},
            {0x8217, Variable, 31, "name=compressor_1_ct_current raw=31 raw_hex=0x001F value=3.1 unit=A status=decoded confidence=documented"},
            {0x0402, LongVariable, 0x12345678, "name=remote_controller_address raw=305419896 raw_hex=0x12345678 value=120 unit=na status=decoded confidence=documented"},
            {0x0411, LongVariable, 0x00F90011, "name=ad_install_level_cool_mode_upper raw=16318481 raw_hex=0x00F90011 value=24.9 unit=C status=decoded confidence=documented"},
            {0x823E, Variable, 810, "name=outdoor_fan_channel_2_speed raw=810 raw_hex=0x032A value=810 unit=rpm status=decoded confidence=provisional"},
            {0x8413, LongVariable, 684, "name=outdoor_electrical_power_aggregate raw=684 raw_hex=0x000002AC value=684 unit=W status=decoded confidence=documented"},
            {0x8414, LongVariable, -1, "name=outdoor_electrical_energy_total raw=4294967295 raw_hex=0xFFFFFFFF value=4294967.295 unit=kWh status=decoded confidence=documented"},
            {0x8413, LongVariable, -2147483647L - 1, "name=outdoor_electrical_power_aggregate raw=2147483648 raw_hex=0x80000000 value=2147483648 unit=W status=decoded confidence=documented"},
            {0x823B, Variable, 151, "name=inverter_dc_link_voltage raw=151 raw_hex=0x0097 value=na unit=na status=raw_only confidence=provisional"},
            {0x4211, Variable, 30, "name=indoor_capacity_request raw=30 raw_hex=0x001E value=na unit=na status=raw_only confidence=provisional"},
            {0x4038, Enum, 0, "name=humidity_unverified raw=0 raw_hex=0x00 value=na unit=na status=raw_only confidence=provisional"},
            {0x421C, Variable, 971, "name=unknown_0x421C raw=971 raw_hex=0x03CB value=na unit=na status=unknown_code confidence=unknown"},
            {0x429D, Variable, 965, "name=unknown_0x429D raw=965 raw_hex=0x03C5 value=na unit=na status=unknown_code confidence=unknown"},
            {0x4016, Enum, 1, "name=unknown_0x4016 raw=1 raw_hex=0x01 value=na unit=na status=unknown_code confidence=unknown"},
            {0x4001, Enum, 0, "name=indoor_mode_selected raw=0 raw_hex=0x00 value=AUTO unit=na status=decoded confidence=documented"},
            {0x4002, Enum, 11, "name=indoor_mode_reported raw=11 raw_hex=0x0B value=AUTO_COOL unit=na status=decoded confidence=documented"},
            {0x4002, Enum, 255, "name=indoor_mode_reported raw=255 raw_hex=0xFF value=NULL unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 13, "name=indoor_fan_reported raw=13 raw_hex=0x0D value=UL unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 14, "name=indoor_fan_reported raw=14 raw_hex=0x0E value=LL unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 15, "name=indoor_fan_reported raw=15 raw_hex=0x0F value=HH unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 16, "name=indoor_fan_reported raw=16 raw_hex=0x10 value=SPEED unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 17, "name=indoor_fan_reported raw=17 raw_hex=0x11 value=NATURAL_LOW unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 18, "name=indoor_fan_reported raw=18 raw_hex=0x12 value=NATURAL_MID unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 19, "name=indoor_fan_reported raw=19 raw_hex=0x13 value=NATURAL_HIGH unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 510, "name=indoor_fan_reported raw=254 raw_hex=0xFE value=OFF unit=na status=decoded confidence=documented"},
            {0x8061, Enum, 255, "name=indoor_deice_step raw=255 raw_hex=0xFF value=NO_DEFROST unit=na status=decoded confidence=documented"},
            {0x4001, Enum, 14, "name=indoor_mode_selected raw=14 raw_hex=0x0E value=na unit=na status=unmapped_enum confidence=documented"},
            {0x4007, Enum, 0, "name=indoor_fan_reported raw=0 raw_hex=0x00 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x4045, Enum, 0, "name=quiet_mode_available raw=0 raw_hex=0x00 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8001, Enum, 21, "name=outdoor_operation_state raw=21 raw_hex=0x15 value=NONSTOP_HEAT_COOL_CHANGE unit=na status=decoded confidence=documented"},
            {0x8001, Enum, 32, "name=outdoor_operation_state raw=32 raw_hex=0x20 value=SMART_INSTALL_TEST unit=na status=decoded confidence=documented"},
            {0x8001, Enum, 33, "name=outdoor_operation_state raw=33 raw_hex=0x21 value=DEFROST_PERFORMANCE_TEST unit=na status=decoded confidence=documented"},
            {0x8001, Enum, 34, "name=outdoor_operation_state raw=34 raw_hex=0x22 value=INVERTER_FAN_PBA_CHECK unit=na status=decoded confidence=documented"},
            {0x8003, Enum, 0, "name=outdoor_thermal_mode raw=0 raw_hex=0x00 value=UNDEFINED unit=na status=decoded confidence=documented"},
            {0x8235, Variable, 0, "name=outdoor_error_code raw=0 raw_hex=0x0000 value=NO_ERROR unit=na status=decoded confidence=documented"},
            {0x8235, Variable, 256, "name=outdoor_error_code raw=256 raw_hex=0x0100 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8235, Enum, 0, "name=outdoor_error_code raw=0 raw_hex=0x00 value=na unit=na status=type_mismatch confidence=documented"},
            {0x4203, LongVariable, 65536, "name=indoor_room_temperature raw=65536 raw_hex=0x00010000 value=na unit=na status=type_mismatch confidence=documented"},
            {0x821A, LongVariable, 0xFFFF, "name=compressor_suction_temperature raw=65535 raw_hex=0x0000FFFF value=na unit=na status=type_mismatch confidence=documented"},
        };
        for (const auto &entry : cases)
        {
            const auto line = format_nasa_diagnostic_scalar(packet, scalar(entry.code, entry.type, entry.value),
                                                            std::numeric_limits<std::size_t>::max());
            check_line_bound(line);
            expect_line(line.substr(line.find("name=")), entry.expected);
        }
    }

    void test_markers_are_per_code()
    {
        const auto packet = make_packet();
        for (uint16_t code : {0x0202, 0x8202, 0x821A, 0x8248, 0x824F, 0x8274, 0x8275, 0x42D1, 0x42D2, 0x42D3})
        {
            const auto line = format_nasa_diagnostic_scalar(packet, scalar(code, Variable, 0xFFFF), 0);
            assert(line.find("raw=65535 raw_hex=0xFFFF value=na unit=na status=unverified_marker ") != std::string::npos);
            assert(format_nasa_diagnostic_scalar(packet, scalar(code, Variable, 0xFFFE), 0).find("unverified_marker") == std::string::npos);
        }
        for (uint16_t code : {0x8262, 0x8263, 0x8267, 0x8268})
        {
            const auto line = format_nasa_diagnostic_scalar(packet, scalar(code, Variable, 0xFE0C), 0);
            assert(line.find("raw=65036 raw_hex=0xFE0C value=na unit=na status=unverified_marker ") != std::string::npos);
            assert(format_nasa_diagnostic_scalar(packet, scalar(code, Variable, 0xFE0D), 0).find("value=-49.9 unit=C status=decoded ") != std::string::npos);
        }
    }

    void test_snet_service_readings()
    {
        const auto packet = make_packet();
        const struct
        {
            uint16_t code;
            MessageSetType type;
            long value;
            const char *expected;
        } cases[] = {
            {0x4076, Enum, 0, "name=indoor_room_sensor_selected raw=0 raw_hex=0x00 value=INDOOR_UNIT unit=na status=decoded confidence=documented"},
            {0x4076, Enum, 1, "name=indoor_room_sensor_selected raw=1 raw_hex=0x01 value=WIRED_REMOTE unit=na status=decoded confidence=documented"},
            {0x4076, Enum, 2, "name=indoor_room_sensor_selected raw=2 raw_hex=0x02 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8017, Enum, 0, "name=hot_gas_valve_1_drive raw=0 raw_hex=0x00 value=OFF unit=na status=decoded confidence=documented"},
            {0x8017, Enum, 1, "name=hot_gas_valve_1_drive raw=1 raw_hex=0x01 value=ON unit=na status=decoded confidence=documented"},
            {0x8017, Enum, 2, "name=hot_gas_valve_1_drive raw=2 raw_hex=0x02 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8229, Variable, 0, "name=outdoor_eev_1_position raw=0 raw_hex=0x0000 value=0 unit=na status=decoded confidence=documented"},
            {0x822A, Variable, 158, "name=outdoor_eev_2_position raw=158 raw_hex=0x009E value=158 unit=na status=decoded confidence=documented"},
            {0x822B, Variable, 240, "name=outdoor_eev_3_position raw=240 raw_hex=0x00F0 value=240 unit=na status=decoded confidence=documented"},
            {0x822C, Variable, 310, "name=outdoor_eev_4_position raw=310 raw_hex=0x0136 value=310 unit=na status=decoded confidence=documented"},
            {0x822D, Variable, 10000, "name=outdoor_eev_5_position raw=10000 raw_hex=0x2710 value=10000 unit=na status=decoded confidence=documented"},
            {0x82F5, Variable, 0x1203, "name=outdoor_overload_detection raw=4611 raw_hex=0x1203 value=count:18,status:3 unit=na status=decoded confidence=documented"},
            {0x82F5, Variable, 0xFF00, "name=outdoor_overload_detection raw=65280 raw_hex=0xFF00 value=count:255,status:0 unit=na status=decoded confidence=documented"},
            {0x4217, Variable, 120, "name=indoor_eev_1_position raw=120 raw_hex=0x0078 value=120 unit=na status=decoded confidence=documented"},
            {0x4218, Variable, 3000, "name=indoor_eev_2_position raw=3000 raw_hex=0x0BB8 value=3000 unit=na status=decoded confidence=documented"},
            {0x8046, Enum, 0, "name=outdoor_test_completion raw=0 raw_hex=0x00 value=NOT_COMPLETED unit=na status=decoded confidence=documented"},
            {0x8046, Enum, 1, "name=outdoor_test_completion raw=1 raw_hex=0x01 value=COMPLETED unit=na status=decoded confidence=documented"},
            {0x8046, Enum, 2, "name=outdoor_test_completion raw=2 raw_hex=0x02 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8206, Variable, 185, "name=outdoor_high_pressure raw=185 raw_hex=0x00B9 value=18.5 unit=kgf/cm2 status=decoded confidence=documented"},
            {0x8208, Variable, 0xFFF9, "name=outdoor_low_pressure raw=65529 raw_hex=0xFFF9 value=-0.7 unit=kgf/cm2 status=decoded confidence=documented"},
            {0x821C, Variable, 0xFFDB, "name=outdoor_liquid_line_temperature raw=65499 raw_hex=0xFFDB value=-3.7 unit=C status=decoded confidence=documented"},
        };
        for (const auto &entry : cases)
        {
            const auto line = format_nasa_diagnostic_scalar(packet, scalar(entry.code, entry.type, entry.value), 0);
            check_line_bound(line);
            expect_line(line.substr(line.find("name=")), entry.expected);
        }
    }

    std::vector<std::string> capture_diagnostics(const std::function<void()> &run,
                                                 std::vector<std::string> *legacy_lines = nullptr)
    {
        std::vector<std::string> lines;
        auto previous = std::move(esphome::test_log_sink);
        esphome::test_log_sink = [&](const char *level, const char *tag, const std::string &line)
        {
            if (std::string(tag) != "samsung_ac.diagnostics" && line.find("NASA_DIAG") != 0)
            {
                if (legacy_lines != nullptr)
                    legacy_lines->push_back(line);
                return;
            }
            assert(std::string(level) == "DEBUG");
            assert(std::string(tag) == "samsung_ac.diagnostics");
            check_line_bound(line);
            lines.push_back(line);
        };
        run();
        esphome::test_log_sink = std::move(previous);
        return lines;
    }

    std::vector<std::string> capture_lines(const Packet &packet, unsigned repeats = 1)
    {
        return capture_diagnostics([&]()
        {
            for (unsigned i = 0; i < repeats; ++i)
                log_nasa_diagnostics(packet);
        });
    }

    void test_empty_packet_records()
    {
        const struct
        {
            DataType type;
            const char *name;
        } cases[] = {
            {DataType::Undefined, "undefined"}, {DataType::Read, "read"}, {DataType::Write, "write"},
            {DataType::Request, "request"}, {DataType::Notification, "notification"}, {DataType::Response, "response"},
            {DataType::Ack, "ack"}, {DataType::Nack, "nack"}, {static_cast<DataType>(8), "undefined"},
            {static_cast<DataType>(15), "undefined"}, {static_cast<DataType>(255), "undefined"},
        };
        for (const auto &entry : cases)
        {
            const auto lines = capture_lines(make_packet(entry.type));
            assert(lines.size() == 1);
            expect_line(lines[0], std::string("NASA_DIAG src=20.00.01 dst=b3.00.ff dt=") + entry.name + " seq=1 fields=0");
        }
    }

    void test_scalar_logging_keeps_order_and_duplicates()
    {
        auto packet = make_packet(DataType::Response);
        packet.sa = {AddressClass::Outdoor, 0x0A, 0xAB};
        packet.command.packetNumber = 255;
        packet.messages.push_back(scalar(0x4016, Enum, 1));
        packet.messages.push_back(scalar(0x8235, Variable, 0));
        const auto lines = capture_lines(packet, 2);
        assert(lines.size() == 4);
        expect_line(lines[0], "NASA_DIAG src=10.0a.ab dst=b3.00.ff dt=response seq=255 index=0 code=0x4016 type=enum "
                             "name=unknown_0x4016 raw=1 raw_hex=0x01 value=na unit=na status=unknown_code confidence=unknown");
        expect_line(lines[1], "NASA_DIAG src=10.0a.ab dst=b3.00.ff dt=response seq=255 index=1 code=0x8235 type=variable "
                             "name=outdoor_error_code raw=0 raw_hex=0x0000 value=NO_ERROR unit=na status=decoded confidence=documented");
        expect_line(lines[2], lines[0]);
        expect_line(lines[3], lines[1]);
        assert(packet.messages.size() == 2 && packet.messages[0].value == 1 && packet.messages[1].value == 0);
    }

    constexpr const char *STRUCTURE_PREFIX =
        "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x460A type=structure";

    Packet make_structure_packet(std::size_t length, unsigned seed)
    {
        auto packet = make_packet();
        packet.messages.emplace_back(static_cast<MessageNumber>(0x460A));
        auto &message = packet.messages[0];
        message.type = Structure;
        message.size = static_cast<uint16_t>(length + 2);
        message.structure = {};
        message.structure.size = static_cast<uint8_t>(length);
        for (std::size_t i = 0; i < length; ++i)
            message.structure.data[i] = static_cast<uint8_t>(i + seed);
        return packet;
    }

    void test_structure_fragments_are_lossless()
    {
        for (std::size_t length : {0, 32, 33, 255})
        {
            const auto packet = make_structure_packet(length, 0xF0);
            assert(format_nasa_diagnostic_scalar(packet, packet.messages[0], 0).empty());
            const auto lines = capture_lines(packet);
            assert(lines.size() == (length == 0 ? 1 : (length + 31) / 32));
            if (length == 0)
            {
                expect_line(lines[0], std::string(STRUCTURE_PREFIX) + " len=0 offset=0 hex=na");
                continue;
            }

            std::vector<uint8_t> payload;
            for (const auto &line : lines)
            {
                const auto prefix = std::string(STRUCTURE_PREFIX) + " len=" + std::to_string(length) +
                                    " offset=" + std::to_string(payload.size()) + " hex=";
                assert(line.compare(0, prefix.size(), prefix) == 0);
                const auto hex = line.substr(prefix.size());
                assert(hex.size() == 2 * std::min<std::size_t>(32, length - payload.size()));
                assert(hex.find_first_not_of("0123456789ABCDEF") == std::string::npos);
                for (std::size_t i = 0; i < hex.size(); i += 2)
                    payload.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
            }
            assert(payload.size() == length);
            for (std::size_t i = 0; i < length; ++i)
            {
                assert(payload[i] == static_cast<uint8_t>(i + 0xF0));
                assert(packet.messages[0].structure.data[i] == payload[i]);
            }
        }
        assert(capture_lines(make_structure_packet(32, 0xF0)) != capture_lines(make_structure_packet(32, 0xF1)));
    }

    void test_invalid_structure_metadata_has_no_payload()
    {
        const auto invalid = std::string(STRUCTURE_PREFIX) + " status=invalid_structure";
        auto packet = make_packet();
        packet.messages.reserve(2);
        packet.messages.emplace_back(static_cast<MessageNumber>(0x460A));
        auto &message = packet.messages[0];
        message.type = Structure;
        // Leave the union inactive for the outer guards, as the parser can do.
        for (uint16_t size : {1, 258})
        {
            message.size = size;
            assert(capture_lines(packet) == std::vector<std::string>{invalid});
        }
        message.size = 2; // Valid outer size, but the union remains inactive in a multi-field packet.
        packet.messages.push_back(scalar(0x4000, Enum, 1));
        const auto multiple = capture_lines(packet);
        assert(multiple.size() == 2);
        expect_line(multiple[0], invalid);
        assert(multiple[1].find(" index=1 code=0x4000 ") != std::string::npos);

        packet.messages.pop_back();
        message.size = 4;
        message.structure = {};
        message.structure.size = 1;
        assert(capture_lines(packet) == std::vector<std::string>{invalid});
    }

    void test_error_history_summary_gating()
    {
        auto packet = make_structure_packet(10, 0);
        auto &message = packet.messages.front();
        message.messageNumber = static_cast<MessageNumber>(0x461E);
        const std::string prefix = "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x461E type=structure";
        const auto notification = capture_lines(packet);
        assert(notification.size() == 2);
        expect_line(notification[0], prefix + " len=10 offset=0 hex=00010203040506070809");
        expect_line(notification[1], prefix + " len=10 name=indoor_error_history codes=0x001,0x203,0x405,0x607,0x809 status=decoded confidence=documented");

        for (DataType type : {DataType::Read, DataType::Write, DataType::Request, DataType::Ack, DataType::Nack, DataType::Undefined})
        {
            packet.command.dataType = type;
            const auto lines = capture_lines(packet);
            assert(lines.size() == 1 && lines[0].find(" offset=0 hex=00010203040506070809") != std::string::npos);
        }
        packet.command.dataType = DataType::Response;
        packet.sa.klass = AddressClass::Outdoor;
        assert(capture_lines(packet).size() == 1);
        packet.command.dataType = DataType::Notification;
        packet.sa.klass = AddressClass::Indoor;
        message.size = 11;
        message.structure.size = 9;
        assert(capture_lines(packet) == std::vector<std::string>{prefix + " len=9 offset=0 hex=000102030405060708"});
        message.structure.size = 10;
        assert(capture_lines(packet) == std::vector<std::string>{prefix + " status=invalid_structure"});
        message.size = 12;
        packet.messages.push_back(scalar(0x4000, Enum, 1));
        const auto multiple = capture_lines(packet);
        assert(multiple.size() == 2);
        expect_line(multiple[0], prefix + " status=invalid_structure");
    }

    // Two fields make dropped, duplicated, or reordered records visible within a reception.
    constexpr const char *INDOOR_NOTIFICATION = "320016200001b300ffc0140102420400e9421c03cb555734";
    const std::string ROOM_DIAGNOSTIC =
        "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x4204 type=variable "
        "name=indoor_temperature_0x4204 raw=233 raw_hex=0x00E9 value=23.3 unit=C status=decoded confidence=provisional";
    const std::string UNKNOWN_DIAGNOSTIC =
        "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=1 code=0x421C type=variable "
        "name=unknown_0x421C raw=971 raw_hex=0x03CB value=na unit=na status=unknown_code confidence=unknown";
    constexpr const char *ROOM_NOTIFICATION = "320012200000b000ffc0140201420400f44b4d34";
    const std::string SECOND_ROOM_DIAGNOSTIC =
        "NASA_DIAG src=20.00.00 dst=b0.00.ff dt=notification seq=2 index=0 code=0x4204 type=variable "
        "name=indoor_temperature_0x4204 raw=244 raw_hex=0x00F4 value=24.4 unit=C status=decoded confidence=provisional";

    struct Callback
    {
        std::string name;
        std::string address;
        uint16_t id = 0;
        float value = 0;

        bool operator==(const Callback &other) const
        {
            return std::tie(name, address, id, value) == std::tie(other.name, other.address, other.id, other.value);
        }
    };

    class RxTarget : public DebugTarget
    {
    public:
        std::vector<Callback> callbacks;

        void register_address(const std::string address) override
        {
            callbacks.push_back({"address", address});
        }
        void set_room_temperature(const std::string address, float value) override
        {
            callbacks.push_back({"room", address, 0, value});
        }
        void set_power(const std::string address, bool value) override
        {
            callbacks.push_back({"power", address, 0, value ? 1.0f : 0.0f});
        }
        void set_custom_sensor(const std::string address, uint16_t code, float value) override
        {
            callbacks.push_back({"custom", address, code, value});
        }
        void set_outdoor_cumulative_energy(const std::string &address, float value) override
        {
            callbacks.push_back({"energy", address, 0, value});
        }
        void ack_data(uint8_t id) override
        {
            callbacks.push_back({"ack", {}, id});
        }
        void set_display_light(const std::string &address, esphome::optional<bool> value) override
        {
            callbacks.push_back({value ? (value.value() ? "display_on" : "display_off") : "display_unconfirmed", address});
        }
    };

    void expect_records(const std::vector<std::string> &actual, const std::vector<std::string> &expected)
    {
        if (actual.size() != expected.size())
            std::cerr << "Expected " << expected.size() << " NASA_DIAG RX records, got " << actual.size() << '\n';
        assert(actual.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i)
            expect_line(actual[i], expected[i]);
    }

    class ServiceTarget : public RxTarget
    {
    public:
        using State = std::variant<float, bool, std::string>;
        using Sample = std::tuple<std::string, uint16_t, State>;
        std::vector<Sample> samples;
        bool subscribed = true;
        uint32_t now = 0;

        uint32_t get_miliseconds() override { return now; }

        bool wants_nasa_service_telemetry(const std::string &address, uint16_t code) override
        {
            return subscribed && (address == "20.00.01" || address == "10.00.00") && code != 0x822E;
        }

        void set_nasa_service_telemetry(const std::string &address, uint16_t code, const NasaServiceValue &value) override
        {
            samples.emplace_back(address, code, value.state);
        }
    };

    void test_service_scalars_publish_without_debug()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const auto previous_diagnostics = std::exchange(debug_log_diagnostics, false);
        const struct
        {
            Address source;
            uint16_t code;
            MessageSetType type;
            long raw;
            ServiceTarget::State expected;
        } cases[] = {
            {{AddressClass::Indoor, 0, 1}, 0x4076, Enum, 0, std::string("INDOOR_UNIT")},
            {{AddressClass::Indoor, 0, 1}, 0x4076, Enum, 1, std::string("WIRED_REMOTE")},
            {{AddressClass::Indoor, 0, 1}, 0x4027, Enum, 0, false},
            {{AddressClass::Indoor, 0, 1}, 0x4027, Enum, 1, true},
            {{AddressClass::Indoor, 0, 1}, 0x4028, Enum, 0, false},
            {{AddressClass::Indoor, 0, 1}, 0x4028, Enum, 1, true},
            {{AddressClass::Indoor, 0, 1}, 0x4217, Variable, 0, 0.0f},
            {{AddressClass::Indoor, 0, 1}, 0x4217, Variable, 120, 120.0f},
            {{AddressClass::Indoor, 0, 1}, 0x4217, Variable, 3000, 3000.0f},
            {{AddressClass::Indoor, 0, 1}, 0x4218, Variable, 0, 0.0f},
            {{AddressClass::Indoor, 0, 1}, 0x4218, Variable, 3000, 3000.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x8017, Enum, 0, false},
            {{AddressClass::Outdoor, 0, 0}, 0x8017, Enum, 1, true},
            {{AddressClass::Outdoor, 0, 0}, 0x8046, Enum, 0, false},
            {{AddressClass::Outdoor, 0, 0}, 0x8046, Enum, 1, true},
            {{AddressClass::Outdoor, 0, 0}, 0x8206, Variable, 185, 18.5f},
            {{AddressClass::Outdoor, 0, 0}, 0x8208, Variable, 0xFFF9, -0.7f},
            {{AddressClass::Outdoor, 0, 0}, 0x821C, Variable, 0xFFDB, -3.7f},
            {{AddressClass::Outdoor, 0, 0}, 0x821C, Variable, 0xFE66, -41.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x821C, Variable, 1500, 150.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x8229, Variable, 0, 0.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x822A, Variable, 158, 158.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x822B, Variable, 240, 240.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x822C, Variable, 310, 310.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x822D, Variable, 10000, 10000.0f},
            {{AddressClass::Outdoor, 0, 0}, 0x82F5, Variable, 0x1203, std::string("count:18,status:3")},
        };
        for (const auto &entry : cases)
        {
            auto packet = make_packet();
            packet.sa = entry.source;
            packet.messages = {scalar(entry.code, entry.type, entry.raw)};
            auto frame = packet.encode();
            ServiceTarget target;
            expect_records(capture_diagnostics([&]()
            {
                assert(process_data(frame, &target).type == DecodeResultType::Processed);
            }), {});
            auto address = packet.sa.to_string();
            const std::vector<ServiceTarget::Sample> expected = {{address, entry.code, entry.expected}};
            const std::vector<Callback> legacy = {{"address", address}, {"custom", address, entry.code, static_cast<float>(entry.raw)}};
            assert(target.samples == expected);
            assert(target.callbacks == legacy && target.all_publish_data.empty());
        }
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
    }

    void test_service_scalars_ignore_ineligible_samples()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        ServiceTarget target;
        const auto replay = [&](Address address, uint16_t code, MessageSetType type, long raw,
                                DataType data_type = DataType::Notification)
        {
            auto packet = make_packet(data_type);
            packet.sa = address;
            packet.messages = {scalar(code, type, raw)};
            auto frame = packet.encode();
            assert(process_data(frame, &target).type == DecodeResultType::Processed);
        };
        const Address outdoor{AddressClass::Outdoor, 0, 0};
        const Address indoor{AddressClass::Indoor, 0, 1};
        replay(indoor, 0x4076, Enum, 2);
        replay(indoor, 0x4027, Enum, 2);
        replay(indoor, 0x4028, Enum, 2);
        replay(outdoor, 0x8017, Enum, 2);
        replay(outdoor, 0x8046, Enum, 2);
        replay(indoor, 0x4217, Variable, 3001);
        replay(indoor, 0x4218, Variable, 3001);
        for (uint16_t code = 0x8229; code <= 0x822D; ++code)
            replay(outdoor, code, Variable, 10001);
        replay(outdoor, 0x821C, Variable, 0xFE65); // -41.1 °C.
        replay(outdoor, 0x821C, Variable, 1501);
        replay({AddressClass::Indoor, 0, 0}, 0x4076, Enum, 1);
        replay(indoor, 0x8208, Variable, 185);
        replay(outdoor, 0x822E, Variable, 100);
        replay(outdoor, 0x8003, Enum, 1);
        for (auto type : {DataType::Read, DataType::Request, DataType::Response})
            replay(outdoor, 0x8208, Variable, 185, type);

        auto short_scalar = hex_to_bytes("320011100000b300ffc01408018208ffa65134");
        assert(process_data(short_scalar, &target).type == DecodeResultType::Discard);
        auto packet = make_packet();
        packet.sa = outdoor;
        packet.messages = {scalar(0x8208, Variable, 185)};
        auto bad_crc = packet.encode();
        bad_crc[bad_crc.size() - 2] ^= 1;
        assert(process_data(bad_crc, &target).type == DecodeResultType::Discard);

        target.subscribed = false;
        replay(outdoor, 0x8208, Variable, 185);
        assert(target.samples.empty() && target.all_publish_data.empty());
        protocol_processing = previous_processing;
    }

    void test_service_ranges_do_not_hide_raw_diagnostics()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const auto previous_diagnostics = std::exchange(debug_log_diagnostics, true);
        ServiceTarget target;
        auto packet = make_packet();
        packet.messages = {scalar(0x4217, Variable, 3001)};
        auto frame = packet.encode();
        const auto lines = capture_diagnostics([&]()
        {
            assert(process_data(frame, &target).type == DecodeResultType::Processed);
        });
        const std::vector<Callback> legacy = {{"address", "20.00.01"}, {"custom", "20.00.01", 0x4217, 3001.0f}};
        assert(target.samples.empty() && target.callbacks == legacy && target.all_publish_data.empty());
        expect_records(lines, {
            "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x4217 type=variable "
            "name=indoor_eev_1_position raw=3001 raw_hex=0x0BB9 value=3001 unit=na status=decoded confidence=documented",
        });
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
    }

    void test_error_history_telemetry_is_independent_of_debug()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const auto previous_diagnostics = debug_log_diagnostics;
        const std::vector<ServiceTarget::Sample> expected = {
            {"20.00.01", 0x461E, std::string("0x123,0xBCD,0x000,0xFFF,0xFFF")},
        };
        for (bool debug : {false, true})
        {
            debug_log_diagnostics = debug;
            for (const auto *fixture : {
                     "32003e200001500000c0150801461ea1230bcd0000ffffffff"
                     "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223242f6e34",
                     "32003e200001500000c0140801461ea1230bcd0000ffffffff"
                     "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223246b1d34",
                 })
            {
                ServiceTarget target;
                const auto lines = capture_diagnostics([&]() { test_process_data(fixture, target); });
                assert(target.samples == expected && target.all_publish_data.empty());
                const std::vector<Callback> legacy = {{"address", "20.00.01"}};
                assert(target.callbacks == legacy);
                assert(lines.size() == (debug ? 3U : 0U));
                if (debug)
                    assert(lines.back().find("codes=0x123,0xBCD,0x000,0xFFF,0xFFF") != std::string::npos);
            }
        }
        ServiceTarget target;
        test_process_data("320019200001500000c0150801461ea1230bcd0000ffffffddc034", target);
        assert(target.samples.empty() && target.all_publish_data.empty());
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
    }

    void test_nasa_custom_override_receives_its_message_not_fixed_current()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        auto packet = make_packet();
        packet.sa = {AddressClass::Outdoor, 0, 0};
        packet.messages = {scalar(0x8217, Variable, 46), scalar(0x8206, Variable, 185)};
        auto frame = packet.encode();
        RxTarget target;
        assert(process_data(frame, &target).type == DecodeResultType::Processed);
        assert(target.last_set_outdoor_current_address == "10.00.00");
        assert(target.last_set_outdoor_current_value == 46.0f);
        const std::vector<Callback> legacy = {
            {"address", "10.00.00"}, {"custom", "10.00.00", 0x8217, 46.0f},
            {"custom", "10.00.00", 0x8206, 185.0f},
        };
        assert(target.callbacks == legacy && target.all_publish_data.empty());
        protocol_processing = previous_processing;
    }

    void test_expired_display_read_does_not_consume_error_history()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        auto &protocol = *static_cast<NasaProtocol *>(get_protocol("20.00.01"));
        auto previous_protocol = std::exchange(protocol, NasaProtocol{});
        ServiceTarget target;
        protocol.enable_display_light("20.00.01");
        protocol.protocol_update(&target);
        assert(target.all_publish_data.size() == 1);
        auto read_frame = hex_to_bytes(target.all_publish_data.front());
        Packet read{};
        assert(read.decode(read_frame).type == DecodeResultType::Processed);

        auto history = make_structure_packet(10, 0);
        history.da = read.sa;
        history.command.dataType = DataType::Response;
        history.command.packetNumber = read.command.packetNumber;
        history.messages.front().messageNumber = static_cast<MessageNumber>(0x461E);
        target.now = 2000;
        auto frame = history.encode();
        assert(process_data(frame, &target).type == DecodeResultType::Processed);
        const std::vector<ServiceTarget::Sample> expected = {
            {"20.00.01", 0x461E, std::string("0x001,0x203,0x405,0x607,0x809")},
        };
        assert(target.samples == expected && target.all_publish_data.size() == 1);
        protocol = std::move(previous_protocol);
        protocol_processing = previous_processing;
    }

    void test_error_history_response_is_passive()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const bool previous_diagnostics = std::exchange(debug_log_diagnostics, true);
        RxTarget target;
        const auto lines = capture_diagnostics([&]()
        {
            // Indoor response with a full 46-byte payload and a nonzero first high nibble.
            test_process_data("32003e200001500000c0150801461ea1230bcd0000ffffffff"
                              "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223242f6e34", target);
        });
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
        assert(target.all_publish_data.empty());
        const std::vector<Callback> callbacks = {{"address", "20.00.01"}};
        assert(target.callbacks == callbacks);
        const std::string prefix = "NASA_DIAG src=20.00.01 dst=50.00.00 dt=response seq=8 index=0 code=0x461E type=structure len=46";
        expect_records(lines, {
            prefix + " offset=0 hex=A1230BCD0000FFFFFFFF0102030405060708090A0B0C0D0E0F10111213141516",
            prefix + " offset=32 hex=1718191A1B1C1D1E1F2021222324",
            prefix + " name=indoor_error_history codes=0x123,0xBCD,0x000,0xFFF,0xFFF status=decoded confidence=documented",
        });
    }

    void test_energy_counter_preserves_wire_value()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const bool previous_diagnostics = std::exchange(debug_log_diagnostics, true);
        RxTarget target;
        const auto lines = capture_diagnostics([&]()
        {
            // Literal big-endian RX frames with CRCs; do not use the TX encoder.
            test_process_data("320014100000b000ffc01401018414010000018bb834", target);
            test_process_data("320014100000b000ffc014020184147fffffff61af34", target);
            test_process_data("320014100000b000ffc0140301841480000000bfb334", target);
            test_process_data("320014100000b000ffc01404018414ffffffff3c5c34", target);
        });
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
        assert(target.all_publish_data.empty());
        expect_records(lines, {
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=1 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=16777217 raw_hex=0x01000001 value=16777.217 unit=kWh status=decoded confidence=documented",
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=2 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=2147483647 raw_hex=0x7FFFFFFF value=2147483.647 unit=kWh status=decoded confidence=documented",
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=3 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=2147483648 raw_hex=0x80000000 value=2147483.648 unit=kWh status=decoded confidence=documented",
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=4 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=4294967295 raw_hex=0xFFFFFFFF value=4294967.295 unit=kWh status=decoded confidence=documented",
        });
    }

    void test_unaccepted_nasa_rx_is_silent()
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const bool previous_diagnostics = std::exchange(debug_log_diagnostics, true);
        auto frame = hex_to_bytes(ROOM_NOTIFICATION);
        auto prefix = frame;
        prefix.pop_back(); // Same scalar frame, still waiting for the final byte.
        RxTarget target;
        expect_records(capture_diagnostics([&]()
        {
            assert(process_data(prefix, &target).type == DecodeResultType::Fill);
        }), {});
        assert(target.callbacks.empty() && target.all_publish_data.empty());

        // Complete the same frame as a positive control before checking CRC rejection.
        prefix.push_back(frame.back());
        expect_records(capture_diagnostics([&]()
        {
            assert(process_data(prefix, &target).type == DecodeResultType::Processed);
        }), {SECOND_ROOM_DIAGNOSTIC});
        const std::vector<Callback> callbacks = {
            {"address", "20.00.00"}, {"custom", "20.00.00", 0x4204, 244.0f}, {"room", "20.00.00", 0, 24.4f},
        };
        assert(target.callbacks == callbacks && target.all_publish_data.empty());

        frame.at(frame.size() - 2) ^= 1; // Flip one CRC bit; leave framing and scalar data intact.
        target.callbacks.clear();
        expect_records(capture_diagnostics([&]()
        {
            assert(process_data(frame, &target).type == DecodeResultType::Discard);
        }), {});
        assert(target.callbacks.empty() && target.all_publish_data.empty());
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
    }

    struct RxTrace
    {
        std::vector<std::string> diagnostics;
        std::vector<Callback> callbacks;
        std::vector<std::string> transmissions;
    };

    RxTrace run_nasa_rx(bool diagnostics_enabled)
    {
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const bool previous_diagnostics = std::exchange(debug_log_diagnostics, diagnostics_enabled);
        RxTarget target;
        auto lines = capture_diagnostics([&]()
        {
            test_process_data(INDOOR_NOTIFICATION, target);
            // Same sender and sequence as the preceding 0x4204 frame, but 0x4203 must not update climate.
            test_process_data("320012200001b300ffc01401014203fff4784d34", target);
            test_process_data(ROOM_NOTIFICATION, target); // Same code, different addresses.
            test_process_data("32000e20000180ff00c016030096ac34", target); // Empty ACK.
            test_process_data("32000e20000180ff00c0170400380b34", target); // Empty NACK.
            test_process_data("320012500000200001c01105014201000075ce34", target); // Read.
            test_process_data("32001280ff00200002c013f201420101186e5434", target); // Existing NASA Request fixture.
            test_process_data("320012500000200001c0120701420100dc2cfd34", target); // Write.
            // Explicit big-endian LongVariable RX bytes: 0x8414, 0x000F6868 Wh.
            test_process_data("320014100000b000ffc01406018414000f6868692934", target);
            test_process_data("320013200001500000c0150801460a1234ab259e34", target); // Single structure Response.
            get_protocol("20.00.02")->protocol_update(&target);
        });
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
        return {std::move(lines), std::move(target.callbacks), std::move(target.all_publish_data)};
    }

    void test_accepted_nasa_rx_is_passive()
    {
        assert(!debug_log_messages && !debug_log_undefined_messages && !debug_log_raw_bytes);
        const auto disabled = run_nasa_rx(false);
        const auto enabled = run_nasa_rx(true);
        expect_records(disabled.diagnostics, {});
        const std::vector<Callback> callbacks = {
            {"address", "20.00.01"}, {"custom", "20.00.01", 0x4204, 233.0f}, {"room", "20.00.01", 0, 23.3f},
            {"custom", "20.00.01", 0x421C, 971.0f},
            {"address", "20.00.01"}, {"custom", "20.00.01", 0x4203, 65524.0f},
            {"address", "20.00.00"}, {"custom", "20.00.00", 0x4204, 244.0f}, {"room", "20.00.00", 0, 24.4f},
            {"address", "20.00.01"}, {"ack", {}, 3}, {"address", "20.00.01"},
            {"address", "50.00.00"}, {"address", "80.ff.00"}, {"address", "50.00.00"},
            {"address", "10.00.00"}, {"custom", "10.00.00", 0x8414, 1009768.0f}, {"energy", "10.00.00", 0, 1009768.0f},
            {"address", "20.00.01"},
        };
        assert(disabled.callbacks == callbacks);
        assert(enabled.callbacks == disabled.callbacks);
        assert(disabled.transmissions.empty());
        assert(enabled.transmissions == disabled.transmissions);
        expect_records(enabled.diagnostics, {
            ROOM_DIAGNOSTIC,
            UNKNOWN_DIAGNOSTIC,
            "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x4203 type=variable "
            "name=indoor_room_temperature raw=65524 raw_hex=0xFFF4 value=-1.2 unit=C status=decoded confidence=documented",
            SECOND_ROOM_DIAGNOSTIC,
            "NASA_DIAG src=20.00.01 dst=80.ff.00 dt=ack seq=3 fields=0",
            "NASA_DIAG src=20.00.01 dst=80.ff.00 dt=nack seq=4 fields=0",
            "NASA_DIAG src=50.00.00 dst=20.00.01 dt=read seq=5 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=0 raw_hex=0x0000 value=na unit=na status=read_request confidence=documented",
            "NASA_DIAG src=80.ff.00 dst=20.00.02 dt=request seq=242 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=280 raw_hex=0x0118 value=28.0 unit=C status=decoded confidence=documented",
            "NASA_DIAG src=50.00.00 dst=20.00.01 dt=write seq=7 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=220 raw_hex=0x00DC value=22.0 unit=C status=decoded confidence=documented",
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=6 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=1009768 raw_hex=0x000F6868 value=1009.768 unit=kWh status=decoded confidence=documented",
            "NASA_DIAG src=20.00.01 dst=50.00.00 dt=response seq=8 index=0 code=0x460A type=structure len=3 offset=0 hex=1234AB",
        });
    }

    void receive_display_reply(RxTarget &target, const std::string &read)
    {
        auto bytes = hex_to_bytes(read);
        Packet reply;
        assert(reply.decode(bytes).type == DecodeResultType::Processed);
        assert(reply.command.dataType == DataType::Read);
        assert(reply.messages.size() == 1 && reply.messages[0].messageNumber == MessageNumber::ENUM_in_display_light);
        std::swap(reply.sa, reply.da);
        reply.command.dataType = DataType::Response;
        reply.messages[0].value = 1;
        test_process_data(bytes_to_hex(reply.encode()), target);
    }

    RxTrace run_power_rx(bool diagnostics_enabled)
    {
        constexpr const char *POWER_NOTIFICATION = "320011200002b300ffc0140901400001588034";
        const auto previous_processing = std::exchange(protocol_processing, ProtocolProcessing::NASA);
        const bool previous_diagnostics = std::exchange(debug_log_diagnostics, false);
        const bool previous_messages = std::exchange(debug_log_messages, true);
        const bool previous_undefined_messages = std::exchange(debug_log_undefined_messages, true);
        const bool previous_on_change = std::exchange(debug_log_messages_on_change, true);
        auto &protocol = *static_cast<NasaProtocol *>(get_protocol("20.00.02"));
        // RX dispatch uses the shared protocol. Restore its state after each run.
        auto previous_protocol = std::exchange(protocol, NasaProtocol{});

        expect_records(capture_diagnostics([&]()
        {
            RxTarget setup;
            // Warm this test's own legacy dedup key before enabling the display.
            test_process_data(POWER_NOTIFICATION, setup);
            protocol.enable_display_light("20.00.02");
            protocol.protocol_update(&setup);
            assert(setup.all_publish_data.size() == 1);
            receive_display_reply(setup, setup.all_publish_data.front());
            assert(!setup.callbacks.empty() && setup.callbacks.back().name == "display_on");
        }), {});

        debug_log_diagnostics = diagnostics_enabled;
        RxTarget target;
        std::vector<std::string> legacy;
        auto lines = capture_diagnostics([&]()
        {
            protocol.protocol_update(&target); // The startup Read is complete.
            assert(target.callbacks.empty() && target.all_publish_data.empty());
            test_process_data(POWER_NOTIFICATION, target);
            assert(target.all_publish_data.empty());
            protocol.protocol_update(&target);
            assert(target.all_publish_data.size() == 1);
            receive_display_reply(target, target.all_publish_data.front());
            protocol.protocol_update(&target);
        }, &legacy);
        assert(std::none_of(legacy.begin(), legacy.end(), [](const std::string &line)
        {
            return line.find("s:20.00.02 d:b3.00.ff ENUM_in_operation_power ") == 0;
        }));

        protocol = std::move(previous_protocol);
        debug_log_messages_on_change = previous_on_change;
        debug_log_undefined_messages = previous_undefined_messages;
        debug_log_messages = previous_messages;
        debug_log_diagnostics = previous_diagnostics;
        protocol_processing = previous_processing;
        return {std::move(lines), std::move(target.callbacks), std::move(target.all_publish_data)};
    }

    void test_diagnostics_preserve_power_driven_display_read()
    {
        const auto disabled = run_power_rx(false);
        const auto enabled = run_power_rx(true);
        const std::vector<Callback> callbacks = {
            {"address", "20.00.02"}, {"display_unconfirmed", "20.00.02"},
            {"custom", "20.00.02", 0x4000, 1.0f}, {"power", "20.00.02", 0, 1.0f},
            {"display_unconfirmed", "20.00.02"}, {"address", "20.00.02"}, {"display_on", "20.00.02"},
        };
        assert(disabled.callbacks == callbacks);
        assert(enabled.callbacks == callbacks);
        // Startup consumes sequences 1 and 3; compare the full Read, including sequence and CRC.
        assert(disabled.transmissions == std::vector<std::string>{"32001180ff00200002c0110201401600918734"});
        assert(enabled.transmissions == std::vector<std::string>{"32001180ff00200002c01104014016005c0234"});
        expect_records(disabled.diagnostics, {});
        expect_records(enabled.diagnostics, {
            "NASA_DIAG src=20.00.02 dst=b3.00.ff dt=notification seq=9 index=0 code=0x4000 type=enum "
            "name=indoor_power raw=1 raw_hex=0x01 value=ON unit=na status=decoded confidence=documented",
            "NASA_DIAG src=20.00.02 dst=80.ff.00 dt=response seq=4 index=0 code=0x4016 type=enum "
            "name=unknown_0x4016 raw=1 raw_hex=0x01 value=na unit=na status=unknown_code confidence=unknown",
        });
    }

    void test_repeated_rx_ignores_legacy_deduplication()
    {
        debug_log_diagnostics = true;
        debug_log_messages = true;
        debug_log_undefined_messages = true;
        debug_log_messages_on_change = true;
        protocol_processing = ProtocolProcessing::NASA;
        RxTarget target;
        std::vector<std::string> legacy;
        const auto lines = capture_diagnostics([&]()
        {
            test_process_data(INDOOR_NOTIFICATION, target);
            test_process_data(INDOOR_NOTIFICATION, target);
        }, &legacy);
        debug_log_diagnostics = false;
        debug_log_messages = false;
        debug_log_undefined_messages = false;
        debug_log_messages_on_change = false;
        const auto count_legacy = [&](const std::string &prefix)
        {
            return std::count_if(legacy.begin(), legacy.end(), [&](const std::string &line)
            {
                return line.find(prefix) == 0;
            });
        };
        assert(count_legacy("MSG:") == 2 && count_legacy("RECV:") == 2);
        assert(count_legacy("s:20.00.01 d:b3.00.ff VAR_in_temp_room_f ") == 1);
        assert(count_legacy("Undefined s:20.00.01 d:b3.00.ff ") == 1);
        expect_records(lines, {ROOM_DIAGNOSTIC, UNKNOWN_DIAGNOSTIC, ROOM_DIAGNOSTIC, UNKNOWN_DIAGNOSTIC});
    }

    void test_non_nasa_rx_is_excluded()
    {
        debug_log_diagnostics = true;
        protocol_processing = ProtocolProcessing::Auto;
        DebugTarget target;
        const auto lines = capture_diagnostics([&]()
        {
            test_process_data("3200c8204b504e000110004ee234", target); // Existing Non-NASA Cmd20 fixture.
        });
        debug_log_diagnostics = false;
        assert(protocol_processing == ProtocolProcessing::NonNASA);
        assert(target.last_register_address == "00");
        assert(target.last_set_room_temperature_address == "00" && target.last_set_room_temperature_value == 25.0f);
        expect_records(lines, {});
    }
} // namespace

int main()
{
    test_scalar_contract();
    test_scalar_conversions_and_fallbacks();
    test_markers_are_per_code();
    test_snet_service_readings();
    test_energy_counter_preserves_wire_value();
    test_empty_packet_records();
    test_scalar_logging_keeps_order_and_duplicates();
    test_structure_fragments_are_lossless();
    test_invalid_structure_metadata_has_no_payload();
    test_error_history_summary_gating();
    test_error_history_response_is_passive();
    test_service_scalars_publish_without_debug();
    test_service_scalars_ignore_ineligible_samples();
    test_service_ranges_do_not_hide_raw_diagnostics();
    test_error_history_telemetry_is_independent_of_debug();
    test_nasa_custom_override_receives_its_message_not_fixed_current();
    test_unaccepted_nasa_rx_is_silent();
    test_accepted_nasa_rx_is_passive();
    test_diagnostics_preserve_power_driven_display_read();
    test_expired_display_read_does_not_consume_error_history();
    test_repeated_rx_ignores_legacy_deduplication();
    test_non_nasa_rx_is_excluded();
    std::cout << "NASA diagnostics tests passed\n";
}
