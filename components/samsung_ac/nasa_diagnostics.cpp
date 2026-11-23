#include "nasa_diagnostics.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>

#include "esphome/core/log.h"
#include "protocol_nasa.h"

namespace esphome
{
    namespace samsung_ac
    {
        namespace
        {
            constexpr char DIAGNOSTIC_TAG[] = "samsung_ac.diagnostics";
            constexpr std::size_t STRUCTURE_CHUNK_SIZE = 32;

            enum class Conversion
            {
                Raw,
                Enum,
                Unsigned,
                SignedTenths,
                UnsignedTenths,
                LowByte,
                HighWordTenths,
                OverloadCountStatus,
                WhToKwh,
            };

            enum class Confidence
            {
                Documented,
                Provisional,
            };

            struct EnumValue
            {
                uint32_t raw;
                const char *name;
            };

            struct Descriptor
            {
                uint16_t code;
                const char *name;
                Conversion conversion = Conversion::Raw;
                const char *unit = "na";
                Confidence confidence = Confidence::Documented;
                std::optional<uint32_t> marker = std::nullopt;
                const EnumValue *values = nullptr;
                std::size_t value_count = 0;
            };

            template<std::size_t N>
            constexpr Descriptor enum_descriptor(uint16_t code, const char *name, const EnumValue (&values)[N])
            {
                return {code, name, Conversion::Enum, "na", Confidence::Documented, std::nullopt, values, N};
            }

            constexpr Descriptor raw_descriptor(uint16_t code, const char *name, Confidence confidence,
                                                std::optional<uint32_t> marker = std::nullopt)
            {
                return {code, name, Conversion::Raw, "na", confidence, marker};
            }

            constexpr EnumValue OFF_ON[] = {{0, "OFF"}, {1, "ON"}};
            constexpr EnumValue INDOOR_POWER[] = {{0, "OFF"}, {1, "ON"}, {2, "ON"}};
            constexpr EnumValue MODE_SELECTED[] = {
                {0, "AUTO"}, {1, "COOL"}, {2, "DRY"}, {3, "FAN"}, {4, "HEAT"}, {21, "COOL_STORAGE"}, {24, "HOT_WATER"},
            };
            constexpr EnumValue MODE_REPORTED[] = {
                // S-NET distinguishes Unknown here from Auto in the selected mode.
                {0, "UNKNOWN"}, {1, "COOL"}, {2, "DRY"}, {3, "FAN"}, {4, "HEAT"}, {11, "AUTO_COOL"},
                {12, "AUTO_DRY"}, {13, "AUTO_FAN"}, {14, "AUTO_HEAT"}, {21, "COOL_STORAGE"}, {24, "HOT_WATER"}, {255, "NULL"},
            };
            constexpr EnumValue FAN_SELECTED[] = {{0, "AUTO"}, {1, "LOW"}, {2, "MID"}, {3, "HIGH"}, {4, "TURBO"}};
            constexpr EnumValue FAN_REPORTED[] = {
                {1, "LOW"}, {2, "MID"}, {3, "HIGH"}, {4, "TURBO"},
                {10, "AUTO_LOW"}, {11, "AUTO_MID"}, {12, "AUTO_HIGH"},
                {13, "UL"}, {14, "LL"}, {15, "HH"}, {16, "SPEED"},
                {17, "NATURAL_LOW"}, {18, "NATURAL_MID"}, {19, "NATURAL_HIGH"}, {254, "OFF"},
            };
            constexpr EnumValue THERMAL_DEMAND[] = {{0, "NO_DEMAND"}, {1, "DEMAND"}};
            constexpr EnumValue ROOM_SENSOR_SELECTED[] = {{0, "INDOOR_UNIT"}, {1, "WIRED_REMOTE"}};
            constexpr EnumValue QUIET_MODE_AVAILABLE[] = {{1, "SUPPORTED"}};
            constexpr EnumValue INDOOR_PRESET[] = {
                {0, "NONE"}, {1, "SLEEP"}, {2, "QUIET"}, {3, "FAST"}, {6, "LONG_REACH"}, {7, "ECO"}, {9, "WINDFREE"},
            };
            constexpr EnumValue SERVICE_OPERATION[] = {{2, "HEAT_TEST"}, {3, "PUMP_OUT"}, {13, "COOL_TEST"}, {14, "PUMP_DOWN"}};
            constexpr EnumValue OPERATION_STATE[] = {
                {0, "STOP"}, {1, "SAFETY"}, {2, "NORMAL"}, {3, "BALANCE"}, {4, "RECOVERY"}, {5, "DEFROST"},
                {6, "COMPRESSOR_DOWN"}, {7, "INHIBITED"}, {8, "LINE_TEST"}, {9, "PCB_TEST"}, {10, "TEST"}, {11, "CHARGE"},
                {12, "PUMP_DOWN"}, {13, "PUMP_OUT"}, {14, "VACUUM"}, {15, "CALORIMETRIC_TEST"}, {16, "PUMP_DOWN_END"},
                {17, "SECONDARY_STOP"}, {18, "PIPE_CHECK"}, {19, "REFRIGERANT_CHECK"}, {20, "FPT_TEST"}, {21, "NONSTOP_HEAT_COOL_CHANGE"},
                {22, "AUTO_INSPECTION"}, {23, "ELECTRICAL_DISCHARGE"}, {24, "SPLIT_DEFROST"}, {25, "INVERTER_CHECK"},
                {26, "NONSTOP_DEFROST"}, {27, "REMOTE_TEST"}, {28, "NOMINAL_TEST"}, {29, "PC_TEST"}, {30, "THERMO_OFF_PUMP_DOWN"},
                {31, "THREE_PHASE_TEST"}, {32, "SMART_INSTALL_TEST"}, {33, "DEFROST_PERFORMANCE_TEST"}, {34, "INVERTER_FAN_PBA_CHECK"},
                {35, "AUTO_PIPE_PAIRING"}, {36, "AUTO_CHARGE"},
            };
            constexpr EnumValue OUTDOOR_THERMAL_MODE[] = {{0, "UNDEFINED"}, {1, "COOL"}, {2, "HEAT"}, {3, "COOL_DOMINANT"}, {4, "HEAT_DOMINANT"}};
            constexpr EnumValue TEST_COMPLETION[] = {{0, "NOT_COMPLETED"}, {1, "COMPLETED"}};
            constexpr EnumValue DEICE_STEP[] = {{1, "STEP_1"}, {2, "STEP_2"}, {3, "STEP_3"}, {7, "FINAL_STEP"}, {255, "NO_DEFROST"}};
            constexpr EnumValue OUTDOOR_ERROR[] = {{0, "NO_ERROR"}};

