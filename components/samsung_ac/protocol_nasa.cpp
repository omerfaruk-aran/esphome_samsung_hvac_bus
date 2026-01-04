#include <set>
#include "esphome/core/util.h"
#include "esphome/core/hal.h"
#include "util.h"
#include "log.h"
#include "protocol_nasa.h"
#include "debug_mqtt.h"
#include <unordered_map>

esphome::samsung_ac::Packet packet_;

namespace esphome
{
    namespace samsung_ac
    {
        struct UndefState
        {
            int32_t last_value;
            uint32_t last_log_ms;
        };

        static std::unordered_map<uint64_t, UndefState> undef_states;

        static uint64_t fnv1a64(const std::string &s)
        {
            uint64_t h = 14695981039346656037ULL;
            for (uint8_t c : s)
            {
                h ^= c;
                h *= 1099511628211ULL;
            }
            return h;
        }

        static uint64_t make_undef_key(const std::string &source, const std::string &dest, uint16_t id, MessageSetType type)
        {
            uint64_t h = fnv1a64(source);
            h ^= (fnv1a64(dest) << 1);
            h ^= (uint64_t)id * 0x9E3779B97F4A7C15ULL;
            h ^= ((uint64_t)type + 1ULL) * 0xBF58476D1CE4E5B9ULL;
            return h;
        }

        struct PacketInfo
        {
            Packet packet;
            int retry_count;
            uint32_t last_sent_time;
        };

