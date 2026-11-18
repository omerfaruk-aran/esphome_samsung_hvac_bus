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
            {0x8238, Variable, 30, "name=compressor_1_frequency_actual raw=30 raw_hex=0x001E value=30 unit=Hz status=decoded confidence=documented"},
            {0x4204, Variable, -1, "name=indoor_room_temperature_modified raw=65535 raw_hex=0xFFFF value=-0.1 unit=C status=decoded confidence=provisional"},
            {0x4205, Variable, 0xFE0C, "name=indoor_evaporator_inlet_temperature raw=65036 raw_hex=0xFE0C value=-50.0 unit=C status=decoded confidence=documented"},
            {0x8217, Variable, 31, "name=compressor_1_ct_current raw=31 raw_hex=0x001F value=3.1 unit=A status=decoded confidence=documented"},
            {0x0402, LongVariable, 0x12345678, "name=remote_controller_address raw=305419896 raw_hex=0x12345678 value=120 unit=na status=decoded confidence=documented"},
            {0x0411, LongVariable, 0x00F90011, "name=ad_install_level_cool_mode_upper raw=16318481 raw_hex=0x00F90011 value=24.9 unit=C status=decoded confidence=documented"},
            {0x823E, Variable, 810, "name=outdoor_fan_channel_2_speed raw=810 raw_hex=0x032A value=810 unit=rpm status=decoded confidence=provisional"},
            {0x8413, LongVariable, 684, "name=outdoor_electrical_power_aggregate raw=684 raw_hex=0x000002AC value=684 unit=W status=decoded confidence=documented"},
            {0x8414, LongVariable, -1, "name=outdoor_electrical_energy_total raw=4294967295 raw_hex=0xFFFFFFFF value=4294967.295 unit=kWh status=decoded confidence=documented"},
            {0x8413, LongVariable, -2147483647L - 1, "name=outdoor_electrical_power_aggregate raw=2147483648 raw_hex=0x80000000 value=2147483648 unit=W status=decoded confidence=documented"},
            {0x823B, Variable, 151, "name=inverter_dc_link_voltage raw=151 raw_hex=0x0097 value=na unit=na status=raw_only confidence=provisional"},
            {0x4211, Variable, 30, "name=indoor_capacity_request raw=30 raw_hex=0x001E value=na unit=na status=raw_only confidence=provisional"},
            {0x822A, Variable, 158, "name=outdoor_eev_2_position raw=158 raw_hex=0x009E value=na unit=na status=raw_only confidence=documented"},
            {0x4038, Enum, 0, "name=humidity_unverified raw=0 raw_hex=0x00 value=na unit=na status=raw_only confidence=provisional"},
            {0x421C, Variable, 971, "name=unknown_0x421C raw=971 raw_hex=0x03CB value=na unit=na status=unknown_code confidence=unknown"},
            {0x429D, Variable, 965, "name=unknown_0x429D raw=965 raw_hex=0x03C5 value=na unit=na status=unknown_code confidence=unknown"},
            {0x4016, Enum, 1, "name=unknown_0x4016 raw=1 raw_hex=0x01 value=na unit=na status=unknown_code confidence=unknown"},
            {0x4002, Enum, 255, "name=indoor_mode_reported raw=255 raw_hex=0xFF value=NULL unit=na status=decoded confidence=documented"},
            {0x4007, Enum, 510, "name=indoor_fan_reported raw=254 raw_hex=0xFE value=OFF unit=na status=decoded confidence=documented"},
            {0x8061, Enum, 255, "name=indoor_deice_step raw=255 raw_hex=0xFF value=NO_DEFROST unit=na status=decoded confidence=documented"},
            {0x4001, Enum, 14, "name=indoor_mode_selected raw=14 raw_hex=0x0E value=na unit=na status=unmapped_enum confidence=documented"},
            {0x4007, Enum, 0, "name=indoor_fan_reported raw=0 raw_hex=0x00 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x4045, Enum, 0, "name=quiet_mode_available raw=0 raw_hex=0x00 value=na unit=na status=unmapped_enum confidence=documented"},
            {0x8001, Enum, 33, "name=outdoor_operation_state raw=33 raw_hex=0x21 value=DEFROST_PERFORMANCE_TEST unit=na status=decoded confidence=documented"},
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

    // Two fields make dropped, duplicated, or reordered records visible within a reception.
    constexpr const char *INDOOR_NOTIFICATION = "320016200001b300ffc0140102420400e9421c03cb555734";
    const std::string ROOM_DIAGNOSTIC =
        "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=0 code=0x4204 type=variable "
        "name=indoor_room_temperature_modified raw=233 raw_hex=0x00E9 value=23.3 unit=C status=decoded confidence=provisional";
    const std::string UNKNOWN_DIAGNOSTIC =
        "NASA_DIAG src=20.00.01 dst=b3.00.ff dt=notification seq=1 index=1 code=0x421C type=variable "
        "name=unknown_0x421C raw=971 raw_hex=0x03CB value=na unit=na status=unknown_code confidence=unknown";

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

    struct RxTrace
    {
        std::vector<std::string> diagnostics;
        std::vector<Callback> callbacks;
        std::vector<std::string> transmissions;
        uint8_t display_sequence;
    };

    RxTrace run_nasa_rx(bool diagnostics_enabled)
    {
        debug_log_diagnostics = diagnostics_enabled;
        protocol_processing = ProtocolProcessing::NASA;
        RxTarget target;
        auto *protocol = get_protocol("20.00.02");
        protocol->enable_display_light("20.00.02");
        protocol->refresh_display_light("20.00.02");
        protocol->protocol_update(&target);
        assert(target.all_publish_data.size() == 1);
        auto bytes = hex_to_bytes(target.all_publish_data.front());
        Packet display_reply;
        assert(display_reply.decode(bytes).type == DecodeResultType::Processed);
        assert(display_reply.command.dataType == DataType::Read);
        assert(display_reply.messages.size() == 1);
        assert(display_reply.messages[0].messageNumber == MessageNumber::ENUM_in_display_light);
        std::swap(display_reply.sa, display_reply.da);
        display_reply.command.dataType = DataType::Response;
        display_reply.messages[0].value = 1;
        const auto display_sequence = display_reply.command.packetNumber;
        target.all_publish_data.clear(); // The startup Read sets up the Response; trace only TX caused by RX.

        auto lines = capture_diagnostics([&]()
        {
            test_process_data(INDOOR_NOTIFICATION, target);
            test_process_data("320012200000b000ffc0140201420400f44b4d34", target); // Same code, different addresses.
            test_process_data("32000e20000180ff00c016030096ac34", target); // Empty ACK.
            test_process_data("32000e20000180ff00c0170400380b34", target); // Empty NACK.
            test_process_data("320012500000200001c01105014201000075ce34", target); // Read.
            test_process_data("32001280ff00200002c013f201420101186e5434", target); // Existing NASA Request fixture.
            test_process_data("320012500000200001c0120701420100dc2cfd34", target); // Write.
            // Explicit big-endian LongVariable RX bytes: 0x8414, 0x000F6868 Wh.
            test_process_data("320014100000b000ffc01406018414000f6868692934", target);
            test_process_data("320013200001500000c0150801460a1234ab259e34", target); // Single structure Response.
            test_process_data(bytes_to_hex(display_reply.encode()), target);
            protocol->protocol_update(&target);
        });
        debug_log_diagnostics = false;
        return {std::move(lines), std::move(target.callbacks), std::move(target.all_publish_data), display_sequence};
    }

    void test_accepted_nasa_rx_is_passive()
    {
        assert(!debug_log_messages && !debug_log_undefined_messages && !debug_log_raw_bytes);
        const auto disabled = run_nasa_rx(false);
        const auto enabled = run_nasa_rx(true);
        expect_records(disabled.diagnostics, {});
        const std::vector<Callback> callbacks = {
            {"display_unconfirmed", "20.00.02"},
            {"address", "20.00.01"}, {"custom", "20.00.01", 0x4204, 233.0f}, {"room", "20.00.01", 0, 23.3f},
            {"custom", "20.00.01", 0x421C, 971.0f},
            {"address", "20.00.00"}, {"custom", "20.00.00", 0x4204, 244.0f}, {"room", "20.00.00", 0, 24.4f},
            {"address", "20.00.01"}, {"ack", {}, 3}, {"address", "20.00.01"},
            {"address", "50.00.00"}, {"address", "80.ff.00"}, {"address", "50.00.00"},
            {"address", "10.00.00"}, {"custom", "10.00.00", 0x8414, 1009768.0f}, {"energy", "10.00.00", 0, 1009768.0f},
            {"address", "20.00.01"}, {"address", "20.00.02"}, {"display_on", "20.00.02"},
        };
        assert(disabled.callbacks == callbacks);
        assert(enabled.callbacks == disabled.callbacks);
        assert(disabled.transmissions.empty());
        assert(enabled.transmissions == disabled.transmissions);
        expect_records(enabled.diagnostics, {
            ROOM_DIAGNOSTIC,
            UNKNOWN_DIAGNOSTIC,
            "NASA_DIAG src=20.00.00 dst=b0.00.ff dt=notification seq=2 index=0 code=0x4204 type=variable "
            "name=indoor_room_temperature_modified raw=244 raw_hex=0x00F4 value=24.4 unit=C status=decoded confidence=provisional",
            "NASA_DIAG src=20.00.01 dst=80.ff.00 dt=ack seq=3 fields=0",
            "NASA_DIAG src=20.00.01 dst=80.ff.00 dt=nack seq=4 fields=0",
            "NASA_DIAG src=50.00.00 dst=20.00.01 dt=read seq=5 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=0 raw_hex=0x0000 value=0.0 unit=C status=decoded confidence=documented",
            "NASA_DIAG src=80.ff.00 dst=20.00.02 dt=request seq=242 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=280 raw_hex=0x0118 value=28.0 unit=C status=decoded confidence=documented",
            "NASA_DIAG src=50.00.00 dst=20.00.01 dt=write seq=7 index=0 code=0x4201 type=variable "
            "name=indoor_setpoint raw=220 raw_hex=0x00DC value=22.0 unit=C status=decoded confidence=documented",
            "NASA_DIAG src=10.00.00 dst=b0.00.ff dt=notification seq=6 index=0 code=0x8414 type=long_variable "
            "name=outdoor_electrical_energy_total raw=1009768 raw_hex=0x000F6868 value=1009.768 unit=kWh status=decoded confidence=documented",
            "NASA_DIAG src=20.00.01 dst=50.00.00 dt=response seq=8 index=0 code=0x460A type=structure len=3 offset=0 hex=1234AB",
            "NASA_DIAG src=20.00.02 dst=80.ff.00 dt=response seq=" + std::to_string(enabled.display_sequence) +
            " index=0 code=0x4016 type=enum name=unknown_0x4016 raw=1 raw_hex=0x01 value=na unit=na status=unknown_code confidence=unknown",
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
    test_empty_packet_records();
    test_scalar_logging_keeps_order_and_duplicates();
    test_structure_fragments_are_lossless();
    test_invalid_structure_metadata_has_no_payload();
    test_accepted_nasa_rx_is_passive();
    test_repeated_rx_ignores_legacy_deduplication();
    test_non_nasa_rx_is_excluded();
    std::cout << "NASA diagnostics tests passed\n";
}