            constexpr Descriptor DICTIONARY[] = {
                // Identification and configuration
                {0x0023, "standby_mode"},
                raw_descriptor(0x0202, "general_error_code", Confidence::Documented, 0xFFFF),
                {0x0207, "connected_indoor_count", Conversion::Unsigned, "count"},
                {0x0402, "remote_controller_address", Conversion::LowByte},
                {0x0409, "ad_install_level_all"},
                {0x040A, "ad_install_level_operation_power"},
                {0x040B, "ad_install_level_operation_mode"},
                {0x040C, "ad_install_level_fan_mode"},
                {0x040D, "ad_install_level_fan_direction"},
                {0x040E, "ad_install_level_temp_target"},
                {0x0410, "ad_install_level_operation_mode_only"},
                {0x0411, "ad_install_level_cool_mode_upper", Conversion::HighWordTenths, "C"},
                {0x0412, "ad_install_level_cool_mode_lower", Conversion::HighWordTenths, "C"},
                {0x0413, "ad_install_level_heat_mode_upper", Conversion::HighWordTenths, "C"},
                {0x0414, "ad_install_level_heat_mode_lower", Conversion::HighWordTenths, "C"},
                {0x0415, "ad_install_level_contact_control"},
                {0x0416, "ad_install_level_key_operation_input"},

                // Indoor unit
                enum_descriptor(0x4000, "indoor_power", INDOOR_POWER),
                enum_descriptor(0x4001, "indoor_mode_selected", MODE_SELECTED),
                enum_descriptor(0x4002, "indoor_mode_reported", MODE_REPORTED),
                enum_descriptor(0x4006, "indoor_fan_selected", FAN_SELECTED),
                enum_descriptor(0x4007, "indoor_fan_reported", FAN_REPORTED),
                enum_descriptor(0x4011, "vertical_swing", OFF_ON),
                {0x4012, "partial_vertical_swing"},
                {0x401B, "central_control_enabled"},
                raw_descriptor(0x4023, "ionizer_available", Confidence::Provisional),
                {0x4024, "filter_warning_configuration"},
                enum_descriptor(0x4027, "filter_warning", OFF_ON),
                enum_descriptor(0x4028, "thermal_demand", THERMAL_DEMAND),
                enum_descriptor(0x402E, "indoor_defrost", OFF_ON),
                raw_descriptor(0x4038, "humidity_unverified", Confidence::Provisional),
                {0x403E, "use_mds"},
                {0x403F, "control_mds"},
                enum_descriptor(0x4043, "ionizer_control", OFF_ON),
                enum_descriptor(0x4045, "quiet_mode_available", QUIET_MODE_AVAILABLE),
                enum_descriptor(0x4046, "quiet_mode", OFF_ON),
                enum_descriptor(0x4060, "indoor_preset", INDOOR_PRESET),
                enum_descriptor(0x4076, "indoor_room_sensor_selected", ROOM_SENSOR_SELECTED),
                enum_descriptor(0x407E, "horizontal_swing", OFF_ON),
                {0x40BC, "in_state_key_tag"},
                {0x40BD, "in_empty_room_control_used"},
                {0x40D5, "in_enter_room_control_used"},
                {0x4201, "indoor_setpoint", Conversion::SignedTenths, "C"},
                raw_descriptor(0x4202, "unconfirmed_0x4202", Confidence::Provisional),
                {0x4203, "indoor_room_temperature", Conversion::SignedTenths, "C"},
                {0x4204, "indoor_temperature_0x4204", Conversion::SignedTenths, "C", Confidence::Provisional},
                {0x4205, "indoor_evaporator_inlet_temperature", Conversion::SignedTenths, "C"},
                {0x4206, "indoor_evaporator_outlet_temperature", Conversion::SignedTenths, "C"},
                raw_descriptor(0x4211, "indoor_capacity_request", Confidence::Provisional),
                raw_descriptor(0x4212, "indoor_capacity_absolute", Confidence::Provisional),
                {0x4217, "indoor_eev_1_position", Conversion::Unsigned},
                {0x4218, "indoor_eev_2_position", Conversion::Unsigned},
                {0x4229, "indoor_model_information"},
                raw_descriptor(0x42D1, "particle_sensor_pm10", Confidence::Documented, 0xFFFF),
                raw_descriptor(0x42D2, "particle_sensor_pm2_5", Confidence::Documented, 0xFFFF),
                raw_descriptor(0x42D3, "particle_sensor_pm1", Confidence::Documented, 0xFFFF),
                {0x440F, "error_inout"},
                {0x4418, "in_empty_room_control_data"},
                {0x441B, "in_enter_room_control_data"},
                {0x441F, "in_eto_cool_control_data"},
                {0x4420, "in_eto_heat_control_data"},
                raw_descriptor(0x4423, "unconfirmed_0x4423", Confidence::Provisional),
                raw_descriptor(0x4424, "unconfirmed_0x4424", Confidence::Provisional),

                // Outdoor unit
                enum_descriptor(0x8000, "outdoor_service_operation", SERVICE_OPERATION),
                enum_descriptor(0x8001, "outdoor_operation_state", OPERATION_STATE),
                enum_descriptor(0x8003, "outdoor_thermal_mode", OUTDOOR_THERMAL_MODE),
                enum_descriptor(0x8010, "compressor_1_running", OFF_ON),
                enum_descriptor(0x8017, "hot_gas_valve_1_drive", OFF_ON),
                enum_descriptor(0x801A, "four_way_valve_drive", OFF_ON),
                raw_descriptor(0x8020, "outdoor_eev_state", Confidence::Provisional),
                enum_descriptor(0x8046, "outdoor_test_completion", TEST_COMPLETION),
                raw_descriptor(0x8047, "outdoor_service_mode", Confidence::Provisional),
                enum_descriptor(0x8061, "indoor_deice_step", DEICE_STEP),
                raw_descriptor(0x8066, "outdoor_mode_limit", Confidence::Provisional),
                raw_descriptor(0x8081, "outdoor_external_command", Confidence::Provisional),
                {0x808E, "refrigerant_check_stage"},
                raw_descriptor(0x809D, "cool_only_model", Confidence::Provisional),
                raw_descriptor(0x80A7, "outdoor_dred_level", Confidence::Provisional),
                raw_descriptor(0x80AF, "base_heater_state", Confidence::Provisional),
                raw_descriptor(0x80B2, "outdoor_channel_switch", Confidence::Provisional),
                raw_descriptor(0x8200, "outdoor_mode_option", Confidence::Provisional),
                {0x8202, "compressor_count", Conversion::Unsigned, "count", Confidence::Documented, 0xFFFF},
                {0x8204, "outdoor_air_temperature", Conversion::SignedTenths, "C"},
                {0x8206, "outdoor_high_pressure", Conversion::SignedTenths, "kgf/cm2"},
                {0x8208, "outdoor_low_pressure", Conversion::SignedTenths, "kgf/cm2"},
                {0x820A, "compressor_1_discharge_temperature", Conversion::SignedTenths, "C"},
                {0x8217, "compressor_1_ct_current", Conversion::UnsignedTenths, "A"},
                {0x8218, "outdoor_heat_exchanger_outlet_temperature", Conversion::SignedTenths, "C"},
                {0x821A, "compressor_suction_temperature", Conversion::SignedTenths, "C", Confidence::Documented, 0xFFFF},
                {0x821C, "outdoor_liquid_line_temperature", Conversion::SignedTenths, "C"},
                {0x8223, "compressor_discharge_target_temperature", Conversion::SignedTenths, "C"},
                {0x8229, "outdoor_eev_1_position", Conversion::Unsigned},
                {0x822A, "outdoor_eev_2_position", Conversion::Unsigned},
                {0x822B, "outdoor_eev_3_position", Conversion::Unsigned},
                {0x822C, "outdoor_eev_4_position", Conversion::Unsigned},
                {0x822D, "outdoor_eev_5_position", Conversion::Unsigned},
                raw_descriptor(0x8233, "outdoor_operating_capacity", Confidence::Provisional),
                enum_descriptor(0x8235, "outdoor_error_code", OUTDOOR_ERROR),
                {0x8236, "compressor_1_frequency_requested", Conversion::Unsigned, "Hz"},
                {0x8237, "compressor_1_frequency_target", Conversion::Unsigned, "Hz"},
                {0x8238, "compressor_1_frequency_actual", Conversion::Unsigned, "Hz"},
                raw_descriptor(0x823B, "inverter_dc_link_voltage", Confidence::Provisional),
                {0x823D, "outdoor_fan_1_speed", Conversion::Unsigned, "rpm"},
                // The logical channel does not establish that a second physical fan exists.
                {0x823E, "outdoor_fan_channel_2_speed", Conversion::Unsigned, "rpm", Confidence::Provisional},
                raw_descriptor(0x823F, "outdoor_control_prime_unit", Confidence::Provisional),
                raw_descriptor(0x8247, "outdoor_defrost_step", Confidence::Provisional),
                raw_descriptor(0x8248, "outdoor_safety_start", Confidence::Provisional, 0xFFFF),
                raw_descriptor(0x824F, "refrigerant_amount_unverified", Confidence::Provisional, 0xFFFF),
                {0x8254, "inverter_ipm_1_temperature", Conversion::SignedTenths, "C"},
                {0x825F, "pipe_inlet_1_temperature", Conversion::SignedTenths, "C"},
                {0x8260, "pipe_inlet_2_temperature", Conversion::SignedTenths, "C"},
                {0x8261, "pipe_inlet_3_temperature", Conversion::SignedTenths, "C"},
                {0x8262, "pipe_inlet_4_temperature", Conversion::SignedTenths, "C", Confidence::Documented, 0xFE0C},
                {0x8263, "pipe_inlet_5_temperature", Conversion::SignedTenths, "C", Confidence::Documented, 0xFE0C},
                {0x8264, "pipe_outlet_1_temperature", Conversion::SignedTenths, "C"},
                {0x8265, "pipe_outlet_2_temperature", Conversion::SignedTenths, "C"},
                {0x8266, "pipe_outlet_3_temperature", Conversion::SignedTenths, "C"},
                {0x8267, "pipe_outlet_4_temperature", Conversion::SignedTenths, "C", Confidence::Documented, 0xFE0C},
                {0x8268, "pipe_outlet_5_temperature", Conversion::SignedTenths, "C", Confidence::Documented, 0xFE0C},
                {0x8274, "compressor_2_frequency_requested", Conversion::Unsigned, "Hz", Confidence::Documented, 0xFFFF},
                {0x8275, "compressor_2_frequency_target", Conversion::Unsigned, "Hz", Confidence::Documented, 0xFFFF},
                {0x8280, "compressor_1_top_temperature", Conversion::SignedTenths, "C"},
                raw_descriptor(0x8287, "outdoor_capacity_code", Confidence::Provisional),
                {0x82BC, "outdoor_project_code"},
                raw_descriptor(0x82DB, "phase_current_unverified", Confidence::Provisional),
                raw_descriptor(0x82E3, "outdoor_product_capacity_code", Confidence::Provisional),
                {0x82F5, "outdoor_overload_detection", Conversion::OverloadCountStatus},
                {0x8411, "outdoor_electrical_power", Conversion::Unsigned, "W"},
                // The reported aggregate does not establish an averaging interval.
                {0x8413, "outdoor_electrical_power_aggregate", Conversion::Unsigned, "W"},
                {0x8414, "outdoor_electrical_energy_total", Conversion::WhToKwh, "kWh"},
            };

