#include <queue>
#include <iostream>
#include <set>
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/hal.h"
#include "util.h"
#include "protocol_nasa.h"
#include "debug_mqtt.h"
#include "samsung_ac_device_custClim.h"
#include "debug_number.h"

esphome::samsung_ac::Packet packet_;

namespace esphome
{
    namespace samsung_ac
    {
        int variable_to_signed(int value)
        {
            if (value < 65535 /*uint16 max*/)
                return value;
            return value - (int)65535 /*uint16 max*/ - 1.0;
        }

        uint16_t crc16(std::vector<uint8_t> &data, int startIndex, int length)
        {
            uint16_t crc = 0;
            for (int index = startIndex; index < startIndex + length; ++index)
            {
                crc = crc ^ ((uint16_t)((uint8_t)data[index]) << 8);
                for (uint8_t i = 0; i < 8; i++)
                {
                    if (crc & 0x8000)
                        crc = (crc << 1) ^ 0x1021;
                    else
                        crc <<= 1;
                }
            }
            return crc;
        };

        Address Address::get_my_address()
        {
            Address address;
            address.klass = AddressClass::JIGTester;
            address.channel = 0xFF;
            address.address = 0;
            return address;
        }

        Address Address::parse(const std::string &str)
        {
            Address address;
            char *pEnd;
            address.klass = (AddressClass)strtol(str.c_str(), &pEnd, 16);
            pEnd++; // .
            address.channel = strtol(pEnd, &pEnd, 16);
            pEnd++; // .
            address.address = strtol(pEnd, &pEnd, 16);
            return address;
        }

        void Address::decode(std::vector<uint8_t> &data, unsigned int index)
        {
            klass = (AddressClass)data[index];
            channel = data[index + 1];
            address = data[index + 2];
        }

        void Address::encode(std::vector<uint8_t> &data)
        {
            data.push_back((uint8_t)klass);
            data.push_back(channel);
            data.push_back(address);
        }

        std::string Address::to_string()
        {
            char str[9];
            sprintf(str, "%02x.%02x.%02x", (int)klass, channel, address);
            return str;
        }

        void Command::decode(std::vector<uint8_t> &data, unsigned int index)
        {
            packetInformation = ((int)data[index] & 128) >> 7 == 1;
            protocolVersion = (uint8_t)(((int)data[index] & 96) >> 5);
            retryCount = (uint8_t)(((int)data[index] & 24) >> 3);
            packetType = (PacketType)(((int)data[index + 1] & 240) >> 4);
            dataType = (DataType)((int)data[index + 1] & 15);
            packetNumber = data[index + 2];
        }

        void Command::encode(std::vector<uint8_t> &data)
        {
            data.push_back((uint8_t)((((int)packetInformation ? 1 : 0) << 7) + ((int)protocolVersion << 5) + ((int)retryCount << 3)));
            data.push_back((uint8_t)(((int)packetType << 4) + (int)dataType));
            data.push_back(packetNumber);
        }

        std::string Command::to_string()
        {
            std::string str;
            str += "{";
            str += "PacketInformation: " + std::to_string(packetInformation) + ";";
            str += "ProtocolVersion: " + std::to_string(protocolVersion) + ";";
            str += "RetryCount: " + std::to_string(retryCount) + ";";
            str += "PacketType: " + std::to_string((int)packetType) + ";";
            str += "DataType: " + std::to_string((int)dataType) + ";";
            str += "PacketNumber: " + std::to_string(packetNumber);
            str += "}";
            return str;
        }

        MessageSet MessageSet::decode(std::vector<uint8_t> &data, unsigned int index, int capacity)
        {
            MessageSet set = MessageSet((MessageNumber)((uint32_t)data[index] * 256U + (uint32_t)data[index + 1]));
            switch (set.type)
            {
            case Enum:
                set.value = (int)data[index + 2];
                set.size = 3;
                break;
            case Variable:
                set.value = (int)data[index + 2] << 8 | (int)data[index + 3];
                if (set.value > 32768) set.value -= 65535;
                set.size = 4;
                break;
            case LongVariable:
                set.value = (int)data[index + 2] << 24 | (int)data[index + 3] << 16 | (int)data[index + 4] << 8 | (int)data[index + 5];
                set.size = 6;
                break;

            case Structure:
                if (capacity != 1)
                {
                    ESP_LOGE(TAG, "structure messages can only have one message but is %d", capacity);
                    return set;
                }
                Buffer buffer;
                set.size = data.size() - index - 3; // 3=end bytes
                buffer.size = set.size - 2;
                for (int i = 0; i < buffer.size; i++)
                {
                    buffer.data[i] = data[i];
                }
                set.structure = buffer;
                break;
            default:
                ESP_LOGE(TAG, "Unkown type");
            }

            return set;
        };