        int variable_to_signed(int value)
        {
            if (value < 65535 /*uint16 max*/)
                return value;
            return value - (int)65535 /*uint16 max*/ - 1.0;
        }

#define LOG_MESSAGE(message_name, temp, source, dest)                                                             \
    if (debug_log_messages)                                                                                       \
    {                                                                                                             \
        if (!debug_log_messages_on_change ||                                                                      \
            log_should_print(                                                                                     \
                log_dedup_key(source, dest, (uint16_t)MessageNumber::message_name),                               \
                static_cast<double>(temp),                                                                        \
                0.0001 /* epsilon */,                                                                             \
                0 /* min_interval_ms */))                                                                         \
        {                                                                                                         \
            LOG_STATE("s:%s d:%s " #message_name " %g", source.c_str(), dest.c_str(), static_cast<double>(temp)); \
        }                                                                                                         \
    }

        static bool should_log_undefined_msg(const std::string &source, const std::string &dest, const MessageSet &message)
        {
            if (!debug_log_messages_on_change)
                return true;

            const uint16_t id = (uint16_t)message.messageNumber;

            const int32_t v = (message.type == MessageSetType::Structure)
                                  ? (int32_t)message.structure.size
                                  : (int32_t)message.value;

            const uint64_t key = make_undef_key(source, dest, id, message.type);
            const uint32_t now = millis();
            constexpr uint32_t MIN_INTERVAL_MS = 120000;

            auto it = undef_states.find(key);
            if (it == undef_states.end())
            {
                undef_states.emplace(key, UndefState{v, now});
                return true;
            }

            UndefState &st = it->second;

            if (st.last_value != v)
            {
                st.last_value = v;
                st.last_log_ms = now;
                return true;
            }

            if ((uint32_t)(now - st.last_log_ms) < MIN_INTERVAL_MS)
            {
                return false;
            }

            st.last_log_ms = now;
            return true;
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

            // NASA address format MUST be "kk.cc.aa" (e.g. "20.00.00")
            if (str.find('.') == std::string::npos)
            {
                // Avoid UB (pEnd++ on '\0') and make the misconfiguration obvious
                LOGE("NASA: invalid address format '%s' (expected 'kk.cc.aa' e.g. '20.00.00')", str.c_str());
                address.klass = AddressClass::Undefined;
                address.channel = 0;
                address.address = 0;
                return address;
            }

            char *pEnd;
            address.klass = (AddressClass)strtol(str.c_str(), &pEnd, 16);
            pEnd++; // .
            address.channel = (uint8_t)strtol(pEnd, &pEnd, 16);
            pEnd++; // .
            address.address = (uint8_t)strtol(pEnd, &pEnd, 16);
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
            sprintf(str, "%02x.%02x.%02x", (uint8_t)klass, (uint8_t)channel, (uint8_t)address);
            return std::string(str);
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
                set.size = 4;
                break;
            case LongVariable:
                set.value = (int)data[index + 2] << 24 | (int)data[index + 3] << 16 | (int)data[index + 4] << 8 | (int)data[index + 5];
                set.size = 6;
                break;

            case Structure:
                if (capacity != 1)
                {
                    LOGE("structure messages can only have one message but is %d", capacity);
                    return set;
                }
                Buffer buffer;
                set.size = data.size() - index - 3; // 3=end bytes
                buffer.size = set.size - 2;
                for (int i = 0; i < buffer.size; i++)
                {
                    buffer.data[i] = data[index + 2 + i]; // <-- FIX: payload starts after 2-byte message number
                }
                set.structure = buffer;
                break;
            default:
                LOGE("Unkown type");
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
                LOGE("Unkown type");
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
            if (_packetCounter == 0)
                _packetCounter++; // skip 0
            packet.command.packetNumber = _packetCounter++;
            return packet;
        }

        DecodeResult Packet::decode(std::vector<uint8_t> &data)
        {
            // Need at least start + 2 size bytes + something
            if (data.size() < 4)
                return {DecodeResultType::Fill};

            // Start byte check (helps resync if upper layer feeds garbage)
            if (data[0] != 0x32)
                return {DecodeResultType::Discard, 1};

            const uint16_t size = ((uint16_t)data[1] << 8) | (uint16_t)data[2];

            // NASA packets are "total_len = size + 2". Minimal practical size is 14 (for empty message list)
            // If size is insane, it's almost certainly a Non-NASA frame (where data[1] is src) or misalignment.
            if (size < 14 || size > 1500)
            {
                // IMPORTANT: consume 1 byte to resync; do NOT spam warnings by default
                // (otherwise Non-NASA frames flood logs)
                if (debug_log_undefined_messages)
                    LOGW("NASA: invalid size %u, discarding 1 byte (resync). Head=%s", size, bytes_to_hex(data).c_str());
                return {DecodeResultType::Discard, 1};
            }

            // Need the full packet in buffer
            const uint16_t total_len = (uint16_t)(size + 2);
            if (total_len > data.size())
                return {DecodeResultType::Fill};

            // Work only on the first packet bytes (buffer may contain multiple packets)
            std::vector<uint8_t> pkt(data.begin(), data.begin() + total_len);

            // End byte must be at index size+1
            if (pkt[size + 1] != 0x34)
            {
                // Misaligned start byte inside payload: discard 1 and resync
                if (debug_log_undefined_messages)
                    LOGW("NASA: invalid end byte, discarding 1 byte (resync). Pkt=%s", bytes_to_hex(pkt).c_str());
                return {DecodeResultType::Discard, 1};
            }

            // CRC check (if end byte is correct, size is likely trustworthy)
            uint16_t crc_actual = crc16(pkt, 3, size - 4);
            uint16_t crc_expected = ((uint16_t)pkt[size - 1] << 8) | (uint16_t)pkt[size];
            if (crc_expected != crc_actual)
            {
                if (debug_log_undefined_messages)
                    LOGW("NASA: invalid crc - got %u but should be %u: %s", crc_actual, crc_expected, bytes_to_hex(pkt).c_str());

                // Here size boundary is believable -> skip whole packet
                return {DecodeResultType::Discard, total_len};
            }

            unsigned int cursor = 3;

            sa.decode(pkt, cursor);
            cursor += sa.size;

            da.decode(pkt, cursor);
            cursor += da.size;

            command.decode(pkt, cursor);
            cursor += command.size;

            int capacity = (int)pkt[cursor];
            cursor++;

            messages.clear();
            for (int i = 1; i <= capacity; ++i)
            {
                MessageSet set = MessageSet::decode(pkt, cursor, capacity);
                messages.push_back(set);
                cursor += set.size;
            }

            return {DecodeResultType::Processed, total_len};
        }

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

        void NasaProtocol::protocol_update(MessageTarget *target)
        {
            for (const auto &pair : outgoing_queue_)
            {
                const std::string &address = pair.first;
                const ProtocolRequest &request = pair.second;

                Address da = Address::parse(address);
                if (da.klass == AddressClass::Undefined)
                    continue;

                Packet packet = Packet::createa_partial(da, DataType::Request);

                if (request.mode)
                {
                    MessageSet mode(MessageNumber::ENUM_in_operation_mode);
                    mode.value = (int)request.mode.value();
                    packet.messages.push_back(mode);
                }

                if (request.waterheatermode)
                {
                    MessageSet waterheatermode(MessageNumber::ENUM_in_water_heater_mode);
                    waterheatermode.value = (int)request.waterheatermode.value();
                    packet.messages.push_back(waterheatermode);
                }

                if (request.power)
                {
                    MessageSet power(MessageNumber::ENUM_in_operation_power);
                    power.value = request.power.value() ? 1 : 0;
                    packet.messages.push_back(power);
                }

                if (request.automatic_cleaning)
                {
                    MessageSet automatic_cleaning(MessageNumber::ENUM_in_operation_automatic_cleaning);
                    automatic_cleaning.value = request.automatic_cleaning.value() ? 1 : 0;
                    packet.messages.push_back(automatic_cleaning);
                }

                if (request.water_heater_power)
                {
                    MessageSet waterheaterpower(MessageNumber::ENUM_in_water_heater_power);
                    waterheaterpower.value = request.water_heater_power.value() ? 1 : 0;
                    packet.messages.push_back(waterheaterpower);
                }

                if (request.target_temp)
                {
                    MessageSet targettemp(MessageNumber::VAR_in_temp_target_f);
                    targettemp.value = request.target_temp.value() * 10.0;
                    packet.messages.push_back(targettemp);
                }

                if (request.water_outlet_target)
                {
                    MessageSet wateroutlettarget(MessageNumber::VAR_in_temp_water_outlet_target_f);
                    wateroutlettarget.value = request.water_outlet_target.value() * 10.0;
                    packet.messages.push_back(wateroutlettarget);
                }

                if (request.target_water_temp)
                {
                    MessageSet targetwatertemp(MessageNumber::VAR_in_temp_water_heater_target_f);
                    targetwatertemp.value = request.target_water_temp.value() * 10.0;
                    packet.messages.push_back(targetwatertemp);
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
                    continue;

                LOG_PACKET_SEND("Publish packet", packet);

                target->publish_data(packet.command.packetNumber, packet.encode());
            }
            outgoing_queue_.clear();
        }

        void NasaProtocol::publish_request(MessageTarget *target, const std::string &address, ProtocolRequest &request)
        {
            ProtocolRequest &queued = outgoing_queue_[address];

            if (request.mode)
            {
                request.power = true; // ensure system turns on when mode is set
                queued.mode = request.mode;
            }

            if (request.waterheatermode)
            {
                request.water_heater_power = true; // ensure system turns on when mode is set
                queued.waterheatermode = request.waterheatermode;
            }

            if (request.power)
                queued.power = request.power;

            if (request.automatic_cleaning)
                queued.automatic_cleaning = request.automatic_cleaning;

            if (request.water_heater_power)
                queued.water_heater_power = request.water_heater_power;

            if (request.target_temp)
                queued.target_temp = request.target_temp;

            if (request.water_outlet_target)
                queued.water_outlet_target = request.water_outlet_target;

            if (request.target_water_temp)
                queued.target_water_temp = request.target_water_temp;

            if (request.fan_mode)
                queued.fan_mode = request.fan_mode;

            if (request.alt_mode)
                queued.alt_mode = request.alt_mode;

            if (request.swing_mode)
                queued.swing_mode = request.swing_mode;
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

        WaterHeaterMode water_heater_mode_to_waterheatermode(int value)
        {
            switch (value)
            {
            case 0:
                return WaterHeaterMode::Eco;
            case 1:
                return WaterHeaterMode::Standard;
            case 2:
                return WaterHeaterMode::Power;
            case 3:
                return WaterHeaterMode::Force;
            default:
                return WaterHeaterMode::Unknown;
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

        void process_messageset(std::string source, std::string dest, MessageSet &message, MessageTarget *target)
        {
            if (debug_mqtt_connected())
            {
                static const std::string topic_prefix = "samsung_ac/nasa/" + source;
                std::string topic_suffix;
                std::string payload;

                if (static_cast<int>(message.messageNumber) != 0)
                {
                    topic_suffix = long_to_hex((uint16_t)message.messageNumber);
                    payload = std::to_string(message.value);
                }
                else
                {
                    topic_suffix.clear();
                    payload.clear();
                }

                switch (message.type)
                {
                case MessageSetType::Enum:
                    debug_mqtt_publish(topic_prefix + "enum/" + topic_suffix, payload);
                    break;
                case MessageSetType::Variable:
                    debug_mqtt_publish(topic_prefix + "var/" + topic_suffix, payload);
                    break;
                case MessageSetType::LongVariable:
                    debug_mqtt_publish(topic_prefix + "var_long/" + topic_suffix, payload);
                    break;
                default:
                    break;
                }
            }

            target->set_custom_sensor(source, (uint16_t)message.messageNumber, (float)message.value);

            switch (message.messageNumber)
            {
            case MessageNumber::VAR_in_temp_room_f: // unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                LOG_MESSAGE(VAR_in_temp_room_f, temp, source, dest);
                target->set_room_temperature(source, temp);
                break;
            }
            case MessageNumber::VAR_in_temp_target_f: // unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                LOG_MESSAGE(VAR_in_temp_target_f, temp, source, dest);
                target->set_target_temperature(source, temp);
                break;
            }
            case MessageNumber::VAR_in_temp_water_outlet_target_f: // unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                LOG_MESSAGE(VAR_in_temp_water_outlet_target_f, temp, source, dest);
                target->set_water_outlet_target(source, temp);
                break;
            }
            case MessageNumber::VAR_in_temp_water_heater_target_f: // unit = 'Celsius' from XML
            {
                double temp = (double)message.value / (double)10;
                LOG_MESSAGE(VAR_in_temp_water_heater_target_f, temp, source, dest);
                target->set_target_water_temperature(source, temp);
                break;
            }
            case MessageNumber::ENUM_in_state_humidity_percent:
            {
                LOG_MESSAGE(ENUM_in_state_humidity_percent, (double)message.value, source, dest);
                break;
            }
            case MessageNumber::ENUM_in_operation_power:
            {
                LOG_MESSAGE(ENUM_in_operation_power, (double)message.value, source, dest);
                target->set_power(source, message.value != 0);
                break;
            }
            case MessageNumber::ENUM_in_operation_automatic_cleaning:
            {
                LOG_MESSAGE(ENUM_in_operation_automatic_cleaning, (double)message.value, source, dest);
                target->set_automatic_cleaning(source, message.value != 0);
                break;
            }
            case MessageNumber::ENUM_in_water_heater_power:
            {
                LOG_MESSAGE(ENUM_in_water_heater_power, (double)message.value, source, dest);
                target->set_water_heater_power(source, message.value != 0);
                break;
            }
            case MessageNumber::ENUM_in_operation_mode:
            {
                LOG_MESSAGE(ENUM_in_operation_mode, (double)message.value, source, dest);
                target->set_mode(source, operation_mode_to_mode(message.value));
                break;
            }
            case MessageNumber::ENUM_in_water_heater_mode:
            {
                LOG_MESSAGE(ENUM_in_water_heater_mode, (double)message.value, source, dest);
                target->set_water_heater_mode(source, water_heater_mode_to_waterheatermode(message.value));
                break;
            }
            case MessageNumber::ENUM_in_fan_mode:
            {
                LOG_MESSAGE(ENUM_in_fan_mode, (double)message.value, source, dest);
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
                break;
            }
            case MessageNumber::ENUM_in_fan_mode_real:
            {
                LOG_MESSAGE(ENUM_in_fan_mode_real, (double)message.value, source, dest);
                break;
            }
            case MessageNumber::ENUM_in_alt_mode:
            {
                LOG_MESSAGE(ENUM_in_alt_mode, (double)message.value, source, dest);
                target->set_altmode(source, message.value);
                break;
            }
            case MessageNumber::ENUM_in_louver_hl_swing:
            {
                LOG_MESSAGE(ENUM_in_louver_hl_swing, (double)message.value, source, dest);
                target->set_swing_vertical(source, message.value == 1);
                break;
            }
            case MessageNumber::ENUM_in_louver_lr_swing:
            {
                LOG_MESSAGE(ENUM_in_louver_lr_swing, (double)message.value, source, dest);
                target->set_swing_horizontal(source, message.value == 1);
                break;
            }
            case MessageNumber::VAR_in_temp_water_tank_f:
            {
                LOG_MESSAGE(VAR_in_temp_water_tank_f, (double)message.value, source, dest);
                break;
            }
            case MessageNumber::VAR_out_sensor_airout:
            {
                double temp = (double)((int16_t)message.value) / (double)10;
                LOG_MESSAGE(VAR_out_sensor_airout, temp, source, dest);
                target->set_outdoor_temperature(source, temp);
                break;
            }
            case MessageNumber::VAR_in_temp_eva_in_f:
            {
                double temp = ((int16_t)message.value) / 10.0;
                LOG_MESSAGE(VAR_in_temp_eva_in_f, temp, source, dest);
                target->set_indoor_eva_in_temperature(source, temp);
                break;
            }
            case MessageNumber::VAR_in_temp_eva_out_f:
            {
                double temp = ((int16_t)message.value) / 10.0;
                LOG_MESSAGE(VAR_in_temp_eva_out_f, temp, source, dest);
                target->set_indoor_eva_out_temperature(source, temp);
                break;
            }
            case MessageNumber::VAR_out_error_code:
            {
                int code = static_cast<int>(message.value);
                if (debug_log_messages)
                {
                    LOG_MESSAGE(VAR_out_error_code, code, source, dest);
                }
                target->set_error_code(source, code);
                break;
            }
            case MessageNumber::ENUM_out_operation_odu_mode:
            {
                int code = static_cast<int>(message.value);
                if (debug_log_messages)
                {
                    LOG_MESSAGE(ENUM_out_operation_odu_mode, code, source, dest);
                }
                target->set_outdoor_operation_odu_mode_text_sensor(source, code);
                break;
            }

            case MessageNumber::ENUM_out_operation_heatcool:
            {
                int code = static_cast<int>(message.value);
                if (debug_log_messages)
                {
                    LOG_MESSAGE(ENUM_out_operation_heatcool, code, source, dest);
                }
                target->set_outdoor_operation_heatcool_text_sensor(source, code);
                break;
            }

            case MessageNumber::LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM:
            {
                double value = static_cast<double>(message.value);
                LOG_MESSAGE(LVAR_OUT_CONTROL_WATTMETER_1W_1MIN_SUM, value, source, dest);
                target->set_outdoor_instantaneous_power(source, value);
                break;
            }
            case MessageNumber::LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM:
            {
                double value = static_cast<double>(message.value);
                LOG_MESSAGE(LVAR_OUT_CONTROL_WATTMETER_ALL_UNIT_ACCUM, value, source, dest);
                target->set_outdoor_cumulative_energy(source, value);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_CT1:
            {
                double value = static_cast<double>(message.value);
                LOG_MESSAGE(VAR_OUT_SENSOR_CT1, value, source, dest);
                target->set_outdoor_current(source, value);
                break;
            }
            case MessageNumber::LVAR_NM_OUT_SENSOR_VOLTAGE:
            {
                double value = static_cast<double>(message.value);
                LOG_MESSAGE(LVAR_NM_OUT_SENSOR_VOLTAGE, value, source, dest);
                target->set_outdoor_voltage(source, value);
                break;
            }
            case MessageNumber::VAR_IN_FSV_3021:
            {
                double value = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_IN_FSV_3021, value, source, dest);
                break;
            }
            case MessageNumber::VAR_IN_FSV_3022:
            {
                double value = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_IN_FSV_3022, value, source, dest);
                break;
            }
            case MessageNumber::VAR_IN_FSV_3023:
            {
                double value = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_IN_FSV_3023, value, source, dest);
                break;
            }

            case MessageNumber::NASA_OUTDOOR_CONTROL_WATTMETER_1UNIT:
            {
                double value = (double)message.value;
                LOG_MESSAGE(NASA_OUTDOOR_CONTROL_WATTMETER_1UNIT, value, source, dest);
                break;
            }
            case MessageNumber::NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM:
            {
                double value = (double)message.value;
                LOG_MESSAGE(NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM, value, source, dest);
                break;
            }
            case MessageNumber::NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM_ACCUM:
            {
                double value = (double)message.value;
                LOG_MESSAGE(NASA_OUTDOOR_CONTROL_WATTMETER_TOTAL_SUM_ACCUM, value, source, dest);
                break;
            }

            case MessageNumber::total_produced_energy:
            {
                double value = (double)message.value;
                LOG_MESSAGE(total_produced_energy, value, source, dest);
                break;
            }
            case MessageNumber::actual_produced_energy:
            {
                double value = (double)message.value;
                LOG_MESSAGE(actual_produced_energy, value, source, dest);
                break;
            }
            default:
            {
                if (debug_log_undefined_messages && should_log_undefined_msg(source, dest, message))
                {
                    LOGW("Undefined s:%s d:%s %s", source.c_str(), dest.c_str(), message.to_string().c_str());
                }
                break;
            }
            }
        }