            constexpr bool dictionary_is_sorted()
            {
                for (std::size_t i = 1; i < std::size(DICTIONARY); ++i)
                {
                    if (DICTIONARY[i - 1].code >= DICTIONARY[i].code)
                        return false;
                }
                return true;
            }

            static_assert(dictionary_is_sorted(), "NASA diagnostic codes must be unique and sorted");

            const Descriptor *find_descriptor(uint16_t code)
            {
                const auto end = std::end(DICTIONARY);
                const auto entry = std::lower_bound(std::begin(DICTIONARY), end, code,
                                                    [](const Descriptor &descriptor, uint16_t key) { return descriptor.code < key; });
                return entry != end && entry->code == code ? entry : nullptr;
            }

            void append_hex(std::string &line, uint32_t value, unsigned digits, const char *alphabet = "0123456789ABCDEF")
            {
                for (unsigned i = digits; i > 0; --i)
                    line += alphabet[(value >> (4 * (i - 1))) & 0xF];
            }

            void append_address(std::string &line, const Address &address)
            {
                constexpr char LOWER_HEX[] = "0123456789abcdef";
                append_hex(line, static_cast<uint8_t>(address.klass), 2, LOWER_HEX);
                line += '.';
                append_hex(line, address.channel, 2, LOWER_HEX);
                line += '.';
                append_hex(line, address.address, 2, LOWER_HEX);
            }

