#include <queue>
#include <map>
#include <cmath>
#include <string>
#include "esphome/core/hal.h"
#include "util.h"
#include "log.h"
#include "protocol_non_nasa.h"

std::map<std::string, esphome::samsung_ac::NonNasaCommand20> last_command20s_;

esphome::samsung_ac::NonNasaDataPacket nonpacket_;

namespace esphome
{
    namespace samsung_ac
    {
        static bool pending_keepalive_ = false;
        static uint32_t pending_keepalive_due_ms_ = 0;
        static uint32_t last_keepalive_sent_ms_ = 0;
        constexpr uint32_t KEEPALIVE_DELAY_MS = 30;
        constexpr uint32_t KEEPALIVE_MIN_INTERVAL_MS = 5000;

        // Track cumulative energy calculation per device address
        // Note: Energy tracker persists across device reconnections. This is intentional to maintain
        // cumulative energy across device restarts. The tracker is keyed by device address, so if
        // a device is removed and re-added with the same address, energy continues accumulating.
        // Note: Uses double for accumulated_energy_kwh to maintain precision during long-term accumulation,
        // matching NASA protocol approach. Converted to float only when publishing (API requirement).
        struct CumulativeEnergyTracker
        {
            double accumulated_energy_kwh = 0.0; // Accumulated energy in kWh (double for precision)
            uint32_t last_update_time_ms = 0;    // Last time power was updated (milliseconds)
            float last_power_w = 0.0f;           // Last power value in Watts
            bool has_previous_update = false;    // Track if we've had at least one update (handles millis()=0 edge case)
        };

        std::map<std::string, CumulativeEnergyTracker> cumulative_energy_trackers_;

        // Energy calculation constants
        // MIN_DELTA_MS: Minimum time delta between energy updates (100ms)
        //   - Skips calculations for very small intervals (negligible energy)
        //   - Reduces unnecessary CPU usage and improves precision
        // MAX_DELTA_MS: Maximum time delta between energy updates (1 hour)
        //   - Caps maximum delta to avoid huge increments from stale data
        //   - Prevents unrealistic energy calculations from long gaps
        constexpr uint32_t MIN_DELTA_MS = 100;     // Minimum 100ms between energy updates
        constexpr uint32_t MAX_DELTA_MS = 3600000; // Maximum 1 hour (3600000 ms) between updates

        // Helper function to update cumulative energy tracker
        // This function handles all edge cases: wraparound, first update, time delta validation, and energy calculation
        // Uses trapezoidal rule for energy calculation: Energy = Average_Power (W) × Time (hours)
        // Returns true if energy was calculated and tracker was updated, false otherwise
        static bool update_cumulative_energy_tracker(CumulativeEnergyTracker &tracker, float current_power_w, uint32_t now)
        {
            // Clamp power to non-negative: HVAC systems consume power (positive values)
            // Negative values would indicate measurement error or sensor issues
            if (current_power_w < 0.0f)
            {
                if (debug_log_messages)
                {
                    LOGW("Cmd8D: Negative power detected (%.2f W), clamping to 0", current_power_w);
                }
                current_power_w = 0.0f;
            }

            // Handle first update: just store values, don't calculate energy yet
            if (!tracker.has_previous_update)
            {
                tracker.last_power_w = current_power_w;
                tracker.last_update_time_ms = now;
                tracker.has_previous_update = true;
                return false; // No energy calculated on first update
            }

            // Calculate time delta, handling millis() wraparound (occurs every ~49.7 days)
            uint32_t delta_ms;
            if (now >= tracker.last_update_time_ms)
            {
                delta_ms = now - tracker.last_update_time_ms;
            }
            else
            {
                // Wraparound detected: calculate from last_update_time_ms to UINT32_MAX, then from 0 to now
                delta_ms = (UINT32_MAX - tracker.last_update_time_ms) + now + 1;
            }

            // Validate time delta: skip if too small (negligible energy) or too large (stale data)
            if (delta_ms < MIN_DELTA_MS)
            {
                return false; // Skip calculation for very small intervals
            }
            if (delta_ms > MAX_DELTA_MS)
            {
                if (debug_log_messages)
                {
                    LOGW("Cmd8D: Large time delta detected (%u ms, ~%.1f hours), capping to 1 hour", delta_ms, delta_ms / 3600000.0f);
                }
                delta_ms = MAX_DELTA_MS; // Cap to 1 hour
            }

            // Calculate energy using trapezoidal rule: Energy = Average_Power (W) × Time (hours)
            // Average power = (last_power + current_power) / 2
            // Time in hours = delta_ms / (1000 * 3600)
            // Energy in kWh = (Average_Power (W) × Time (hours)) / 1000
            double average_power_w = (static_cast<double>(tracker.last_power_w) + static_cast<double>(current_power_w)) / 2.0;
            double time_hours = static_cast<double>(delta_ms) / 3600000.0; // Convert ms to hours
            double energy_kwh = (average_power_w * time_hours) / 1000.0;   // Convert W×h to kWh

            // Accumulate energy
            tracker.accumulated_energy_kwh += energy_kwh;

            // Update tracker state
            tracker.last_power_w = current_power_w;
            tracker.last_update_time_ms = now;

            return true; // Energy was calculated and tracker was updated
        }
        std::list<NonNasaRequestQueueItem> nonnasa_requests;
        bool controller_registered = false;
        bool indoor_unit_awake = true;