        DecodeResult try_decode_nasa_packet(std::vector<uint8_t> &data)
        {
            return packet_.decode(data);
        }

        void process_nasa_packet(MessageTarget *target)
        {
            const auto source = packet_.sa.to_string();
            const auto dest = packet_.da.to_string();
            const auto me = Address::get_my_address().to_string();

            target->register_address(source);

            if (debug_log_undefined_messages)
            {
                LOG_PACKET_RECV("MSG: %s", packet_);
            }

            if (packet_.command.dataType == DataType::Ack)
            {
                if (dest == me)
                {
                    LOG_PACKET_SEND("Ack", packet_);
                    target->ack_data(packet_.command.packetNumber);
                }
                return;
            }

            if (packet_.command.dataType == DataType::Request)
            {
                LOG_PACKET_RECV("Request %s", packet_);
                return;
            }
            if (packet_.command.dataType == DataType::Response)
            {
                LOG_PACKET_RECV("Response %s", packet_);
                return;
            }
            if (packet_.command.dataType == DataType::Write)
            {
                LOG_PACKET_RECV("Write %s", packet_);
                return;
            }
            if (packet_.command.dataType == DataType::Nack)
            {
                LOG_PACKET_RECV("Nack %s", packet_);
                return;
            }
            if (packet_.command.dataType == DataType::Read)
            {
                LOG_PACKET_RECV("Read %s", packet_);
                return;
            }
            LOG_PACKET_RECV("RECV", packet_);

            if (packet_.command.dataType != DataType::Notification)
                return;

            for (auto &message : packet_.messages)
            {
                process_messageset(source, dest, message, target);
            }
        }