            const char *data_type_name(DataType type)
            {
                switch (type)
                {
                case DataType::Read: return "read";
                case DataType::Write: return "write";
                case DataType::Request: return "request";
                case DataType::Notification: return "notification";
                case DataType::Response: return "response";
                case DataType::Ack: return "ack";
                case DataType::Nack: return "nack";
                default: return "undefined";
                }
            }

            std::string packet_prefix(const Packet &packet)
            {
                std::string line = "NASA_DIAG src=";
                append_address(line, packet.sa);
                line += " dst=";
                append_address(line, packet.da);
                line += " dt=";
                line += data_type_name(packet.command.dataType);
                line += " seq=";
                line += std::to_string(packet.command.packetNumber);
                return line;
            }

            void append_field_prefix(std::string &line, std::size_t index, uint16_t code, const char *type)
            {
                line += " index=";
                line += std::to_string(index);
                line += " code=0x";
                append_hex(line, code, 4);
                line += " type=";
                line += type;
            }

            std::string fixed_decimal(uint32_t value, uint32_t divisor, std::size_t places)
            {
                const auto fraction = std::to_string(value % divisor);
                auto result = std::to_string(value / divisor);
                result += '.';
                result.append(places - fraction.size(), '0');
                result += fraction;
                return result;
            }