        void MessageSet::encode(std::vector<uint8_t> &data)
        {
            uint16_t messageNumber = (uint16_t)this->messageNumber;
            data.push_back((uint8_t)((messageNumber >> 8) & 0xff));
            data.push_back((uint8_t)(messageNumber & 0xff));

            switch (type)
            {
            case Enum:
                data.push_back((uint8_t)value);
                break;
            case Variable:
                if (value < 0) value += 65535;
                data.push_back((uint8_t)(value >> 8) & 0xff);
                data.push_back((uint8_t)(value & 0xff));
                break;
            case LongVariable:
                data.push_back((uint8_t)(value & 0x000000ff));
                data.push_back((uint8_t)((value & 0x0000ff00) >> 8));
                data.push_back((uint8_t)((value & 0x00ff0000) >> 16));
                data.push_back((uint8_t)((value & 0xff000000) >> 24));
                break;

            case Structure:
                for (int i = 0; i < structure.size; i++)
                {
                    data.push_back(structure.data[i]);
                }
                break;
            default:
                ESP_LOGE(TAG, "Unkown type");
            }
        }

        std::string MessageSet::to_string()
        {
            switch (type)
            {
            case Enum:
                return "Enum " + long_to_hex((uint16_t)messageNumber) + " = " + std::to_string(value);
            case Variable:
                return "Variable " + long_to_hex((uint16_t)messageNumber) + " = " + std::to_string(value);
            case LongVariable:
                return "LongVariable " + long_to_hex((uint16_t)messageNumber) + " = " + std::to_string(value);
            case Structure:
                return "Structure #" + long_to_hex((uint16_t)messageNumber) + " = " + std::to_string(structure.size);
            default:
                return "Unknown";
            }
        }

        static int _packetCounter = 0;

        std::vector<Packet> out;

        /*
                class OutgoingPacket
                {
                public:
                    OutgoingPacket(uint32_t timeout_seconds, Packet packet)
                    {
                        this->timeout_mili = millis() + (timeout_seconds * 1000);
                        Packet = packet;
                    }

                    // std::function<void(float)> Func;
                    Packet Packet;

                    bool IsTimedout()
                    {
                        return timeout_mili < millis();
                    };

                private:
                    uint32_t timeout_mili{0}; // millis();
                };
        */
        Packet Packet::create(Address da, DataType dataType, MessageNumber messageNumber, int value)
        {
            Packet packet = createa_partial(da, dataType);
            MessageSet message(messageNumber);
            message.value = value;
            packet.messages.push_back(message);
            out.push_back(packet);

            return packet;
        }

        Packet Packet::createa_partial(Address da, DataType dataType)
        {
            Packet packet;
            packet.sa = Address::get_my_address();
            packet.da = da;
            packet.command.packetInformation = true;
            packet.command.packetType = PacketType::Normal;
            packet.command.dataType = dataType;
            packet.command.packetNumber = _packetCounter++;
            return packet;
        }