        uint8_t build_checksum(std::vector<uint8_t> &data)
        {
            uint8_t sum = data[1];
            for (uint8_t i = 2; i < 12; i++)
            {
                sum = sum ^ data[i];
            }
            return sum;
        }

        std::string NonNasaCommand20::to_string()
        {
            std::string str;
            str += "target_temp:" + std::to_string(target_temp) + "; ";
            str += "room_temp:" + std::to_string(room_temp) + "; ";
            str += "pipe_in:" + std::to_string(pipe_in) + "; ";
            str += "pipe_out:" + std::to_string(pipe_out) + "; ";
            str += "power:" + std::to_string(power ? 1 : 0) + "; ";
            str += "wind_direction:" + std::to_string((uint8_t)wind_direction) + "; ";
            str += "fanspeed:" + std::to_string((uint8_t)fanspeed) + "; ";
            str += "mode:" + long_to_hex((uint8_t)mode);
            return str;
        }

        std::string NonNasaCommandC0::to_string()
        {
            std::string str;
            str += "ou_operation_mode:" + long_to_hex((uint8_t)outdoor_unit_operation_mode) + "; ";
            str += "ou_4way_valve:" + std::to_string(outdoor_unit_4_way_valve ? 1 : 0) + "; ";
            str += "ou_hot_gas_bypass:" + std::to_string(outdoor_unit_hot_gas_bypass ? 1 : 0) + "; ";
            str += "ou_compressor:" + std::to_string(outdoor_unit_compressor ? 1 : 0) + "; ";
            str += "ou_ac_fan:" + std::to_string(outdoor_unit_ac_fan ? 1 : 0) + "; ";
            str += "ou_outdoor_temp[°C]:" + std::to_string(outdoor_unit_outdoor_temp_c) + "; ";
            str += "ou_discharge_temp[°C]:" + std::to_string(outdoor_unit_discharge_temp_c) + "; ";
            str += "ou_condenser_mid_temp[°C]:" + std::to_string(outdoor_unit_condenser_mid_temp_c);
            return str;
        }

        std::string NonNasaCommandC1::to_string()
        {
            std::string str;
            str += "ou_sump_temp[°C]:" + std::to_string(outdoor_unit_sump_temp_c);
            return str;
        }

        std::string NonNasaCommand8D::to_string()
        {
            std::string str;
            str += "inverter_current[A]:" + std::to_string(inverter_current_a) + "; ";
            str += "inverter_voltage[V]:" + std::to_string(inverter_voltage_v) + "; ";
            str += "inverter_power[W]:" + std::to_string(inverter_power_w);
            return str;
        }

        std::string NonNasaCommandF0::to_string()
        {
            std::string str;
            str += "ou_freeze_protection:" + std::to_string(outdoor_unit_freeze_protection ? 1 : 0) + "; ";
            str += "ou_heating_overload:" + std::to_string(outdoor_unit_heating_overload ? 1 : 0) + "; ";
            str += "ou_defrost_control:" + std::to_string(outdoor_unit_defrost_control ? 1 : 0) + "; ";
            str += "ou_discharge_protection:" + std::to_string(outdoor_unit_discharge_protection ? 1 : 0) + "; ";
            str += "ou_current_control:" + std::to_string(outdoor_unit_current_control ? 1 : 0) + "; ";
            str += "inverter_order_frequency[Hz]:" + std::to_string(inverter_order_frequency_hz) + "; ";
            str += "inverter_target_frequency[Hz]:" + std::to_string(inverter_target_frequency_hz) + "; ";
            str += "inverter_current_frequency[Hz]:" + std::to_string(inverter_current_frequency_hz) + "; ";
            str += "ou_bldc_fan:" + std::to_string(outdoor_unit_bldc_fan ? 1 : 0) + "; ";
            str += "ou_error_code:" + long_to_hex((uint8_t)outdoor_unit_error_code);
            return str;
        }

        std::string NonNasaCommandF1::to_string()
        {
            std::string str;
            str += "Electronic Expansion Valves: ";
            str += "EEV_A:" + std::to_string(outdoor_unit_EEV_A) + "; ";
            str += "EEV_B:" + std::to_string(outdoor_unit_EEV_B) + "; ";
            str += "EEV_C:" + std::to_string(outdoor_unit_EEV_C) + "; ";
            str += "EEV_D:" + std::to_string(outdoor_unit_EEV_D);
            return str;
        }