            struct DecodedValue
            {
                std::string value = "na";
                const char *unit = "na";
                const char *status = "decoded";
            };

            DecodedValue decode_scalar(const Descriptor *descriptor, MessageSetType type, uint32_t raw)
            {
                if (!descriptor)
                    return {"na", "na", "unknown_code"};

                // Wire type and conversion are independent: 0x8235 is a Variable with enum values.
                const auto expected_type = static_cast<MessageSetType>((descriptor->code & 0x0600) >> 9);
                if (type != expected_type)
                    return {"na", "na", "type_mismatch"};
                if (descriptor->marker == raw)
                    return {"na", "na", "unverified_marker"};

                switch (descriptor->conversion)
                {
                case Conversion::Raw:
                    return {"na", "na", "raw_only"};
                case Conversion::Enum:
                    for (std::size_t i = 0; i < descriptor->value_count; ++i)
                    {
                        if (descriptor->values[i].raw == raw)
                            return {descriptor->values[i].name, descriptor->unit};
                    }
                    return {"na", "na", "unmapped_enum"};
                case Conversion::Unsigned:
                    return {std::to_string(raw), descriptor->unit};
                case Conversion::SignedTenths:
                {
                    if (raw >= 0x8000)
                        return {"-" + fixed_decimal(0x10000 - raw, 10, 1), descriptor->unit};
                    return {fixed_decimal(raw, 10, 1), descriptor->unit};
                }
                case Conversion::UnsignedTenths:
                    return {fixed_decimal(raw, 10, 1), descriptor->unit};
                case Conversion::LowByte:
                    return {std::to_string(raw & 0xFF), descriptor->unit};
                case Conversion::HighWordTenths:
                    return {fixed_decimal(raw >> 16, 10, 1), descriptor->unit};
                case Conversion::OverloadCountStatus:
                    return {"count:" + std::to_string(raw >> 8) + ",status:" + std::to_string(raw & 0xFF), descriptor->unit};
                case Conversion::WhToKwh:
                    return {fixed_decimal(raw, 1000, 3), descriptor->unit};
                }
                return {"na", "na", "raw_only"};
            }

