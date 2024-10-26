#pragma once

#include <set>
#include <map>
#include <optional>
#include <queue>
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "samsung_ac_device.h"
#include "protocol.h"
#include "device_state_tracker.h"

namespace esphome
{
  namespace samsung_ac
  {
    class NasaProtocol;
    class Samsung_AC_Device;

    class Samsung_AC : public PollingComponent,
                       public uart::UARTDevice,
                       public MessageTarget
    {
    public:
      Samsung_AC() = default;

      float get_setup_priority() const override;
      void setup() override;
      void update() override;
      void loop() override;
      void dump_config() override;

      void set_debug_mqtt(std::string host, int port, std::string username, std::string password)
      {
        debug_mqtt_host = host;
        debug_mqtt_port = port;
        debug_mqtt_username = username;
        debug_mqtt_password = password;
      }

      void set_debug_log_messages(bool value)
      {
        debug_log_messages = value;
      }

      void set_debug_log_messages_raw(bool value)
      {
        debug_log_raw_bytes = value;
      }

      void set_non_nasa_keepalive(bool value)
      {
        non_nasa_keepalive = value;
      }
      void set_non_nasa_registration_delay_interval(int value)
      {
        non_nasa_registration_delay_interval = value;
      }
      void set_debug_log_undefined_messages(bool value)
      {
        debug_log_undefined_messages = value;
      }
      void register_device(Samsung_AC_Device *device);

      void /*MessageTarget::*/ register_address(const std::string address) override
      {
        addresses_.insert(address);
      }

      uint32_t /*MessageTarget::*/ get_miliseconds()
      {
        return millis();
      }

      void /*MessageTarget::*/ publish_data(std::vector<uint8_t> &data);

      void /*MessageTarget::*/ set_room_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_room_temperature(value);
      }

      void /*MessageTarget::*/ set_outdoor_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_outdoor_temperature(value);
      }

      void /*MessageTarget::*/ set_indoor_eva_in_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_indoor_eva_in_temperature(value);
      }

      void /*MessageTarget::*/ set_indoor_eva_out_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_indoor_eva_out_temperature(value);
      }

      void /*MessageTarget::*/ set_target_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_target_temperature(value);
      }

      void /*MessageTarget::*/ set_water_outlet_target(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_water_outlet_target(value);
      }

      void /*MessageTarget::*/ set_target_water_temperature(const std::string address, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_target_water_temperature(value);
      }

      void /*MessageTarget::*/ set_power(const std::string address, bool value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_power(value);
      }
      void /*MessageTarget::*/ set_automatic_cleaning(const std::string address, bool value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_automatic_cleaning(value);
      }
      void /*MessageTarget::*/ set_water_heater_power(const std::string address, bool value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_water_heater_power(value);
      }

      void /*MessageTarget::*/ set_mode(const std::string address, Mode mode) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_mode(mode);
      }

      void /*MessageTarget::*/ set_water_heater_mode(const std::string address, WaterHeaterMode waterheatermode) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_water_heater_mode(waterheatermode);
      }

      void /*MessageTarget::*/ set_fanmode(const std::string address, FanMode fanmode) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_fanmode(fanmode);
      }

      void /*MessageTarget::*/ set_altmode(const std::string address, AltMode altmode) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_altmode(altmode);
      }

      void /*MessageTarget::*/ set_swing_vertical(const std::string address, bool vertical) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_swing_vertical(vertical);
      }

      void /*MessageTarget::*/ set_swing_horizontal(const std::string address, bool horizontal) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_swing_horizontal(horizontal);
      }

      void /*MessageTarget::*/ set_custom_sensor(const std::string address, uint16_t message_number, float value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_custom_sensor(message_number, value);
      }

      void /*MessageTarget::*/ set_error_code(const std::string address, int value) override
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
          dev->update_error_code(value);
      }

    protected:
      Samsung_AC_Device *find_device(const std::string address)
      {
        auto it = devices_.find(address);
        if (it != devices_.end())
        {
          return it->second;
        }
        return nullptr;
      }

      std::map<std::string, Samsung_AC_Device *> devices_;
      DeviceStateTracker<Mode> state_tracker_{1000};
      std::set<std::string> addresses_;

      std::vector<uint8_t> data_;
      uint32_t last_transmission_ = 0;
      uint32_t last_protocol_update_ = 0;

      bool data_processing_init = true;

      // settings from yaml
      std::string debug_mqtt_host = "";
      uint16_t debug_mqtt_port = 1883;
      std::string debug_mqtt_username = "";
      std::string debug_mqtt_password = "";
    };

  } // namespace samsung_ac
} // namespace esphome