        std::string NonNasaCommandF3::to_string()
        {
            std::string str;
            str += "inverter_max_frequency[Hz]:" + std::to_string(inverter_max_frequency_hz) + "; ";
            str += "inverter_total_capacity_requirement[kW]:" + std::to_string(inverter_total_capacity_requirement_kw) + "; ";
            str += "inverter_current[ADC]:" + std::to_string(inverter_current_a) + "; ";
            str += "inverter_voltage[VDC]:" + std::to_string(inverter_voltage_v) + "; ";
            str += "inverter_power[W]:" + std::to_string(inverter_power_w);
            return str;
        }

        std::string NonNasaDataPacket::to_string()
        {
            std::string str;
            str += "{";
            str += "src:" + src + ";";
            str += "dst:" + dst + ";";
            str += "cmd:" + long_to_hex((uint8_t)cmd) + ";";
            switch (cmd)
            {
            case NonNasaCommand::Cmd20:
            {
                str += "command20:{" + command20.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdC0:
            {
                str += "commandC0:{" + commandC0.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdC1:
            {
                str += "commandC1:{" + commandC1.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdC6:
            {
                str += "commandC6:{" + commandC6.to_string() + "}";
                break;
            }
            case NonNasaCommand::Cmd8D:
            {
                str += "command8D:{" + command8D.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdF0:
            {
                str += "commandF0:{" + commandF0.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdF1:
            {
                str += "commandF1:{" + commandF1.to_string() + "}";
                break;
            }
            case NonNasaCommand::CmdF3:
            {
                str += "commandF3:{" + commandF3.to_string() + "}";
                break;
            }
            default:
            {
                str += "raw:" + commandRaw.to_string();
                break;
            }
            }

            str += "}";
            return str;
        }

        DecodeResult NonNasaDataPacket::decode(std::vector<uint8_t> &data)
        {
            // Stream-safe: wait until we have at least 14 bytes
            if (data.size() < 14)
                return {DecodeResultType::Fill, 0};

            // Only examine the first 14 bytes (buffer may contain more)
            // Frame format: 0x32 .... 0x34 (14 bytes total)
            if (data[0] != 0x32)
                return {DecodeResultType::Discard, 1};

            if (data[13] != 0x34)
                return {DecodeResultType::Discard, 1};

            // Validate checksum against first 14 bytes
            uint8_t crc_expected = build_checksum(data); // uses bytes [1..11]
            uint8_t crc_actual = data[12];

            if (crc_actual != crc_expected)
            {
                if (debug_log_undefined_messages)
                    LOGW("NonNASA: invalid crc - got %d but should be %d: %s",
                         crc_actual, crc_expected,
                         bytes_to_hex(std::vector<uint8_t>(data.begin(), data.begin() + 14)).c_str());

                // resync gently (consume 1) - safer in streams
                return {DecodeResultType::Discard, 1};
            }

            // Decode using first 14 bytes
            src = long_to_hex(data[1]);
            dst = long_to_hex(data[2]);

            cmd = (NonNasaCommand)data[3];
            switch (cmd)
            {
            case NonNasaCommand::Cmd20:
                command20.target_temp = data[4] - 55;
                command20.room_temp = data[5] - 55;
                command20.pipe_in = data[6] - 55;
                command20.wind_direction = (NonNasaWindDirection)((data[7]) >> 3);
                command20.fanspeed = (NonNasaFanspeed)((data[7] & 0b00000111));
                command20.mode = (NonNasaMode)(data[8] & 0b00111111);
                command20.power = data[8] & 0b10000000;
                command20.pipe_out = data[11] - 55;

                if (command20.wind_direction == (NonNasaWindDirection)0)
                    command20.wind_direction = NonNasaWindDirection::Stop;

                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdC0:
                commandC0.outdoor_unit_operation_mode = data[4];
                commandC0.outdoor_unit_4_way_valve = data[6] & 0b10000000;
                commandC0.outdoor_unit_hot_gas_bypass = data[6] & 0b00100000;
                commandC0.outdoor_unit_compressor = data[6] & 0b00000100;
                commandC0.outdoor_unit_ac_fan = data[7] & 0b00000011;
                commandC0.outdoor_unit_outdoor_temp_c = data[8] - 55;
                commandC0.outdoor_unit_discharge_temp_c = data[10] - 55;
                commandC0.outdoor_unit_condenser_mid_temp_c = data[11] - 55;
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdC1:
                commandC1.outdoor_unit_sump_temp_c = data[8] - 55;
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdC6:
                commandC6.control_status = data[4];
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::Cmd8D:
                // Cmd8D from outdoor unit - contains power/energy data
                // Format: current raw value = data[8], voltage = data[10] * 2
                //
                // Power calculation consistency explanation:
                // The current sensor has a filter (multiply: 0.1) that will apply to the published current value.
                // To maintain the fundamental relationship: published_power = published_current × published_voltage,
                // we must apply the same 0.1 multiplier to the power calculation.
                //
                // Example: raw_current=100, raw_voltage=120 (raw)
                //   - Calculated current = 100 / 10 = 10A
                //   - Published current = 10 * 0.1 = 1A (after filter)
                //   - Calculated voltage = 120 * 2 = 240V
                //   - Published voltage = 240V (no filter)
                //   - Calculated power = (100 / 10) * 0.1 * (120 * 2) = 1 * 240 = 240W
                //   - Published power = 240W
                //   - Verification: 1A × 240V = 240W ✓
                //
                command8D.inverter_current_a = (float)data[8] / 10;                                              // Current in Amps (raw value / 10)
                command8D.inverter_voltage_v = (float)data[10] * 2;                                              // Voltage in Volts (raw value * 2)
                command8D.inverter_power_w = command8D.inverter_current_a * 0.1f * command8D.inverter_voltage_v; // Power in Watts
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdF0:
                commandF0.outdoor_unit_freeze_protection = data[4] & 0b10000000;
                commandF0.outdoor_unit_heating_overload = data[4] & 0b01000000;
                commandF0.outdoor_unit_defrost_control = data[4] & 0b00100000;
                commandF0.outdoor_unit_discharge_protection = data[4] & 0b00010000;
                commandF0.outdoor_unit_current_control = data[4] & 0b00001000;
                commandF0.inverter_order_frequency_hz = data[5];
                commandF0.inverter_target_frequency_hz = data[6];
                commandF0.inverter_current_frequency_hz = data[7];
                commandF0.outdoor_unit_bldc_fan = data[8] & 0b00000011;
                commandF0.outdoor_unit_error_code = data[10];
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdF1:
                commandF1.outdoor_unit_EEV_A = (data[4] * 256) + data[5];
                commandF1.outdoor_unit_EEV_B = (data[6] * 256) + data[7];
                commandF1.outdoor_unit_EEV_C = (data[8] * 256) + data[9];
                commandF1.outdoor_unit_EEV_D = (data[10] * 256) + data[11];
                return {DecodeResultType::Processed, 14};

            case NonNasaCommand::CmdF3:
                commandF3.inverter_max_frequency_hz = data[4];
                commandF3.inverter_total_capacity_requirement_kw = (float)data[5] / 10;
                commandF3.inverter_current_a = (float)data[8] / 10;
                commandF3.inverter_voltage_v = (float)data[9] * 2;
                commandF3.inverter_power_w = commandF3.inverter_current_a * 0.1f * commandF3.inverter_voltage_v;
                return {DecodeResultType::Processed, 14};

            default:
                commandRaw.length = 14 - 4 - 1;
                {
                    auto begin = data.begin() + 4;
                    std::copy(begin, begin + commandRaw.length, commandRaw.data);
                }
                return {DecodeResultType::Processed, 14};
            }
        }

        uint8_t encode_request_mode(NonNasaMode value)
        {
            switch (value)
            {
            case NonNasaMode::Auto:
                return 0;
            case NonNasaMode::Cool:
                return 1;
            case NonNasaMode::Dry:
                return 2;
            case NonNasaMode::Fan:
                return 3;
            case NonNasaMode::Heat:
                return 4;
                // NORMALVENT: 7
                // EXCHANGEVENT: 15
                // AIRFRESH: 23
                // SLEEP: 31
                // AUTOVENT: 79

            default:
                return 0; // Auto
            }
        }

        uint8_t encode_request_fanspeed(NonNasaFanspeed value)
        {
            switch (value)
            {
            case NonNasaFanspeed::Auto:
                return 0;
            case NonNasaFanspeed::Low:
                return 64;
            case NonNasaFanspeed::Medium:
                return 128;
            case NonNasaFanspeed::Fresh:
            case NonNasaFanspeed::High:
                return 160;
            default:
                return 0; // Auto
            }
        }

        NonNasaWindDirection swingmode_to_wind_direction(SwingMode swing)
        {
            switch (swing)
            {
            case SwingMode::Fix:
                return NonNasaWindDirection::Stop;
            case SwingMode::Vertical:
                return NonNasaWindDirection::Vertical;
            case SwingMode::Horizontal:
                return NonNasaWindDirection::Horizontal;
            case SwingMode::All:
                return NonNasaWindDirection::FourWay;
            default:
                return NonNasaWindDirection::Stop;
            }
        }

        uint8_t encode_request_wind_direction(NonNasaWindDirection wind_dir)
        {
            switch (wind_dir)
            {
            case NonNasaWindDirection::Stop:
                return 0x1F;
            case NonNasaWindDirection::Vertical:
                return 0x1A;
            case NonNasaWindDirection::Horizontal:
                return 0x1B;
            case NonNasaWindDirection::FourWay:
                return 0x1C;
            default:
                return 0x1F; // Default: swing off
            }
        }

        std::vector<uint8_t> NonNasaRequest::encode()
        {
            std::vector<uint8_t> data{
                0x32,                     // 00 start
                0xD0,                     // 01 src
                (uint8_t)hex_to_int(dst), // 02 dst
                0xB0,                     // 03 cmd
                0x1F,                     // 04 swing
                0x04,                     // 05 ?
                0,                        // 06 temp + fanmode
                0,                        // 07 operation mode
                0,                        // 08 power + individual mode
                0,                        // 09
                0,                        // 10
                0,                        // 11
                0,                        // 12 crc
                0x34                      // 13 end
            };

            // individual seems to deactivate the locale remotes with message "CENTRAL".
            // seems to be like a building management system.
            bool individual = false;

            data[4] = encode_request_wind_direction(wind_direction);
            if (room_temp > 0)
                data[5] = room_temp;
            data[6] = (target_temp & 31U) | encode_request_fanspeed(fanspeed);
            data[7] = (uint8_t)encode_request_mode(mode);
            data[8] = !power ? (uint8_t)0xC0 : (uint8_t)0xF0;
            data[8] |= (individual ? 6U : 4U);
            data[9] = (uint8_t)0x21;
            data[12] = build_checksum(data);

            return data;
        }

        NonNasaRequest NonNasaRequest::create(std::string dst_address)
        {
            NonNasaRequest request;
            request.dst = dst_address;

            auto it = last_command20s_.find(dst_address);
            if (it != last_command20s_.end())
            {
                auto &last_command20_ = it->second;
                request.room_temp = last_command20_.room_temp;
                request.power = last_command20_.power;
                request.target_temp = last_command20_.target_temp;
                request.fanspeed = last_command20_.fanspeed;
                request.mode = last_command20_.mode;
                request.wind_direction = last_command20_.wind_direction;
            }

            return request;
        }

        NonNasaMode mode_to_nonnasa_mode(Mode value)
        {
            switch (value)
            {
            case Mode::Auto:
                return NonNasaMode::Auto;
            case Mode::Cool:
                return NonNasaMode::Cool;
            case Mode::Dry:
                return NonNasaMode::Dry;
            case Mode::Fan:
                return NonNasaMode::Fan;
            case Mode::Heat:
                return NonNasaMode::Heat;
            default:
                return NonNasaMode::Auto;
            }
        }

        NonNasaFanspeed fanmode_to_nonnasa_fanspeed(FanMode value)
        {
            switch (value)
            {
            case FanMode::High:
                return NonNasaFanspeed::High;
            case FanMode::Mid:
                return NonNasaFanspeed::Medium;
            case FanMode::Low:
                return NonNasaFanspeed::Low;
            case FanMode::Auto:
            default:
                return NonNasaFanspeed::Auto;
            }
        }

        void NonNasaProtocol::publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request)
        {
            auto req = NonNasaRequest::create(address);

            if (request.mode)
            {
                request.power = true; // ensure system turns on when mode is set
                req.mode = mode_to_nonnasa_mode(request.mode.value());
            }

            if (request.power)
                req.power = request.power.value();

            if (request.target_temp)
                req.target_temp = request.target_temp.value();

            if (request.fan_mode)
                req.fanspeed = fanmode_to_nonnasa_fanspeed(request.fan_mode.value());

            if (request.alt_mode)
            {
                LOGW("change altmode is currently not implemented");
            }

            if (request.swing_mode)
            {
                NonNasaWindDirection wind_dir = swingmode_to_wind_direction(request.swing_mode.value());
                req.wind_direction = wind_dir;
            }

            // Add to the queue with the current time
            NonNasaRequestQueueItem reqItem = NonNasaRequestQueueItem();
            reqItem.request = req;
            reqItem.time = millis();
            reqItem.time_sent = 0;
            reqItem.retry_count = 0;
            reqItem.resend_count = 0;

            // Safety check the length of the queue (in case something is spamming control
            // requests we don't want the queue to get too large).
            if (nonnasa_requests.size() < 10)
            {
                nonnasa_requests.push_back(reqItem);
            }
        }

        Mode nonnasa_mode_to_mode(NonNasaMode value)
        {
            switch (value)
            {
            case NonNasaMode::Auto:
            case NonNasaMode::Auto_Heat:
                return Mode::Auto;
            case NonNasaMode::Cool:
                return Mode::Cool;
            case NonNasaMode::Dry:
                return Mode::Dry;
            case NonNasaMode::Fan:
                return Mode::Fan;
            case NonNasaMode::Heat:
                return Mode::Heat;
            default:
                return Mode::Auto;
            }
        }

        // TODO
        WaterHeaterMode nonnasa_water_heater_mode_to_mode(int value)
        {
            switch (value)
            {
            default:
                return WaterHeaterMode::Unknown;
            }
        }

        FanMode nonnasa_fanspeed_to_fanmode(NonNasaFanspeed fanspeed)
        {
            switch (fanspeed)
            {
            case NonNasaFanspeed::Fresh:
            case NonNasaFanspeed::High:
                return FanMode::High;
            case NonNasaFanspeed::Medium:
                return FanMode::Mid;
            case NonNasaFanspeed::Low:
                return FanMode::Low;
            default:
            case NonNasaFanspeed::Auto:
                return FanMode::Auto;
            }
        }

        DecodeResult try_decode_non_nasa_packet(std::vector<uint8_t> &data)
        {
            return nonpacket_.decode(data);
        }

        void send_requests(MessageTarget *target)
        {
            const uint32_t now = millis();
            for (auto &item : nonnasa_requests)
            {
                if (item.time_sent == 0)
                {
                    item.time_sent = now;
                    target->publish_data(0, item.request.encode());
                }
            }
        }

        void send_register_controller(MessageTarget *target)
        {
            LOGD("Sending controller registration request...");

            // Registers our device as a "controller" with the outdoor unit. This will cause the
            // outdoor unit to poll us with a request_control message approximately every second,
            // which we can reply to with a control message if required.
            std::vector<uint8_t> data{
                0x32, // 00 start
                0xD0, // 01 src
                0xc8, // 02 dst
                0xD1, // 03 cmd (register_device)
                0xD2, // 04 device_type (controller)
                0,    // 05
                0,    // 06
                0,    // 07
                0,    // 08
                0,    // 09
                0,    // 10
                0,    // 11
                0,    // 12 crc
                0x34  // 13 end
            };
            data[12] = build_checksum(data);

            // Send now
            target->publish_data(0, std::move(data));
        }

        void process_non_nasa_packet(MessageTarget *target)
        {
            if (debug_log_undefined_messages)
            {
                LOG_PACKET_RECV("RECV", nonpacket_);
            }

            target->register_address(nonpacket_.src);

            // Check if we have a message from the indoor unit. If so, we can assume it is awake.
            if (!indoor_unit_awake && get_address_type(nonpacket_.src) == AddressType::Indoor)
            {
                indoor_unit_awake = true;
            }

            if (nonpacket_.cmd == NonNasaCommand::Cmd20)
            {
                // We may occasionally not receive a control_acknowledgement message when sending a control
                // packet, so as a backup approach check if the state of the device matches that of the
                // sent control packet. This also serves as a backup approach if for some reason a device
                // doesn't send control_acknowledgement messages at all.
                if (debug_log_messages)
                {
                    // signature: same fields -> same signature
                    // Using a small packed int is enough for change detection
                    uint32_t sig = 0;
                    sig ^= ((uint32_t)(nonpacket_.command20.target_temp & 0x7F));
                    sig ^= ((uint32_t)(nonpacket_.command20.room_temp & 0x7F)) << 7;
                    sig ^= ((uint32_t)(nonpacket_.command20.pipe_in & 0x7F)) << 14;
                    sig ^= ((uint32_t)(nonpacket_.command20.pipe_out & 0x7F)) << 21;
                    sig ^= ((uint32_t)(nonpacket_.command20.power ? 1 : 0)) << 28;
                    sig ^= ((uint32_t)((uint8_t)nonpacket_.command20.mode & 0x0F)) << 29;
                    sig ^= ((uint32_t)((uint8_t)nonpacket_.command20.fanspeed)) * 2654435761u;
                    sig ^= ((uint32_t)((uint8_t)nonpacket_.command20.wind_direction)) * 2246822519u;

                    if (!debug_log_messages_on_change ||
                        log_should_print(log_dedup_key(nonpacket_.src, "nonnasa", 0x0020), (double)sig, 0.0, 0))
                    {

                        LOGI("Cmd20 received: src=%s, wind_direction=%d, target_temp=%d, power=%d, mode=%d, fanspeed=%d",
                             nonpacket_.src.c_str(),
                             (uint8_t)nonpacket_.command20.wind_direction,
                             nonpacket_.command20.target_temp,
                             nonpacket_.command20.power,
                             (uint8_t)nonpacket_.command20.mode,
                             (uint8_t)nonpacket_.command20.fanspeed);
                    }
                }

                size_t before_size = nonnasa_requests.size();
                nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                           { return item.time_sent > 0 &&
                                                    nonpacket_.src == item.request.dst &&
                                                    item.request.target_temp == nonpacket_.command20.target_temp &&
                                                    item.request.fanspeed == nonpacket_.command20.fanspeed &&
                                                    item.request.mode == nonpacket_.command20.mode &&
                                                    item.request.power == nonpacket_.command20.power &&
                                                    item.request.wind_direction == nonpacket_.command20.wind_direction; });
                size_t after_size = nonnasa_requests.size();
                if (before_size != after_size)
                {
                    LOGD("Cmd20: Removed %zu matching request(s) for %s (backup ack)",
                         before_size - after_size, nonpacket_.src.c_str());
                }

                // If a state update comes through after a control message has been sent, but before it
                // has been acknowledged, it should be ignored. This prevents the UI status bouncing
                // between states after a command has been issued.
                bool pending_control_message = false;
                for (auto &item : nonnasa_requests)
                {
                    if (item.time_sent > 0 && nonpacket_.src == item.request.dst)
                    {
                        pending_control_message = true;
                        break;
                    }
                }

                // Publish EVA (evaporator) temperatures - pipe_in/pipe_out are equivalent to eva_in/eva_out
                // These are sensor readings and should always be published, regardless of pending control messages
                // Compare to CmdC0 and Cmd8D handlers which explicitly do not check for pending control messages
                // Cast to int8_t first to preserve sign (uint8_t wraps negative values), then to float
                float pipe_in_temp = static_cast<float>(static_cast<int8_t>(nonpacket_.command20.pipe_in));
                float pipe_out_temp = static_cast<float>(static_cast<int8_t>(nonpacket_.command20.pipe_out));
                target->set_indoor_eva_in_temperature(nonpacket_.src, pipe_in_temp);
                target->set_indoor_eva_out_temperature(nonpacket_.src, pipe_out_temp);

                if (!pending_control_message)
                {
                    last_command20s_[nonpacket_.src] = nonpacket_.command20;
                    target->set_target_temperature(nonpacket_.src, nonpacket_.command20.target_temp);
                    // TODO
                    target->set_water_outlet_target(nonpacket_.src, false);
                    // TODO
                    target->set_target_water_temperature(nonpacket_.src, false);
                    target->set_room_temperature(nonpacket_.src, nonpacket_.command20.room_temp);
                    target->set_power(nonpacket_.src, nonpacket_.command20.power);
                    // TODO
                    target->set_water_heater_power(nonpacket_.src, false);
                    target->set_mode(nonpacket_.src, nonnasa_mode_to_mode(nonpacket_.command20.mode));
                    // TODO
                    target->set_water_heater_mode(nonpacket_.src, nonnasa_water_heater_mode_to_mode(-0));
                    target->set_fanmode(nonpacket_.src, nonnasa_fanspeed_to_fanmode(nonpacket_.command20.fanspeed));
                    // TODO
                    target->set_altmode(nonpacket_.src, 0);
                    // Cmd20 swing decode: converting wind_direction to vertical/horizontal booleans
                    target->set_swing_horizontal(nonpacket_.src,
                                                 (nonpacket_.command20.wind_direction == NonNasaWindDirection::Horizontal) ||
                                                     (nonpacket_.command20.wind_direction == NonNasaWindDirection::FourWay));
                    target->set_swing_vertical(nonpacket_.src,
                                               (nonpacket_.command20.wind_direction == NonNasaWindDirection::Vertical) ||
                                                   (nonpacket_.command20.wind_direction == NonNasaWindDirection::FourWay));
                }
            }
            else if (nonpacket_.cmd == NonNasaCommand::CmdC0)
            {
                // CmdC0 comes from the outdoor unit and contains outdoor temperature
                // The temperature is already in Celsius (after subtracting 55 from raw value)
                // Note: No pending control message check needed here since CmdC0 comes from the
                // outdoor unit (typically "c8"), while control messages are sent to indoor units.
                // Outdoor temperature updates are independent status data and should always be processed.
                // Cast to int8_t first to preserve sign (uint8_t wraps negative values), then to float
                float temp = static_cast<float>(static_cast<int8_t>(nonpacket_.commandC0.outdoor_unit_outdoor_temp_c));
                target->set_outdoor_temperature(nonpacket_.src, temp);
            }
            else if (nonpacket_.cmd == NonNasaCommand::Cmd8D)
            {
                // Cmd8D comes from the outdoor unit and contains power/energy sensor data
                // Note: No pending control message check needed here since Cmd8D comes from the
                // outdoor unit (typically "c8"), while control messages are sent to indoor units.
                // Outdoor power/energy updates are independent status data and should always be processed.
                // Note: Following NASA protocol approach - publish raw current value, sensor filter will apply.
                target->set_outdoor_instantaneous_power(nonpacket_.src, nonpacket_.command8D.inverter_power_w);
                target->set_outdoor_current(nonpacket_.src, nonpacket_.command8D.inverter_current_a);
                target->set_outdoor_voltage(nonpacket_.src, nonpacket_.command8D.inverter_voltage_v);

                // Calculate cumulative energy by integrating power over time using trapezoidal rule
                // The helper function handles all edge cases: wraparound, first update, time delta validation
                CumulativeEnergyTracker &tracker = cumulative_energy_trackers_[nonpacket_.src];
                const uint32_t now = millis();
                update_cumulative_energy_tracker(tracker, nonpacket_.command8D.inverter_power_w, now);

                // Publish cumulative energy
                // Sensor has filter multiply: 0.001 and unit is kWh
                // NASA protocol publishes raw value in Wh, filter converts to kWh
                // So we publish in Wh (accumulated_energy_kwh * 1000), filter converts to kWh
                // Convert from double to float for API (API requires float)
                float cumulative_energy_wh = static_cast<float>(tracker.accumulated_energy_kwh * 1000.0);
                target->set_outdoor_cumulative_energy(nonpacket_.src, cumulative_energy_wh);
            }
            else if (nonpacket_.cmd == NonNasaCommand::CmdF0)
            {
                // CmdF0 comes from the outdoor unit and contains error code and status information
                // Note: No pending control message check needed here since CmdF0 comes from the
                // outdoor unit (typically "c8"), while control messages are sent to indoor units.
                // Outdoor error code updates are independent status data and should always be processed.
                int error_code = static_cast<int>(nonpacket_.commandF0.outdoor_unit_error_code);
                if (debug_log_messages && error_code != 0)
                {
                    LOGW("s:%s d:%s CmdF0 outdoor_unit_error_code %d", nonpacket_.src.c_str(), nonpacket_.dst.c_str(), error_code);
                }
                target->set_error_code(nonpacket_.src, error_code);
            }
            else if (nonpacket_.cmd == NonNasaCommand::CmdC6)
            {
                // We have received a request_control message. This is a message outdoor units will
                // send to a registered controller, allowing us to reply with any control commands.
                // Control commands should be sent immediately (per SNET Pro behaviour).
                if (nonpacket_.src == "c8" && nonpacket_.dst == "d0" && nonpacket_.commandC6.control_status == true)
                {
                    if (controller_registered == false)
                    {
                        LOGD("Controller registered");
                        controller_registered = true;
                    }
                    if (indoor_unit_awake)
                    {
                        // We know the outdoor unit is awake due to this request_control message, so we only
                        // need to check that the indoor unit is awake.
                        send_requests(target);
                    }
                }
            }
            else if (nonpacket_.cmd == NonNasaCommand::Cmd54 && nonpacket_.dst == "d0")
            {
                // We have received a control_acknowledgement message. This message will come from an
                // indoor unit in reply to a control message from us, allowing us to confirm the control
                // message was successfully sent. The data portion contains the same data we sent (however
                // we can just assume it's for any sent packet, rather than comparing).
                nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                           { return item.time_sent > 0 && nonpacket_.src == item.request.dst; });
            }
            else if (nonpacket_.src == "c8" && nonpacket_.dst == "ad" && (nonpacket_.commandRaw.data[0] & 1) == 1)
            {
                // We have received a broadcast registration request. It isn't necessary to register
                // more than once, however we can use this as a keepalive method. A 30ms delay is added
                // to allow other controllers to register. This mimics SNET Pro behaviour.
                // It's unknown why the first data byte must be odd.
                if (non_nasa_keepalive)
                {
                    const uint32_t now = millis();
                    // rate limit
                    // Wrap-safe elapsed time (unsigned subtraction works across millis() rollover)
                    const uint32_t elapsed_ms = now - last_keepalive_sent_ms_;

                    if (elapsed_ms >= KEEPALIVE_MIN_INTERVAL_MS)
                    {
                        pending_keepalive_ = true;
                        pending_keepalive_due_ms_ = now + KEEPALIVE_DELAY_MS;
                    }
                }
            }
        }

        void NonNasaProtocol::protocol_update(MessageTarget *target)
        {
            // non-blocking keepalive send (scheduled from broadcast request)
            if (non_nasa_keepalive && pending_keepalive_)
            {
                const uint32_t now = millis();
                if ((int32_t)(now - pending_keepalive_due_ms_) >= 0)
                {
                    send_register_controller(target);
                    last_keepalive_sent_ms_ = now;
                    pending_keepalive_ = false;
                }
            }
            else if (!non_nasa_keepalive)
            {
                pending_keepalive_ = false;
            }

            // If we're not currently registered, keep sending a registration request until it has
            // been confirmed by the outdoor unit.
            if (!controller_registered)
            {
                send_register_controller(target);
            }

            // If we have *any* messages in the queue for longer than 15s, assume failure and
            // remove from queue (the AC or UART connection is likely offline).
            const uint32_t now = millis();
            nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                       { return now - item.time > 15000; });

            // If we have any *sent* messages in the queue that haven't received an ack in under 5s,
            // assume they failed and queue for resend on the next request_control message. Retry at
            // most 3 times.
            for (auto &item : nonnasa_requests)
            {
                if (item.time_sent > 0 && item.resend_count < 3 && now - item.time_sent > 4500)
                {
                    item.time_sent = 0; // Resend
                    item.resend_count++;
                }
            }

            // If we have any *unsent* messages in the queue for over 1000ms, it likely means the indoor
            // and/or outdoor unit has gone to sleep due to inactivity. Send a registration request to
            // wake the unit up.
            for (auto &item : nonnasa_requests)
            {
                if (item.time_sent == 0 && now - item.time > 1000 && item.resend_count == 0 && item.retry_count == 0)
                {
                    // Both the outdoor and the indoor unit must be awake before we can send a command
                    indoor_unit_awake = false;
                    item.retry_count++;
                    LOGD("Device is likely sleeping, waking...");
                    send_register_controller(target);
                    break;
                }
            }
        }
    } // namespace samsung_ac
} // namespace esphome