        void process_messageset_debug(std::string source, std::string dest, MessageSet &message, MessageTarget *target)
        {
            if (source == "20.00.00" || source == "20.00.01" || source == "20.00.02" || source == "20.00.03")
                return;

            switch (message.messageNumber)
            {
            case MessageNumber::ENUM_IN_OPERATION_VENT_POWER:
                LOG_MESSAGE(ENUM_IN_OPERATION_VENT_POWER, message.value, source, dest);
                break;

            case MessageNumber::ENUM_IN_OPERATION_VENT_MODE:
                LOG_MESSAGE(ENUM_IN_OPERATION_VENT_MODE, message.value, source, dest);
                break;

            case MessageNumber::ENUM_in_louver_hl_swing:
                LOG_MESSAGE(ENUM_in_louver_hl_swing, message.value, source, dest);
                break;

            case MessageNumber::ENUM_in_louver_hl_part_swing:
                LOG_MESSAGE(ENUM_in_louver_hl_part_swing, message.value, source, dest);
                break;

            case MessageNumber::ENUM_in_alt_mode:
                LOG_MESSAGE(ENUM_in_alt_mode, message.value, source, dest);
                break;

            case MessageNumber::ENUM_IN_QUIET_MODE:
                LOG_MESSAGE(ENUM_IN_QUIET_MODE, message.value, source, dest);
                break;

            case MessageNumber::ENUM_IN_OPERATION_POWER_ZONE1:
                LOG_MESSAGE(ENUM_IN_OPERATION_POWER_ZONE1, message.value, source, dest);
                break;
            case MessageNumber::ENUM_IN_OPERATION_POWER_ZONE2:
                LOG_MESSAGE(ENUM_IN_OPERATION_POWER_ZONE2, message.value, source, dest);
                break;
            case MessageNumber::ENUM_in_operation_mode_real:
                LOG_MESSAGE(ENUM_in_operation_mode_real, message.value, source, dest);
                break;

            case MessageNumber::ENUM_in_fan_vent_mode:
                LOG_MESSAGE(ENUM_in_fan_vent_mode, message.value, source, dest);
                break;
            case MessageNumber::VAR_in_capacity_request: // VAR_in_capacity_request unit = 'kW'
            {
                double temp = (double)message.value / 8.6;
                LOG_MESSAGE(VAR_in_capacity_request, temp, source, dest);
                break;
            }

            case MessageNumber::ENUM_out_load_4way:
                LOG_MESSAGE(ENUM_out_load_4way, message.value, source, dest);
                break;

            case MessageNumber::VAR_OUT_SENSOR_PIPEIN3: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEIN3, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEIN4: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEIN4, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEIN5: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEIN5, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEOUT1: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEOUT1, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEOUT2: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEOUT2, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEOUT3: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEOUT3, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEOUT4: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEOUT4, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_OUT_SENSOR_PIPEOUT5: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_OUT_SENSOR_PIPEOUT5, temp, source, dest);
                break;
            }
            case MessageNumber::VAR_out_control_order_cfreq_comp2:
                LOG_MESSAGE(VAR_out_control_order_cfreq_comp2, message.value, source, dest);
                break;

