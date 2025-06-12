#include "esphome/core/log.h"
#include "protocol.h"
#include "util.h"
#include "protocol_nasa.h"
#include "protocol_non_nasa.h"

namespace esphome
{
    namespace samsung_ac
    {
        bool debug_log_raw_bytes = false;
        bool non_nasa_keepalive = false;
        bool debug_log_undefined_messages = false;
        bool debug_log_messages = false;

        ProtocolProcessing protocol_processing = ProtocolProcessing::Auto;

        // This functions is designed to run after a new value was added
        // to the data vector. One by one.
        DataResult process_data(std::vector<uint8_t> &data, MessageTarget *target)
        {
            if (data.size() > 1500)
            {
                ESP_LOGV(TAG, "current packat exceeds the size limits: %s", bytes_to_hex(data).c_str());
                return DataResult::Clear;
            }

            // Check if its a decodeable NonNASA packat
            DecodeResult result = DecodeResult::Ok;

            if (protocol_processing == ProtocolProcessing::Auto || protocol_processing == ProtocolProcessing::NonNASA)
            {
                result = try_decode_non_nasa_packet(data);
                if (result == DecodeResult::Ok)
                {
                    if (debug_log_raw_bytes)
                    {
                        ESP_LOGW(TAG, "RAW: %s", bytes_to_hex(data).c_str());
                    }

                    // Non-NASA protocol confirmed, use for future packets
                    if (protocol_processing == ProtocolProcessing::Auto)
                    {
                        protocol_processing = ProtocolProcessing::NonNASA;
                    }

                    process_non_nasa_packet(target);
                    return DataResult::Clear;
                }
            }

            if (protocol_processing == ProtocolProcessing::Auto || protocol_processing == ProtocolProcessing::NASA)
            {
                result = try_decode_nasa_packet(data);
                if (result == DecodeResult::Ok)
                {
                    if (debug_log_raw_bytes)
                    {
                        ESP_LOGW(TAG, "RAW: %s", bytes_to_hex(data).c_str());
                    }

                    // NASA protocol confirmed, use for future packets
                    if (protocol_processing == ProtocolProcessing::Auto)
                    {
                        protocol_processing = ProtocolProcessing::NASA;
                    }

                    process_nasa_packet(target);
                    return DataResult::Clear;
                }
            }

            if (result == DecodeResult::SizeDidNotMatch || result == DecodeResult::UnexpectedSize)
            {
                return DataResult::Fill;
            }

            if (debug_log_raw_bytes)
            {
                ESP_LOGV(TAG, "RAW: %s", bytes_to_hex(data).c_str());
            }

            if (result == DecodeResult::InvalidStartByte)
            {
                ESP_LOGV(TAG, "invalid start byte: %s", bytes_to_hex(data).c_str());
            }
            else if (result == DecodeResult::InvalidEndByte)
            {
                ESP_LOGV(TAG, "invalid end byte: %s", bytes_to_hex(data).c_str());
            }
            else if (result == DecodeResult::CrcError)
            {
                // is logged within decoder
            }
            return DataResult::Clear;
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