            std::string format_scalar(std::string line, DataType data_type, const MessageSet &message, std::size_t index)
            {
                const char *type;
                unsigned hex_digits;
                uint32_t mask;
                switch (message.type)
                {
                case Enum:         type = "enum";          hex_digits = 2; mask = 0xFF;       break;
                case Variable:     type = "variable";      hex_digits = 4; mask = 0xFFFF;     break;
                case LongVariable: type = "long_variable"; hex_digits = 8; mask = 0xFFFFFFFF; break;
                default: return {};
                }

                const auto code = static_cast<uint16_t>(message.messageNumber);
                const uint32_t raw = static_cast<uint32_t>(message.value) & mask;
                const auto *descriptor = find_descriptor(code);
                const auto decoded = data_type == DataType::Read
                    ? DecodedValue{"na", "na", "read_request"}
                    : decode_scalar(descriptor, message.type, raw);

                append_field_prefix(line, index, code, type);
                line += " name=";
                if (descriptor)
                    line += descriptor->name;
                else
                {
                    line += "unknown_0x";
                    append_hex(line, code, 4);
                }
                line += " raw=";
                line += std::to_string(raw);
                line += " raw_hex=0x";
                append_hex(line, raw, hex_digits);
                line += " value=";
                line += decoded.value;
                line += " unit=";
                line += decoded.unit;
                line += " status=";
                line += decoded.status;
                line += " confidence=";
                line += !descriptor ? "unknown" :
                    (descriptor->confidence == Confidence::Documented ? "documented" : "provisional");
                return line;
            }

            void log_line(const std::string &line)
            {
                ESP_LOGD(DIAGNOSTIC_TAG, "%s", line.c_str());
            }

            void log_structure(std::string line, const MessageSet &message, std::size_t index, std::size_t field_count)
            {
                append_field_prefix(line, index, static_cast<uint16_t>(message.messageNumber), "structure");

                // The parser can leave the union inactive when a structure is not the sole field.
                if (field_count != 1 || message.size < 2 || message.size > sizeof(Buffer::data) + 2)
                {
                    log_line(line + " status=invalid_structure");
                    return;
                }
                const std::size_t length = message.size - 2;
                if (message.structure.size != length)
                {
                    log_line(line + " status=invalid_structure");
                    return;
                }

                line += " len=";
                line += std::to_string(length);
                if (length == 0)
                {
                    log_line(line + " offset=0 hex=na");
                    return;
                }

                for (std::size_t offset = 0; offset < length; offset += STRUCTURE_CHUNK_SIZE)
                {
                    auto fragment = line + " offset=";
                    fragment += std::to_string(offset);
                    fragment += " hex=";
                    const auto end = std::min(length, offset + STRUCTURE_CHUNK_SIZE);
                    for (std::size_t i = offset; i < end; ++i)
                        append_hex(fragment, message.structure.data[i], 2);
                    log_line(fragment);
                }
            }
        } // namespace

        std::string format_nasa_diagnostic_scalar(const Packet &packet, const MessageSet &message, std::size_t index)
        {
            if (message.type == Structure)
                return {};
            return format_scalar(packet_prefix(packet), packet.command.dataType, message, index);
        }

        void log_nasa_diagnostics(const Packet &packet)
        {
            const auto prefix = packet_prefix(packet);
            if (packet.messages.empty())
            {
                log_line(prefix + " fields=0");
                return;
            }

            for (std::size_t index = 0; index < packet.messages.size(); ++index)
            {
                const auto &message = packet.messages[index];
                if (message.type == Structure)
                    log_structure(prefix, message, index, packet.messages.size());
                else
                    log_line(format_scalar(prefix, packet.command.dataType, message, index));
            }
        }
    } // namespace samsung_ac
} // namespace esphome
