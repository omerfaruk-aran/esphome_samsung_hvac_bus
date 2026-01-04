#pragma once

#include <set>
#include <map>
#include <optional>
#include <queue>
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "samsung_ac_device.h"
#include "protocol.h"
#include "log.h"
#include "device_state_tracker.h"

namespace esphome
{
  namespace samsung_ac
  {
    class NasaProtocol;
    class Samsung_AC_Device;

    // time to wait since last wire activity before sending
    const uint16_t silenceInterval = 100;

    // minimum time before a retry attempt
    const uint16_t retryInterval = 500;

    // minimum number of retries, even beyond timeout
    const uint8_t minRetries = 1;

    // maximum time to wait before discarding command
    const uint16_t sendTimeout = 4000;

    struct OutgoingData
    {
      uint8_t id;
      std::vector<uint8_t> data;
      uint32_t nextRetry;
      uint32_t timeout;
      uint8_t retries;
    };

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

      template <typename SensorType, typename ValueType>
      void update_device_sensor(const std::string &address, SensorType Samsung_AC_Device::*sensor_ptr, ValueType value)
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr && dev->*sensor_ptr != nullptr)
        {
          dev->update_sensor_state(dev->*sensor_ptr, value);
        }
      }

      template <typename Func>
      void execute_if_device_exists(const std::string &address, Func func)
      {
        Samsung_AC_Device *dev = find_device(address);
        if (dev != nullptr)
        {
          func(dev);
        }
      }

      void set_flow_control_pin(GPIOPin *flow_control_pin)
      {
        this->flow_control_pin_ = flow_control_pin;
      }

      void set_debug_mqtt(std::string host, int port, std::string username, std::string password)
      {
        debug_mqtt_host = host;
        debug_mqtt_port = port;
        debug_mqtt_username = username;
        debug_mqtt_password = password;
      }

      void set_debug_log_messages_on_change(bool v) { debug_log_messages_on_change = v; }

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
      void set_debug_log_undefined_messages(bool value)
      {
        debug_log_undefined_messages = value;
      }

      void register_device(Samsung_AC_Device *device);

      void register_address(const std::string address) override
      {
        addresses_.insert(address);
      }

      uint32_t get_miliseconds()
      {
        return millis();
      }

      void publish_data(uint8_t id, std::vector<uint8_t> &&data);

      void ack_data(uint8_t id);

      void set_room_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_room_temperature(value); });
      }

      void set_outdoor_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_sensor_state(dev->outdoor_temperature, value); });
      }

      void set_indoor_eva_in_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_sensor_state(dev->indoor_eva_in_temperature, value); });
      }

      void set_indoor_eva_out_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_sensor_state(dev->indoor_eva_out_temperature, value); });
      }

      void set_target_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_target_temperature(value); });
      }

      void set_water_outlet_target(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_water_outlet_target(value); });
      }

      void set_target_water_temperature(const std::string address, float value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_target_water_temperature(value); });
      }

      void set_power(const std::string address, bool value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_power(value); });
      }
      void set_automatic_cleaning(const std::string address, bool value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_automatic_cleaning(value); });
      }

      void set_water_heater_power(const std::string address, bool value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_water_heater_power(value); });
      }

      void set_mode(const std::string address, Mode mode) override
      {
        execute_if_device_exists(address, [mode](Samsung_AC_Device *dev)
                                 { dev->update_mode(mode); });
      }

      void set_water_heater_mode(const std::string address, WaterHeaterMode waterheatermode) override
      {
        execute_if_device_exists(address, [waterheatermode](Samsung_AC_Device *dev)
                                 { dev->update_water_heater_mode(waterheatermode); });
      }

      void set_fanmode(const std::string address, FanMode fanmode) override
      {
        execute_if_device_exists(address, [fanmode](Samsung_AC_Device *dev)
                                 { dev->update_fanmode(fanmode); });
      }

      void set_altmode(const std::string address, AltMode altmode) override
      {
        execute_if_device_exists(address, [altmode](Samsung_AC_Device *dev)
                                 { dev->update_altmode(altmode); });
      }

      void set_swing_vertical(const std::string address, bool vertical) override
      {
        execute_if_device_exists(address, [vertical](Samsung_AC_Device *dev)
                                 { dev->update_swing_vertical(vertical); });
      }

      void set_swing_horizontal(const std::string address, bool horizontal) override
      {
        execute_if_device_exists(address, [horizontal](Samsung_AC_Device *dev)
                                 { dev->update_swing_horizontal(horizontal); });
      }

      void set_custom_sensor(const std::string address, uint16_t message_number, float value) override
      {
        execute_if_device_exists(address, [message_number, value](Samsung_AC_Device *dev)
                                 { dev->update_custom_sensor(message_number, value); });
      }

      void set_error_code(const std::string address, int value) override
      {
        execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                                 { dev->update_error_code(value); });
      }

      void set_outdoor_instantaneous_power(const std::string &address, float value)
      {
        update_device_sensor(address, &Samsung_AC_Device::outdoor_instantaneous_power, value);
      }

      void set_outdoor_cumulative_energy(const std::string &address, float value)
      {
        update_device_sensor(address, &Samsung_AC_Device::outdoor_cumulative_energy, value);
      }

      void set_outdoor_current(const std::string &address, float value)
      {
        update_device_sensor(address, &Samsung_AC_Device::outdoor_current, value);
      }

      void set_outdoor_voltage(const std::string &address, float value)
      {
        update_device_sensor(address, &Samsung_AC_Device::outdoor_voltage, value);
      }

      void set_outdoor_operation_odu_mode_text_sensor(const std::string &address, int value)
      {
          execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                           { dev->update_enum_text(0x8001, value); });
      }

      void set_outdoor_operation_heatcool_text_sensor(const std::string &address, int value)
      {
       execute_if_device_exists(address, [value](Samsung_AC_Device *dev)
                           { dev->update_enum_text(0x8003, value); });
      }

    protected:
      Samsung_AC_Device *find_device(const std::string &address)
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

      std::deque<OutgoingData> send_queue_;
      std::vector<uint8_t> data_;
      bool read_data();
      void before_write();
      bool write_data();
      void after_write();

      uint32_t last_transmission_ = 0;
      uint32_t last_protocol_update_ = 0;

      bool data_processing_init = true;

      // settings from yaml
      GPIOPin *flow_control_pin_{nullptr};
      std::string debug_mqtt_host = "";
      uint16_t debug_mqtt_port = 1883;
      std::string debug_mqtt_username = "";
      std::string debug_mqtt_password = "";
    };

  } // namespace samsung_ac
} // namespace esphome