        DecodeResult Packet::decode(std::vector<uint8_t> &data)
        {
            if (data[0] != 0x32)
                return DecodeResult::InvalidStartByte;

            if (data.size() < 16 || data.size() > 1500)
                return DecodeResult::UnexpectedSize;

            int size = (int)data[1] << 8 | (int)data[2];
            if (size + 2 != data.size())
                return DecodeResult::SizeDidNotMatch;

            if (data[data.size() - 1] != 0x34)
                return DecodeResult::InvalidEndByte;

            uint16_t crc_actual = crc16(data, 3, size - 4);
            uint16_t crc_expected = (int)data[data.size() - 3] << 8 | (int)data[data.size() - 2];
            if (crc_expected != crc_actual)
            {
                ESP_LOGW(TAG, "NASA: invalid crc - got %d but should be %d: %s", crc_actual, crc_expected, bytes_to_hex(data).c_str());
                return DecodeResult::CrcError;
            }

            unsigned int cursor = 3;

            sa.decode(data, cursor);
            cursor += sa.size;

            da.decode(data, cursor);
            cursor += da.size;

            command.decode(data, cursor);
            cursor += command.size;

            int capacity = (int)data[cursor];
            cursor++;

            messages.clear();
            for (int i = 1; i <= capacity; ++i)
            {
                MessageSet set = MessageSet::decode(data, cursor, capacity);
                messages.push_back(set);
                cursor += set.size;
            }

            return DecodeResult::Ok;
        };

        std::vector<uint8_t> Packet::encode()
        {
            std::vector<uint8_t> data;

            data.push_back(0x32);
            data.push_back(0); // size
            data.push_back(0); // size
            sa.encode(data);
            da.encode(data);
            command.encode(data);

            data.push_back((uint8_t)messages.size());
            for (int i = 0; i < messages.size(); i++)
            {
                messages[i].encode(data);
            }

            int endPosition = data.size() + 1;
            data[1] = (uint8_t)(endPosition >> 8);
            data[2] = (uint8_t)(endPosition & (int)0xFF);

            uint16_t checksum = crc16(data, 3, endPosition - 4);
            data.push_back((uint8_t)((unsigned int)checksum >> 8));
            data.push_back((uint8_t)((unsigned int)checksum & (unsigned int)0xFF));

            data.push_back(0x34);

            /*
            for (int i = 0; i < 100; ++i)
                data.insert(data.begin(), 0x55); // Preamble
            */

            return data;
        };

        std::string Packet::to_string()
        {
            std::string str;
            str += "#Packet Src:" + sa.to_string() + " Dst:" + da.to_string() + " " + command.to_string() + "\n";

            for (int i = 0; i < messages.size(); i++)
            {
                if (i > 0)
                    str += "\n";
                str += " > " + messages[i].to_string();
            }

            return str;
        }

        int fanmode_to_nasa_fanmode(FanMode mode)
        {
            // This stuff did not exists in XML only in Remcode.dll
            switch (mode)
            {
            case FanMode::Low:
                return 1;
            case FanMode::Mid:
                return 2;
            case FanMode::High:
                return 3;
            case FanMode::Turbo:
                return 4;
            case FanMode::Auto:
            default:
                return 0;
            }
        }

        void NasaProtocol::publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request)
        {
            Packet packet = Packet::createa_partial(Address::parse(address), DataType::Request);

            if (request.caller.has_value()) { // customClimate
                Samsung_AC_CustClim *caller = request.caller.value();
                if (caller->presToSend >= 0) {
                    MessageSet pres((MessageNumber)caller->presAddr);
                    pres.value = caller->presToSend;
                    packet.messages.push_back(pres);
                    ESP_LOGI(TAG, "Pushing pres %i at 0x%X for %s", pres.value, (MessageNumber)caller->presAddr, address.c_str());
                    caller->presToSend = -1;
                }
            }

            if (request.mode)
            {
                MessageNumber addr = MessageNumber::ENUM_in_operation_mode;
                if (request.caller.has_value()) {
                    addr = (MessageNumber)request.caller.value()->modeAddr;
                } else {
                    request.power = true; // ensure system turns on when mode is set
                }
                MessageSet mode(addr);
                mode.value = (int)request.mode.value();
                packet.messages.push_back(mode);
                ESP_LOGI(TAG, "Pushing mode %i at 0x%X for %s", mode.value , addr, address.c_str());
            }

            if (request.power)
            {
                MessageNumber addr = request.caller.has_value() ? (MessageNumber)request.caller.value()->enable: MessageNumber::ENUM_in_operation_power;
                MessageSet power(addr);
                power.value = request.power.value() ? 1 : 0;
                packet.messages.push_back(power);
                ESP_LOGI(TAG, "Pushing power %u at 0x%X for %s", power.value  , addr, address.c_str());
            }

            if (request.target_temp)
            {
                MessageNumber addr = request.caller.has_value() ? (MessageNumber)request.caller.value()->set: MessageNumber::VAR_in_temp_target_f;
                MessageSet targettemp(addr);
                targettemp.value = request.target_temp.value() * 10.0;
                packet.messages.push_back(targettemp);
            }

            if (request.fan_mode)
            {
                MessageSet fanmode(MessageNumber::ENUM_in_fan_mode);
                fanmode.value = fanmode_to_nasa_fanmode(request.fan_mode.value());
                packet.messages.push_back(fanmode);
            }

            if (request.alt_mode)
            {
                MessageSet altmode(MessageNumber::ENUM_in_alt_mode);
                altmode.value = request.alt_mode.value();
                packet.messages.push_back(altmode);
            }

            if (request.swing_mode)
            {
                MessageSet hl_swing(MessageNumber::ENUM_in_louver_hl_swing);
                hl_swing.value = static_cast<uint8_t>(request.swing_mode.value()) & 1;
                packet.messages.push_back(hl_swing);

                MessageSet lr_swing(MessageNumber::ENUM_in_louver_lr_swing);
                lr_swing.value = (static_cast<uint8_t>(request.swing_mode.value()) >> 1) & 1;
                packet.messages.push_back(lr_swing);
            }

            if (packet.messages.size() == 0)
                return;

            ESP_LOGW(TAG, "publish packet %s", packet.to_string().c_str());

            out.push_back(packet);

            auto data = packet.encode();
            target->publish_data(data);
        }

