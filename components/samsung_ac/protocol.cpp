#include "protocol.h"
#include "util.h"
#include "log.h"
#include "protocol_nasa.h"
#include "protocol_non_nasa.h"

namespace esphome
{
    namespace samsung_ac
    {
        static bool looks_like_non_nasa_frame(const std::vector<uint8_t> &data)
        {
            if (data.size() < 14)
                return false;
            if (data[0] != 0x32)
                return false;
            if (data[13] != 0x34)
                return false;
            uint8_t crc = data[1];
            for (int i = 2; i < 12; i++)
                crc ^= data[i];
            return crc == data[12];
        }

        bool non_nasa_keepalive = false;

        ProtocolProcessing protocol_processing = ProtocolProcessing::Auto;

        uint16_t skip_data(std::vector<uint8_t> &data, int from)
        {
            // Skip over filler data or broken packets
            // Example:
            // 320037d8fedbff81cb7ffbfd808d00803f008243000082350000805e008031008248ffff801a0082d400000b6a34 f9f6f1f9f9 32000e200002
            // Note that first part is a mangled packet, then regular filler data, then start of a new packet
            // and that one new proper packet will continue with the next data read

            // find next value of 0x32, and retry with that one
            return std::find(data.begin() + from, data.end(), 0x32) - data.begin();
        }

        // This functions is designed to run after a new value was added
        // to the data vector. One by one.
        DecodeResult process_data(std::vector<uint8_t> &data, MessageTarget *target)
        {
            if (data.empty())
                return {DecodeResultType::Fill, 0};

            if (*data.begin() != 0x32)
                return {DecodeResultType::Discard, skip_data(data, 0)};

            if (protocol_processing == ProtocolProcessing::Auto)
            {
                // Strong signature check instead of heuristic on data[1]
                if (looks_like_non_nasa_frame(data))
                {
                    auto r = try_decode_non_nasa_packet(data);
                    if (r.type == DecodeResultType::Processed)
                    {
                        protocol_processing = ProtocolProcessing::NonNASA;
                        process_non_nasa_packet(target);
                        return r;
                    }
                }
            }

            DecodeResult result = {DecodeResultType::Fill, 0};

            // Check if its a decodeable NonNASA packat
            if (protocol_processing == ProtocolProcessing::Auto || protocol_processing == ProtocolProcessing::NonNASA)
            {
                result = try_decode_non_nasa_packet(data);
                if (result.type == DecodeResultType::Processed)
                {
                    // Non-NASA protocol confirmed, use for future packets
                    if (protocol_processing == ProtocolProcessing::Auto)
                    {
                        protocol_processing = ProtocolProcessing::NonNASA;
                    }

                    process_non_nasa_packet(target);
                    return result;
                }
            }
            if (protocol_processing == ProtocolProcessing::NonNASA)
            {
                if (result.type == DecodeResultType::Discard)
                {
                    return {DecodeResultType::Discard, skip_data(data, 1)};
                }
                return result;
            }

            // fallback to nasa

            result = try_decode_nasa_packet(data);
            if (result.type == DecodeResultType::Processed)
            {
                // NASA protocol confirmed, use for future packets
                if (protocol_processing == ProtocolProcessing::Auto)
                {
                    protocol_processing = ProtocolProcessing::NASA;
                }

                process_nasa_packet(target);
            }
            if (result.type == DecodeResultType::Discard)
            {
                return {DecodeResultType::Discard, skip_data(data, 1)};
            }
            return result;
        }

        bool is_nasa_address(const std::string &address)
        {
            return address.size() != 2;
        }

        AddressType get_address_type(const std::string &address)
        {
            if (address == "c8" || address.rfind("10.", 0) == 0)
                return AddressType::Outdoor;

            if (address == "00" || address == "01" || address == "02" || address == "03" || address.rfind("20.", 0) == 0)
                return AddressType::Indoor;

            return AddressType::Other;
        }

        Protocol *nasaProtocol = new NasaProtocol();
        Protocol *nonNasaProtocol = new NonNasaProtocol();

        Protocol *get_protocol(const std::string &address)
        {
            if (!is_nasa_address(address))
                return nonNasaProtocol;

            return nasaProtocol;
        }
    } // namespace samsung_ac
} // namespace esphome
