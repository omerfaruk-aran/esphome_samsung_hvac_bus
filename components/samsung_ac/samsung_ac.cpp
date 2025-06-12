#include "esphome/core/log.h"
#include "samsung_ac.h"
#include "debug_mqtt.h"
#include "util.h"
#include <vector>

namespace esphome
{
  namespace samsung_ac
  {
    void Samsung_AC::setup()
    {
      if (debug_log_messages)
      {
        ESP_LOGW(TAG, "setup");
      }
      if (this->flow_control_pin_ != nullptr)
      {
        this->flow_control_pin_->setup();
      }
    }

    void Samsung_AC::update()
    {
      if (debug_log_messages)
      {
        ESP_LOGW(TAG, "update");
      }

      for (const auto &pair : devices_)
      {
        optional<Mode> current_value = pair.second->_cur_mode;
        std::string address = pair.second->address;

        if (current_value.has_value())
        {
          state_tracker_.update(address, current_value.value());
        }
      }

      debug_mqtt_connect(debug_mqtt_host, debug_mqtt_port, debug_mqtt_username, debug_mqtt_password);

      // Waiting for first update before beginning processing data
      if (data_processing_init)
      {
        ESP_LOGCONFIG(TAG, "Data Processing starting");
        data_processing_init = false;
      }

      std::string devices;
      for (const auto &pair : devices_)
      {
        if (!devices.empty())
          devices += ", ";
        devices += pair.second->address;
      }
      ESP_LOGCONFIG(TAG, "Configured devices: %s", devices.c_str());

      std::string knownIndoor, knownOutdoor, knownOther;
      for (const auto &address : addresses_)
      {
        auto &target = (get_address_type(address) == AddressType::Outdoor) ? knownOutdoor : (get_address_type(address) == AddressType::Indoor) ? knownIndoor
                                                                                                                                               : knownOther;
        if (!target.empty())
          target += ", ";
        target += address;
      }

      ESP_LOGCONFIG(TAG, "Discovered devices:");
      ESP_LOGCONFIG(TAG, "  Outdoor: %s", (knownOutdoor.length() == 0 ? "-" : knownOutdoor.c_str()));
      ESP_LOGCONFIG(TAG, "  Indoor:  %s", (knownIndoor.length() == 0 ? "-" : knownIndoor.c_str()));
      if (knownOther.length() > 0)
      {
        ESP_LOGCONFIG(TAG, "  Other:   %s", knownOther.c_str());
      }
    }

    void Samsung_AC::register_device(Samsung_AC_Device *device)
    {
      if (find_device(device->address) != nullptr)
      {
        ESP_LOGW(TAG, "There is already and device for address %s registered.", device->address.c_str());
        return;
      }

      devices_.insert({device->address, device});
    }

    void Samsung_AC::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Samsung_AC:");
      LOG_PIN("  Flow Control Pin: ", this->flow_control_pin_);
    }

    void Samsung_AC::publish_data(std::vector<uint8_t> &data)
    {
      ESP_LOGW(TAG, "write %s", bytes_to_hex(data).c_str());

      if (this->flow_control_pin_ != nullptr) {
        ESP_LOGD(TAG, "switching flow_control_pin to write");
        this->flow_control_pin_->digital_write(true);
        }

      this->write_array(data);
      this->flush();

      if (this->flow_control_pin_ != nullptr) {
        ESP_LOGD(TAG, "switching flow_control_pin to read");
        this->flow_control_pin_->digital_write(false);
        }
    }

    void Samsung_AC::loop()
    {
      if (data_processing_init)
        return;

      const uint32_t now = millis();
      if (!data_.empty() && (now - last_transmission_ >= 500))
      {
        ESP_LOGW(TAG, "Last transmission too long ago. Reset RX index.");
        data_.clear();
      }

      while (available())
      {
        last_transmission_ = now;
        uint8_t c;
        if (!read_byte(&c))
          continue;
        if (data_.empty() && c != 0x32)
          continue; // skip until start-byte found

        data_.push_back(c);

        if (process_data(data_, this) == DataResult::Clear)
        {
          data_.clear();
          break; // wait for next loop
        }
      }

      // Allow device protocols to perform recurring tasks (at most every 200ms)
      if (now - last_protocol_update_ >= 200)
      {
        last_protocol_update_ = now;
        for (const auto &pair : devices_)
        {
          Samsung_AC_Device *device = pair.second;
          device->protocol_update(this);
        }
      }
    }

    float Samsung_AC::get_setup_priority() const { return setup_priority::DATA; }
  } // namespace samsung_ac
} // namespace esphome