        Mode operation_mode_to_mode(int value)
        {
            switch (value)
            {
            case 0:
                return Mode::Auto;
            case 1:
                return Mode::Cool;
            case 2:
                return Mode::Dry;
            case 3:
                return Mode::Fan;
            case 4:
                return Mode::Heat;
                // case 21:  Cool Storage
                // case 24: Hot Water
            default:
                return Mode::Unknown;
            }
        }

        FanMode fan_mode_real_to_fanmode(int value)
        {
            switch (value)
            {
            case 1: // Low
                return FanMode::Low;
            case 2: // Mid
                return FanMode::Mid;
            case 3: // High
                return FanMode::High;
            case 4: // Turbo
                return FanMode::Turbo;
            case 10: // AutoLow
            case 11: // AutoMid
            case 12: // AutoHigh
            case 13: // UL    - Windfree?
            case 14: // LL    - Auto?
            case 15: // HH
                return FanMode::Auto;
            case 254:
                return FanMode::Off;
            case 16: // Speed
            case 17: // NaturalLow
            case 18: // NaturalMid
            case 19: // NaturalHigh
            default:
                return FanMode::Unknown;
            }
        }
        
        void process_messageset(std::string source, std::string dest, MessageSet &message, optional<std::set<uint16_t>> &custom, MessageTarget *target)
        {
            if (debug_mqtt_connected())
            {
                if (message.type == MessageSetType::Enum)
                {
                    debug_mqtt_publish("samsung_ac/nasa/enum/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                }
                else if (message.type == MessageSetType::Variable)
                {
                    debug_mqtt_publish("samsung_ac/nasa/var/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                }
                else if (message.type == MessageSetType::LongVariable)
                {
                    debug_mqtt_publish("samsung_ac/nasa/var_long/" + long_to_hex((uint16_t)message.messageNumber), std::to_string(message.value));
                }
            }
            
            if (custom && custom.value().find((uint16_t)message.messageNumber) != custom.value().end())
            {
                target->set_custom_sensor(source, (uint16_t)message.messageNumber, (float)message.value);
            }
            
            target->getValueForCustomClimate(source, (int16_t) message.messageNumber, message.value);

            switch (message.messageNumber)
            {
            case MessageNumber::VAR_in_temp_room_f: //  unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_room_f %f", source.c_str(), dest.c_str(), temp);
                target->set_room_temperature(source, temp);
                return;
            }
            case MessageNumber::VAR_in_temp_target_f: // unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                // if (value == 1) value = 'waterOutSetTemp'; //action in xml
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_target_f %f", source.c_str(), dest.c_str(), temp);
                target->set_target_temperature(source, temp);
                return;
            }
            case MessageNumber::ENUM_in_state_humidity_percent:
            {
                // XML Enum no value but in Code it adds unit
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_state_humidity_percent %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            case MessageNumber::ENUM_in_operation_power:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_power %s", source.c_str(), dest.c_str(), message.value == 0 ? "off" : "on");
                target->set_power(source, message.value != 0);
                return;
            }
            case MessageNumber::ENUM_in_operation_mode:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_mode %li", source.c_str(), dest.c_str(), message.value);
                target->set_mode(source, operation_mode_to_mode(message.value));
                return;
            }
            case MessageNumber::ENUM_in_fan_mode:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_fan_mode %li", source.c_str(), dest.c_str(), message.value);
                FanMode mode = FanMode::Unknown;
                if (message.value == 0)
                    mode = FanMode::Auto;
                else if (message.value == 1)
                    mode = FanMode::Low;
                else if (message.value == 2)
                    mode = FanMode::Mid;
                else if (message.value == 3)
                    mode = FanMode::High;
                else if (message.value == 4)
                    mode = FanMode::Turbo;
                target->set_fanmode(source, mode);
                return;
            }
            case MessageNumber::ENUM_in_fan_mode_real:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_fan_mode_real %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            case MessageNumber::ENUM_in_alt_mode:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_alt_mode %li", source.c_str(), dest.c_str(), message.value);
                target->set_altmode(source, message.value);
                return;
            }
            case MessageNumber::ENUM_in_louver_hl_swing:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_louver_hl_swing %li", source.c_str(), dest.c_str(), message.value);
                target->set_swing_vertical(source, message.value == 1);
                return;
            }
            case MessageNumber::ENUM_in_louver_lr_swing:
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_louver_lr_swing %li", source.c_str(), dest.c_str(), message.value);
                target->set_swing_horizontal(source, message.value == 1);
                return;
            }
            case MessageNumber::VAR_in_temp_water_tank_f:
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_water_tank_f %f", source.c_str(), dest.c_str(), message.value);
                return;
            }
            case MessageNumber::VAR_out_sensor_airout:
            {
                double temp = (double)((int16_t)message.value) / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_out_sensor_airout %li", source.c_str(), dest.c_str(), message.value);
                target->set_outdoor_temperature(source, temp);
                return;
            }

            default:
            {
                if ((uint16_t)message.messageNumber == 0x4065)
                {
                    // ENUM_IN_WATER_HEATER_POWER
                    ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_WATER_HEATER_POWER %s", source.c_str(), dest.c_str(), message.value == 0 ? "off" : "on");
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x4260)
                {
                    // VAR_IN_FSV_3021
                    double temp = (double)message.value / (double)10;
                    ESP_LOGW(TAG, "s:%s d:%s VAR_IN_FSV_3021 %f", source.c_str(), dest.c_str(), temp);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x4261)
                {
                    // VAR_IN_FSV_3022
                    double temp = (double)message.value / (double)10;
                    ESP_LOGW(TAG, "s:%s d:%s VAR_IN_FSV_3022 %f", source.c_str(), dest.c_str(), temp);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x4262)
                {
                    // VAR_IN_FSV_3023
                    double temp = (double)message.value / (double)10;
                    ESP_LOGW(TAG, "s:%s d:%s VAR_IN_FSV_3023 %f", source.c_str(), dest.c_str(), temp);
                    return;
                }

                if ((uint16_t)message.messageNumber == 0x8414)
                {
                    //  LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM
                    double kwh = (double)message.value / (double)1000;
                    ESP_LOGW(TAG, "s:%s d:%s LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM %fkwh", source.c_str(), dest.c_str(), kwh);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8413)
                {
                    //  LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM %f", source.c_str(), dest.c_str(), value);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8411)
                {
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s NASA_OUTDOOR_CONTROL_WATTMETER_1UNIT  %f", source.c_str(), dest.c_str(), value);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8427)
                {
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s total produced energy  %f", source.c_str(), dest.c_str(), value);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8426)
                {
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s actual produced energy %f", source.c_str(), dest.c_str(), value);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8415)
                {
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM %f", source.c_str(), dest.c_str(), value);
                    return;
                }
                if ((uint16_t)message.messageNumber == 0x8416)
                {
                    double value = (double)message.value;
                    ESP_LOGW(TAG, "s:%s d:%s NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM_ACCUM %f", source.c_str(), dest.c_str(), value);
                    return;
                }
            }
            }
        }

        DecodeResult try_decode_nasa_packet(std::vector<uint8_t> data)
        {
            return packet_.decode(data);
        }

        void process_nasa_packet(MessageTarget *target)
        {
            const auto source = packet_.sa.to_string();
            const auto dest = packet_.da.to_string();

            target->register_address(source);

            if (debug_log_packets)
            {
                ESP_LOGW(TAG, "MSG: %s", packet_.to_string().c_str());
            }

            for (auto& dn : Samsung_AC_NumberDebug::elements) {
                for (int i = 0; i < packet_.messages.size(); i++){
                    MessageSet ms = packet_.messages[i];
                    auto addr = long_to_hex((uint16_t)ms.messageNumber);
                    if (dn->targetValue == ms.value && ms.type != Structure && dn->targetValue != Samsung_AC_NumberDebug::UNUSED) {
                        if (dn->source == packet_.sa.to_string() || dn->source == ""){
                            std::string str;
                            str += "#Packet Src:" + packet_.sa.to_string() + " Dst:" + packet_.da.to_string() + " " + packet_.command.to_string() + " value " + ms.to_string() ;
                            ESP_LOGW(TAG, "\033[1;36mDebugNumber : %s", str.c_str());
                        }
                    }
                }
            }

            if (packet_.command.dataType == DataType::Ack)
            {
                for (int i = 0; i < out.size(); i++)
                {
                    if (out[i].command.packetNumber == packet_.command.packetNumber)
                    {
                        ESP_LOGW(TAG, "found %d (size=%u)", out[i].command.packetNumber, out.size());
                        out.erase(out.begin() + i);
                        if (out.size() > 50) {
                            ESP_LOGE(TAG, "Flushing OUT (size %u)", out.size());
                            out.clear();
                        }
                        break;
                    }
                }

                ESP_LOGW(TAG, "Ack %s s %d", packet_.to_string().c_str(), out.size());
                return;
            }

            if (packet_.command.dataType == DataType::Request)
            {
                ESP_LOGW(TAG, "Request %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.command.dataType == DataType::Response)
            {
                ESP_LOGW(TAG, "Response %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.command.dataType == DataType::Write)
            {
                ESP_LOGW(TAG, "Write %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.command.dataType == DataType::Nack)
            {
                ESP_LOGW(TAG, "Nack %s", packet_.to_string().c_str());
                return;
            }
            if (packet_.command.dataType == DataType::Read)
            {
                ESP_LOGW(TAG, "Read %s", packet_.to_string().c_str());
                return;
            }

            if (packet_.command.dataType != DataType::Notification)
                return;

            optional<std::set<uint16_t>> custom = target->get_custom_sensors(source);
            for (auto &message : packet_.messages)
            {
                process_messageset(source, dest, message, custom, target);
            }
        }

        void process_messageset_debug(std::string source, std::string dest, MessageSet &message, MessageTarget *target)
        {
            if (source == "20.00.00" ||
                source == "20.00.01" ||
                source == "20.00.03")
                return;
            if (((uint16_t)message.messageNumber) == 0x4003)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_VENT_POWER %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x4004)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_VENT_MODE %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x4011)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x4012)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_louver_hl_part_swing %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x4060)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_ALTERNATIVE_MODE %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x406E)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_QUIET_MODE %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x4119)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_POWER_ZONE1 %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            if (((uint16_t)message.messageNumber) == 0x411E)
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_OPERATION_POWER_ZONE2 %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            return;

            switch ((uint16_t)message.messageNumber)
            {
            case 0x4002: // ENUM_in_operation_mode_real
            {
                // Todo Map
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_operation_mode_real %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x4008: // ENUM_in_fan_vent_mode
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_in_fan_vent_mode %li", source.c_str(), dest.c_str(), message.value);
                // fan_vent_mode_to_fanmode();
                return;
            }

            case 0x4011: // ENUM_IN_LOUVER_HL_SWING
            {
                // Todo Map
                /*
               case 0:
    return 'Off';
    case 1:
    return 'On';
    default:
    return undefined;
                */
                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x4012: // ENUM_IN_LOUVER_HL_SWING
            {
                // Todo Map

                ESP_LOGW(TAG, "s:%s d:%s ENUM_IN_LOUVER_HL_SWING %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x4205: // VAR_in_temp_eva_in_f unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_eva_in_f %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x4206: // VAR_in_temp_eva_out_f unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_temp_eva_out_f %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x4211: // VAR_in_capacity_request unit = 'kW'
            {
                double temp = (double)message.value / (double)8.6;
                ESP_LOGW(TAG, "s:%s d:%s VAR_in_capacity_request %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8001: // ENUM_out_operation_odu_mode
            {
                // Todo Map
                ESP_LOGW(TAG, "s:%s d:%s ENUM_out_operation_odu_mode %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x8003: // ENUM_out_operation_heatcool
            {
                //['Undefined', 'Cool', 'Heat', 'CoolMain', 'HeatMain'];
                // Todo Map
                ESP_LOGW(TAG, "s:%s d:%s ENUM_out_operation_heatcool %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x801a: // ENUM_out_load_4way
            {
                ESP_LOGW(TAG, "s:%s d:%s ENUM_out_load_4way %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x8235: // VAR_out_error_code
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_out_error_code %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x8261: // VAR_OUT_SENSOR_PIPEIN3 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN3 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8262: // VAR_OUT_SENSOR_PIPEIN4 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN4 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8263: // VAR_OUT_SENSOR_PIPEIN5 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEIN5 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8264: // VAR_OUT_SENSOR_PIPEOUT1 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT1 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8265: // VAR_OUT_SENSOR_PIPEOUT2 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT2 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8266: // VAR_OUT_SENSOR_PIPEOUT3 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT3 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8267: // VAR_OUT_SENSOR_PIPEOUT4 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT4 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8268: // VAR_OUT_SENSOR_PIPEOUT5 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_SENSOR_PIPEOUT5 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x8274: // VAR_out_control_order_cfreq_comp2
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_out_control_order_cfreq_comp2 %li", source.c_str(), dest.c_str(), message.value);
                return;
            }
            case 0x8275: // VAR_out_control_target_cfreq_comp2
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_out_control_target_cfreq_comp2 %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x82bc: // VAR_OUT_PROJECT_CODE
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PROJECT_CODE %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x82e3: // VAR_OUT_PRODUCT_OPTION_CAPA
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PRODUCT_OPTION_CAPA %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x8280: // VAR_out_sensor_top1 unit = 'Celsius'
            {
                double temp = (double)message.value / (double)10;
                ESP_LOGW(TAG, "s:%s d:%s VAR_out_sensor_top1 %f", source.c_str(), dest.c_str(), temp);
                return;
            }

            case 0x82db: // VAR_OUT_PHASE_CURRENT
            {
                ESP_LOGW(TAG, "s:%s d:%s VAR_OUT_PHASE_CURRENT %li", source.c_str(), dest.c_str(), message.value);
                return;
            }

            case 0x402:
            case 0x409:
            case 0x40a:
            case 0x40b:
            case 0x40c:
            case 0x40d:
            case 0x40e:
            case 0x410:
            case 0x411:
            case 0x412:
            case 0x413:
            case 0x414:
            case 0x415:
            case 0x416:
            case 0x601:
            case 0x207:
            case 0x41b:
            case 0x60c:
            case 0x24fb:
            case 0x4015:
            case 0x4016:
            case 0x401b:
            case 0x4023:
            case 0x4024:
            case 0x4027:
            case 0x4028:
            case 0x402d:
            case 0x402e:
            case 0x4035:
            case 0x403e:
            case 0x403f:
            case 0x4043:
            case 0x4045:
            case 0x4046:
            case 0x4047:
            case 0x4048:
            case 0x4059:
            case 0x4060:
            case 0x4074:
            case 0x407d:
            case 0x407e:
            case 0x40ae:
            case 0x40af:
            case 0x40bc:
            case 0x40bd:
            case 0x40d5:
            case 0x410a:
            case 0x410b:
            case 0x410c:
            case 0x4111:
            case 0x4112:
            case 0x42df:
            case 0x4604:
            case 0x80af:
            case 0x8204:
            case 0x820a:
            case 0x8217:
            case 0x8218:
            case 0x821a:
            case 0x8223:
            case 0x4212:
            case 0x4222:
            case 0x4229:
            case 0x42e0:
            case 0x8229:
            case 0x822a:
            case 0x822b:
            case 0x822c:
            case 0x8233:
            case 0x8236:
            case 0x8237:
            case 0x8238:
            case 0x8239:
            case 0x823b:
            case 0x823d:
            case 0x42e3:
            case 0x42e5:
            case 0x440e:
            case 0x440f:
            case 0x4418:
            case 0x441b:
            case 0x441f:
            case 0x4420:
            case 0x4423:
            case 0x4424:
            case 0x8000:
            case 0x8002:
            case 0x800d:
            case 0x8010:
            case 0x8020:
            case 0x8030:
            case 0x8032:
            case 0x8033:
            case 0x8043:
            case 0x8045:
            case 0x8046:
            case 0x8048:
            case 0x8061:
            case 0x8066:
            case 0x8077:
            case 0x807c:
            case 0x807d:
            case 0x807e:
            case 0x8081:
            case 0x808c:
            case 0x808e:
            case 0x808f:
            case 0x809d:
            case 0x8047:
            case 0x8200:
            case 0x8201:
            case 0x8202:
            case 0x822d:
            case 0x8287:
            case 0x82a1:
            case 0x82b5:
            case 0x82b6:
            case 0x8411:
            case 0x8413:
            case 0x8414:
            case 0x8608:
            case 0x860c:
            case 0x860d:
            case 0x840a:
            case 0x8410:
            case 0x823e:
            case 0x8247:
            case 0x8249:
            case 0x824b:
            case 0x824c:
            case 0x824f:
            case 0x8254:
            case 0x825f:
            case 0x8260:
            case 0x2400:
            case 0x2401:
            case 0x24fc:
            {
                // ESP_LOGW(TAG, "s:%s d:%s Todo %s %li", source.c_str(), dest.c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                return; // Todo
            }

            case 0x8601: // STR_out_install_inverter_and_bootloader_info
            case 0x608:  // STR_ad_dbcode_micom_main
            case 0x603:  // STR_ad_option_cycle
            case 0x602:  // STR_ad_option_install_2
            case 0x600:  // STR_ad_option_basic
            case 0x202:  // VAR_ad_error_code1
            case 0x42d1: // VAR_IN_DUST_SENSOR_PM10_0_VALUE
            case 0x42d2: // VAR_IN_DUST_SENSOR_PM2_5_VALUE
            case 0x42d3: // VAR_IN_DUST_SENSOR_PM1_0_VALUE
            {
                // ESP_LOGW(TAG, "s:%s d:%s Ignore %s %li", source.c_str(), dest.c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                return; // Ingore cause not important
            }

            case 0x23:
            case 0x61d:
            case 0x400a:
            case 0x400f:
            case 0x42e1:
            case 0x42e2:
            case 0x42e4:
            case 0x22f9:
            case 0x22fa:
            case 0x22fb:
            case 0x22fc:
            case 0x22fd:
            case 0x22fe:
            case 0x22ff:
            case 0x80a7:
            case 0x80a8:
            case 0x80a9:
            case 0x80aa:
            case 0x80ab:
            case 0x80b2:
            case 0x4285:
            case 0x429d:
            case 0x826a:
            case 0x22f7:
            case 0x82da:
            case 0x82d9:
            case 0x82ee:
            case 0x82ef:
            case 0x82e6:
            case 0x82e5:
            case 0x82dd:
            case 0x4202:
            case 0x82d4:
            case 0x421c:
            case 0x8031:
            case 0x805e:
            case 0x8243:
            case 0x803f:
            case 0x808d:
            case 0x8248:
            case 0x823f:
            case 0x4204:
            case 0x4006:
            {
                // ESP_LOGW(TAG, "s:%s d:%s NoMap %s %li", source.c_str(), dest.c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                return; // message types witch have no mapping in xml
            }

            default:
                ESP_LOGW(TAG, "s:%s d:%s !! unknown %s", source.c_str(), dest.c_str(), message.to_string().c_str());
            }
            return;
        }

    } // namespace samsung_ac
} // namespace esphome




