#include <queue>
#include <map>
#include <cmath>
#include <string>
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "util.h"
#include "protocol_non_nasa.h"

std::map<std::string, esphome::samsung_ac::NonNasaCommand20> last_command20s_;

esphome::samsung_ac::NonNasaDataPacket nonpacket_;

namespace esphome
{
    namespace samsung_ac
    {

        std::list<NonNasaRequestQueueItem> nonnasa_requests;
        bool controller_registered = false;
        bool indoor_unit_awake = true;

        bool has_pending_control_message(const std::string &src)
        {
            for (const auto &item : nonnasa_requests)
            {
                if (item.time_sent > 0 && src == item.request.dst)
                {
                    return true;
                }
            }
            return false;
        }

        uint8_t build_checksum(std::vector<uint8_t> &data)
        {
            if (data.size() < 12)
            {
                ESP_LOGE(TAG, "Invalid data size for checksum calculation. Expected at least 12 bytes, got %d.", data.size());
                return 0;
            }

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
            if (data[0] != 0x32)
                return DecodeResult::InvalidStartByte;

            if (data.size() != 14)
                return DecodeResult::UnexpectedSize;

            if (data[data.size() - 1] != 0x34)
                return DecodeResult::InvalidEndByte;

            auto crc_expected = build_checksum(data);
            auto crc_actual = data[data.size() - 2];
            if (crc_actual != crc_expected)
            {
                ESP_LOGW(TAG, "NonNASA: invalid crc - got %d but should be %d: %s", crc_actual, crc_expected, bytes_to_hex(data).c_str());
                return DecodeResult::CrcError;
            }

            src = long_to_hex(data[1]);
            dst = long_to_hex(data[2]);

            cmd = (NonNasaCommand)data[3];
            switch (cmd)
            {
            case NonNasaCommand::Cmd20: // temperatures
            {
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

                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdC0: // outdoor unit data
            {
                commandC0.outdoor_unit_operation_mode = data[4]; // modes need to be specified
                commandC0.outdoor_unit_4_way_valve = (data[6] & 0b10000000) != 0;
                commandC0.outdoor_unit_hot_gas_bypass = (data[6] & 0b00100000) != 0;
                commandC0.outdoor_unit_compressor = (data[6] & 0b00000100) != 0;
                commandC0.outdoor_unit_ac_fan = (data[7] & 0b00000011) != 0;
                commandC0.outdoor_unit_outdoor_temp_c = data[8] - 55;
                commandC0.outdoor_unit_discharge_temp_c = data[10] - 55;
                commandC0.outdoor_unit_condenser_mid_temp_c = data[11] - 55;
                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdC1: // outdoor unit data
            {
                commandC1.outdoor_unit_sump_temp_c = data[8] - 55;
                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdC6:
            {
                commandC6.control_status = data[4];
                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdF0: // outdoor unit data
            {
                commandF0.outdoor_unit_freeze_protection = (data[4] & 0b10000000) != 0;
                commandF0.outdoor_unit_heating_overload = (data[4] & 0b01000000) != 0;
                commandF0.outdoor_unit_defrost_control = (data[4] & 0b00100000) != 0;
                commandF0.outdoor_unit_discharge_protection = (data[4] & 0b00010000) != 0;
                commandF0.outdoor_unit_current_control = (data[4] & 0b00001000) != 0;
                commandF0.inverter_order_frequency_hz = data[5];
                commandF0.inverter_target_frequency_hz = data[6];
                commandF0.inverter_current_frequency_hz = data[7];
                commandF0.outdoor_unit_bldc_fan = (data[8] & 0b00000011) != 0; // not sure if correct, i have no ou with BLDC-fan
                commandF0.outdoor_unit_error_code = data[10];
                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdF1: // outdoor unit eev-values
            {
                commandF1.outdoor_unit_EEV_A = (data[4] * 256) + data[5];
                commandF1.outdoor_unit_EEV_B = (data[6] * 256) + data[7];
                commandF1.outdoor_unit_EEV_C = (data[8] * 256) + data[9];
                commandF1.outdoor_unit_EEV_D = (data[10] * 256) + data[11];
                return DecodeResult::Ok;
            }
            case NonNasaCommand::CmdF3: // power consumption
            {
                // Maximum frequency for Inverter (compressor-motor of outdoor-unit) in Hz
                commandF3.inverter_max_frequency_hz = data[4];
                // Sum of required heating/cooling capacity ordered by the indoor-units in kW
                commandF3.inverter_total_capacity_requirement_kw = (float)data[5] / 10;
                // DC-current to the inverter of outdoor-unit in A
                commandF3.inverter_current_a = (float)data[8] / 10;
                // voltage of the DC-link to inverter in V
                commandF3.inverter_voltage_v = (float)data[9] * 2;
                // Power consumption of the outdoo unit inverter in W
                commandF3.inverter_power_w = commandF3.inverter_current_a * commandF3.inverter_voltage_v;
                return DecodeResult::Ok;
            }
            default:
            {
                // ESP_LOGW(TAG, "Unknown or unsupported command received: %02X", (uint8_t)cmd);

                commandRaw.length = data.size() - 4 - 1;
                auto begin = data.begin() + 4;
                std::copy(begin, begin + commandRaw.length, commandRaw.data);

                return DecodeResult::UnknownCommand;
            }
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

        std::vector<uint8_t> NonNasaRequest::encode()
        {
            std::vector<uint8_t> data{
                0x32,                     // 00 start
                0xD0,                     // 01 src
                (uint8_t)hex_to_int(dst), // 02 dst
                0xB0,                     // 03 cmd
                0x1F,                     // 04 ?
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

            if (room_temp > 0)
                data[5] = room_temp;
            data[6] = (target_temp & 31U) | encode_request_fanspeed(fanspeed);
            data[7] = (uint8_t)encode_request_mode(mode);
            data[8] = !power ? (uint8_t)0xC0 : (uint8_t)0xF0;
            data[8] |= (individual ? 6U : 4U);
            data[9] = (uint8_t)0x21;
            data[12] = build_checksum(data);

            data[9] = (uint8_t)0x21;

            return data;
        }

        NonNasaRequest NonNasaRequest::create(const std::string &dst_address)
        {
            NonNasaRequest request;
            request.dst = dst_address;

            const auto &last_command20_ = last_command20s_[dst_address];
            request.room_temp = last_command20_.room_temp;
            request.power = last_command20_.power;
            request.target_temp = last_command20_.target_temp;
            request.fanspeed = last_command20_.fanspeed;
            request.mode = last_command20_.mode;

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
                ESP_LOGW(TAG, "change altmode is currently not implemented");
            }

            if (request.swing_mode)
            {
                ESP_LOGW(TAG, "change swingmode is currently not implemented");
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

        DecodeResult try_decode_non_nasa_packet(std::vector<uint8_t> data)
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
                    auto data = item.request.encode();
                    target->publish_data(data);
                }
            }
        }

        void send_register_controller(MessageTarget *target)
        {
            ESP_LOGD(TAG, "Sending controller registration request...");

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
            target->publish_data(data);
        }

        void process_non_nasa_packet(MessageTarget *target)
        {
            if (debug_log_undefined_messages)
            {
                ESP_LOGW(TAG, "MSG: %s", nonpacket_.to_string().c_str());
            }

            target->register_address(nonpacket_.src);

            // Check if we have a message from the indoor unit. If so, we can assume it is awake.
            if (!indoor_unit_awake && get_address_type(nonpacket_.src) == AddressType::Indoor)
            {
                indoor_unit_awake = true;
            }

            switch (nonpacket_.cmd)
            {
            case NonNasaCommand::Cmd20:
            { // We may occasionally not receive a control_acknowledgement message when sending a control
                // packet, so as a backup approach check if the state of the device matches that of the
                // sent control packet. This also serves as a backup approach if for some reason a device
                // doesn't send control_acknowledgement messages at all.
                nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                           { return item.time_sent > 0 &&
                                                    nonpacket_.src == item.request.dst &&
                                                    item.request.target_temp == nonpacket_.command20.target_temp &&
                                                    item.request.fanspeed == nonpacket_.command20.fanspeed &&
                                                    item.request.mode == nonpacket_.command20.mode &&
                                                    item.request.power == nonpacket_.command20.power; });

                // If a state update comes through after a control message has been sent, but before it
                // has been acknowledged, it should be ignored. This prevents the UI status bouncing
                // between states after a command has been issued.
                if (!has_pending_control_message(nonpacket_.src))
                {
                    last_command20s_[nonpacket_.src] = nonpacket_.command20;
                    target->set_target_temperature(nonpacket_.src, nonpacket_.command20.target_temp);
                    target->set_water_outlet_target(nonpacket_.src, false);      // TODO
                    target->set_target_water_temperature(nonpacket_.src, false); // TODO
                    target->set_room_temperature(nonpacket_.src, nonpacket_.command20.room_temp);
                    target->set_power(nonpacket_.src, nonpacket_.command20.power);
                    target->set_water_heater_power(nonpacket_.src, false); // TODO
                    target->set_mode(nonpacket_.src, nonnasa_mode_to_mode(nonpacket_.command20.mode));
                    target->set_water_heater_mode(nonpacket_.src, nonnasa_water_heater_mode_to_mode(-0)); // TODO
                    target->set_fanmode(nonpacket_.src, nonnasa_fanspeed_to_fanmode(nonpacket_.command20.fanspeed));
                    target->set_altmode(nonpacket_.src, 0);              // TODO
                    target->set_swing_horizontal(nonpacket_.src, false); // TODO
                    target->set_swing_vertical(nonpacket_.src, false);   // TODO
                }
                break;
            }
            case NonNasaCommand::CmdC6:
            {
                // We have received a request_control message. This is a message outdoor units will
                // send to a registered controller, allowing us to reply with any control commands.
                // Control commands should be sent immediately (per SNET Pro behaviour).
                if (nonpacket_.src == "c8" && nonpacket_.dst == "d0" && nonpacket_.commandC6.control_status == true)
                {
                    if (controller_registered == false)
                    {
                        ESP_LOGD(TAG, "Controller registered");
                        controller_registered = true;
                    }
                    if (indoor_unit_awake)
                    {
                        // We know the outdoor unit is awake due to this request_control message, so we only
                        // need to check that the indoor unit is awake.
                        send_requests(target);
                    }
                }
                break;
            }
            case NonNasaCommand::Cmd54:
            {
                if (nonpacket_.dst == "d0")
                {
                    // We have received a control_acknowledgement message. This message will come from an
                    // indoor unit in reply to a control message from us, allowing us to confirm the control
                    // message was successfully sent. The data portion contains the same data we sent (however
                    // we can just assume it's for any sent packet, rather than comparing).
                    nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                               { return item.time_sent > 0 && nonpacket_.src == item.request.dst; });
                }
                break;
            }
            case NonNasaCommand::CmdC0:
            {
                // Add checks to ensure pending messages are not overwritten
                if (!has_pending_control_message(nonpacket_.src))
                {
                    // Publish outdoor temperature if there are no pending control messages
                    target->set_outdoor_temperature(nonpacket_.src, nonpacket_.commandC0.outdoor_unit_outdoor_temp_c);
                }
                break;
            }
            case NonNasaCommand::CmdF3:
            {
                // Add checks to ensure pending messages are not overwritten
                if (!has_pending_control_message(nonpacket_.src))
                {
                    // Publish power energy if there are no pending control messages
                    target->set_outdoor_instantaneous_power(nonpacket_.src, nonpacket_.commandF3.inverter_power_w);
                    target->set_outdoor_cumulative_energy(nonpacket_.src, nonpacket_.commandF3.inverter_total_capacity_requirement_kw);
                    target->set_outdoor_current(nonpacket_.src, nonpacket_.commandF3.inverter_current_a);
                    target->set_outdoor_voltage(nonpacket_.src, nonpacket_.commandF3.inverter_voltage_v);
                }
                break;
            }
            default:
            {
                if (nonpacket_.src == "c8" && nonpacket_.dst == "ad" && (nonpacket_.commandRaw.data[0] & 1) == 1)
                {
                    // We have received a broadcast registration request. It isn't necessary to register
                    // more than once, however we can use this as a keepalive method. A 30ms delay is added
                    // to allow other controllers to register. This mimics SNET Pro behaviour.
                    // It's unknown why the first data byte must be odd.
                    if (non_nasa_keepalive)
                    {
                        delay(30);
                        send_register_controller(target);
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "Received unknown command: %02X from %s to %s", (uint8_t)nonpacket_.cmd, nonpacket_.src.c_str(), nonpacket_.dst.c_str());
                }
                break;
            }
            }
        }

        void NonNasaProtocol::protocol_update(MessageTarget *target)
        {
            // Get the current time
            const uint32_t now = millis();
            if (start_millis == 0)
            {
                start_millis = millis();
            }

            // Dynamic delay on startup
            static uint32_t dynamic_delay_interval = 10000;   // Initial delay: 10 seconds
            static const uint32_t max_delay_interval = 60000; // Maximum delay: 60 seconds

            // If the controller is not yet registered
            if (!controller_registered)
            {
                // If the dynamic delay interval has passed, send a registration request
                if (now - start_millis >= dynamic_delay_interval)
                {
                    send_register_controller(target);
                    ESP_LOGD(TAG, "Sending registration request with delay: %d ms", dynamic_delay_interval);

                    // If the registration fails, double the delay, but do not exceed the maximum delay
                    dynamic_delay_interval = std::min(dynamic_delay_interval * 2, max_delay_interval);
                }
            }
            else
            {
                // Reset the delay interval if the registration was successful
                dynamic_delay_interval = 10000; // Reset to initial delay
            }

            // Remove messages from the queue that have been waiting for more than 15 seconds
            nonnasa_requests.remove_if([&](const NonNasaRequestQueueItem &item)
                                       { return now - item.time > 15000; });

            // If any sent messages have not received an acknowledgment within 5 seconds,
            // assume they failed and queue them for resend, retrying up to 3 times
            for (auto &item : nonnasa_requests)
            {
                if (item.time_sent > 0 && item.resend_count < 3 && now - item.time_sent > 4500)
                {
                    item.time_sent = 0; // Resend
                    item.resend_count++;
                }
            }

            // Check unsent messages in the queue and send a registration request if necessary
            for (auto &item : nonnasa_requests)
            {
                if (item.time_sent == 0 && now - item.time > 1000 && item.resend_count == 0 && item.retry_count == 0)
                {
                    // Both the indoor and outdoor units must be awake before sending a command
                    indoor_unit_awake = false;
                    item.retry_count++;
                    ESP_LOGD(TAG, "Device is likely sleeping, waking...");
                    send_register_controller(target);
                    break;
                }
            }
        }
    } // namespace samsung_ac
} // namespace esphome