            case MessageNumber::VAR_out_control_target_cfreq_comp2:
                LOG_MESSAGE(VAR_out_control_target_cfreq_comp2, message.value, source, dest);
                break;

            case MessageNumber::VAR_OUT_PROJECT_CODE:
                LOG_MESSAGE(VAR_OUT_PROJECT_CODE, message.value, source, dest);
                break;

            case MessageNumber::VAR_OUT_PRODUCT_OPTION_CAPA:
                LOG_MESSAGE(VAR_OUT_PRODUCT_OPTION_CAPA, message.value, source, dest);
                break;

            case MessageNumber::VAR_out_sensor_top1: // unit = 'Celsius'
            {
                double temp = (double)message.value / 10.0;
                LOG_MESSAGE(VAR_out_sensor_top1, temp, source, dest);
                break;
            }

            case MessageNumber::VAR_OUT_PHASE_CURRENT:
                LOG_MESSAGE(VAR_OUT_PHASE_CURRENT, message.value, source, dest);
                break;

            case MessageNumber::VAR_IN_DUST_SENSOR_PM10_0_VALUE:
                if (debug_log_messages)
                {
                    LOG_MESSAGE(VAR_IN_DUST_SENSOR_PM10_0_VALUE, (double)message.value, source, dest);
                }
                break; // Ingore cause not important

            case MessageNumber::VAR_IN_DUST_SENSOR_PM2_5_VALUE:
                if (debug_log_messages)
                {
                    LOG_MESSAGE(VAR_IN_DUST_SENSOR_PM2_5_VALUE, (double)message.value, source, dest);
                }
                break; // Ingore cause not important

            case MessageNumber::VAR_IN_DUST_SENSOR_PM1_0_VALUE:
                if (debug_log_messages)
                {
                    LOG_MESSAGE(VAR_IN_DUST_SENSOR_PM1_0_VALUE, (double)message.value, source, dest);
                }
                break; // Ingore cause not important
            default:
                break; // fallthrough to ignore list
            }

            // 2) Second: keep your huge ignore list as raw ids (no need to enum them all)
            switch ((uint16_t)message.messageNumber)
            {
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
            case 0x8601: // STR_out_install_inverter_and_bootloader_info
            case 0x608:  // STR_ad_dbcode_micom_main
            case 0x603:  // STR_ad_option_cycle
            case 0x602:  // STR_ad_option_install_2
            case 0x600:  // STR_ad_option_basic
            case 0x202:  // VAR_ad_error_code1
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
            case 0x4203:
            case 0x4006:
            {
                // LOGW("s:%s d:%s NoMap %s %li", source.c_str(), dest.c_str(), long_to_hex((int)message.messageNumber).c_str(), message.value);
                break; // message types which have no mapping in xml
            }

            default:
                if (debug_log_undefined_messages)
                {
                    LOGW("s:%s d:%s !! unknown %s", source.c_str(), dest.c_str(), message.to_string().c_str());
                }
                break;
            }
        }

    } // namespace samsung_ac
} // namespace esphome
