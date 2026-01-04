#pragma once

#include <set>
#include <optional>
#include <algorithm>
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/climate/climate.h"
#include "protocol.h"
#include "samsung_ac.h"
#include "conversions.h"

namespace esphome
{
  namespace samsung_ac
  {
    class Samsung_AC;
    class Samsung_AC_Device;

    class Samsung_AC_Climate : public climate::Climate
    {
    public:
      climate::ClimateTraits traits();
      void control(const climate::ClimateCall &call);
      void apply_fanmode_from_device(FanMode value);
      void apply_altmode_from_device(const AltModeDesc &mode);

      void set_map_auto_to_heat_cool(bool value) { map_auto_to_heat_cool_ = value; }
      bool get_map_auto_to_heat_cool() const { return map_auto_to_heat_cool_; }

      Samsung_AC_Device *device;

    protected:
      void set_alt_mode_by_name(ProtocolRequest &request, const AltModeName &name);
      bool map_auto_to_heat_cool_{false};
    };

    class Samsung_AC_Number : public number::Number
    {
    public:
      void control(float value) override
      {
        write_state_(value);
      }
      std::function<void(float)> write_state_;
    };

    class Samsung_AC_Mode_Select : public select::Select
    {
    public:
      void publish_state_(Mode mode)
      {
        this->publish_state(mode_to_str(mode));
      }

      void control(const std::string &value) override
      {
        write_state_(str_to_mode(value));
      }

      std::function<void(Mode)> write_state_;
    };

    class Samsung_AC_Water_Heater_Mode_Select : public select::Select
    {
    public:
      void publish_state_(WaterHeaterMode waterheatermode)
      {
        this->publish_state(water_heater_mode_to_str(waterheatermode));
      }

      void control(const std::string &value) override
      {
        write_state_(str_to_water_heater_mode(value));
      }

      std::function<void(WaterHeaterMode)> write_state_;
    };

    class Samsung_AC_Switch : public switch_::Switch
    {
    public:
      std::function<void(bool)> write_state_;

    protected:
      void write_state(bool state) override
      {
        this->publish_state(state);
        write_state_(state);
      }
    };

    struct Samsung_AC_Sensor
    {
      uint16_t message_number;
      sensor::Sensor *sensor;
    };

    class Samsung_AC_Device
    {
    public:
      Samsung_AC_Device(const std::string &address, MessageTarget *target)
      {
        this->address = address;
        this->target = target;
        this->protocol = get_protocol(address);
      }

      void set_map_auto_to_heat_cool(bool value)
      {
        map_auto_to_heat_cool_ = value;
        if (climate != nullptr)
          climate->set_map_auto_to_heat_cool(value);
      }

      std::string address;
      sensor::Sensor *room_temperature{nullptr};
      sensor::Sensor *outdoor_temperature{nullptr};
      sensor::Sensor *indoor_eva_in_temperature{nullptr};
      sensor::Sensor *indoor_eva_out_temperature{nullptr};
      sensor::Sensor *error_code{nullptr};
      sensor::Sensor *outdoor_instantaneous_power{nullptr};
      sensor::Sensor *outdoor_cumulative_energy{nullptr};
      sensor::Sensor *outdoor_current{nullptr};
      sensor::Sensor *outdoor_voltage{nullptr};
      text_sensor::TextSensor *outdoor_operation_odu_mode_text{nullptr};
      text_sensor::TextSensor *outdoor_operation_heatcool_text{nullptr};

      Samsung_AC_Number *target_temperature{nullptr};
      Samsung_AC_Number *water_outlet_target{nullptr};
      Samsung_AC_Number *target_water_temperature{nullptr};
      Samsung_AC_Switch *power{nullptr};
      Samsung_AC_Switch *automatic_cleaning{nullptr};
      Samsung_AC_Switch *water_heater_power{nullptr};
      Samsung_AC_Mode_Select *mode{nullptr};
      Samsung_AC_Water_Heater_Mode_Select *waterheatermode{nullptr};
      Samsung_AC_Climate *climate{nullptr};
      std::map<uint16_t, sensor::Sensor *> custom_sensor_map;
      float room_temperature_offset{0};

      template <typename SwingType>
      void update_swing(SwingType &swing_variable, uint8_t mask, bool value)
      {
        swing_variable = combine(swing_variable, mask, value);
        climate->publish_state();
      }

      void update_sensor_state(sensor::Sensor *target_sensor, float value)
      {
        if (target_sensor != nullptr)
        {
          target_sensor->publish_state(value);
        }
      }

      void set_error_code_sensor(sensor::Sensor *sensor)
      {
        error_code = sensor;
      }

      void update_error_code(int value)
      {
        if (error_code != nullptr)
          error_code->publish_state(value);
      }

      void set_outdoor_instantaneous_power_sensor(sensor::Sensor *sensor)
      {
        outdoor_instantaneous_power = sensor;
      }

      void set_outdoor_cumulative_energy_sensor(sensor::Sensor *sensor)
      {
        outdoor_cumulative_energy = sensor;
      }

      void set_outdoor_current_sensor(sensor::Sensor *sensor)
      {
        outdoor_current = sensor;
      }

      void set_outdoor_voltage_sensor(sensor::Sensor *sensor)
      {
        outdoor_voltage = sensor;
      }

      void set_outdoor_temperature_sensor(sensor::Sensor *sensor)
      {
        outdoor_temperature = sensor;
      }

      void set_indoor_eva_in_temperature_sensor(sensor::Sensor *sensor)
      {
        indoor_eva_in_temperature = sensor;
      }

      void set_indoor_eva_out_temperature_sensor(sensor::Sensor *sensor)
      {
        indoor_eva_out_temperature = sensor;
      }

      void update_custom_sensor(uint16_t message_number, float value)
      {
        auto it = custom_sensor_map.find(message_number);
        if (it != custom_sensor_map.end())
        {
          it->second->publish_state(value);
        }
      }

      void set_room_temperature_sensor(sensor::Sensor *sensor)
      {
        room_temperature = sensor;
      }

      void update_room_temperature(float value)
      {
        if (room_temperature != nullptr)
          room_temperature->publish_state(value + room_temperature_offset);
        if (climate != nullptr)
        {
          climate->current_temperature = value + room_temperature_offset;
          climate->publish_state();
        }
      }

      void add_custom_sensor(int message_number, sensor::Sensor *sensor)
      {
        custom_sensor_map[(uint16_t)message_number] = sensor;
      }

      void set_power_switch(Samsung_AC_Switch *switch_)
      {
        power = switch_;
        power->write_state_ = [this](bool value)
        {
          ProtocolRequest request;
          request.power = value;
          publish_request(request);
        };
      }

      void set_automatic_cleaning_switch(Samsung_AC_Switch *switch_)
      {
        automatic_cleaning = switch_;
        automatic_cleaning->write_state_ = [this](bool value)
        {
          ProtocolRequest request;
          request.automatic_cleaning = value;
          publish_request(request);
        };
      }

      void set_water_heater_power_switch(Samsung_AC_Switch *switch_)
      {
        water_heater_power = switch_;
        water_heater_power->write_state_ = [this](bool value)
        {
          ProtocolRequest request;
          request.water_heater_power = value;
          publish_request(request);
        };
      }

      void set_mode_select(Samsung_AC_Mode_Select *select)
      {
        mode = select;
        mode->write_state_ = [this](Mode value)
        {
          ProtocolRequest request;
          request.mode = value;
          publish_request(request);
        };
      }

      void set_water_heater_mode_select(Samsung_AC_Water_Heater_Mode_Select *select)
      {
        waterheatermode = select;
        waterheatermode->write_state_ = [this](WaterHeaterMode value)
        {
          ProtocolRequest request;
          request.waterheatermode = value;
          publish_request(request);
        };
      }

      void set_target_temperature_number(Samsung_AC_Number *number)
      {
        target_temperature = number;
        target_temperature->write_state_ = [this](float value)
        {
          ProtocolRequest request;
          request.target_temp = value;
          publish_request(request);
        };
      };

      void set_water_outlet_target_number(Samsung_AC_Number *number)
      {
        water_outlet_target = number;
        water_outlet_target->write_state_ = [this](float value)
        {
          ProtocolRequest request;
          request.water_outlet_target = value;
          publish_request(request);
        };
      };

      void set_target_water_temperature_number(Samsung_AC_Number *number)
      {
        target_water_temperature = number;
        target_water_temperature->write_state_ = [this](float value)
        {
          ProtocolRequest request;
          request.target_water_temp = value;
          publish_request(request);
        };
      };

      void set_climate(Samsung_AC_Climate *value)
      {
        climate = value;
        climate->device = this;
        climate->set_map_auto_to_heat_cool(map_auto_to_heat_cool_);
      }

      void update_target_temperature(float value)
      {
        if (target_temperature != nullptr)
          target_temperature->publish_state(value);
        if (climate != nullptr)
        {
          climate->target_temperature = value;
          climate->publish_state();
        }
      }

      void update_water_outlet_target(float value)
      {
        if (water_outlet_target != nullptr)
          water_outlet_target->publish_state(value);
      }

      void update_target_water_temperature(float value)
      {
        if (target_water_temperature != nullptr)
          target_water_temperature->publish_state(value);
      }

      optional<bool> _cur_power;
      optional<bool> _cur_automatic_cleaning;
      optional<bool> _cur_water_heater_power;
      optional<Mode> _cur_mode;
      optional<WaterHeaterMode> _cur_water_heater_mode;

      void update_power(bool value)
      {
        _cur_power = value;
        if (power != nullptr)
          power->publish_state(value);
        if (climate != nullptr)
          calc_and_publish_mode();
      }

      void update_automatic_cleaning(bool value)
      {
        _cur_automatic_cleaning = value;
        if (automatic_cleaning != nullptr)
          automatic_cleaning->publish_state(value);
        if (climate != nullptr)
          calc_and_publish_mode();
      }

      void update_water_heater_power(bool value)
      {
        _cur_water_heater_power = value;
        if (water_heater_power != nullptr)
          water_heater_power->publish_state(value);
      }

      void update_mode(Mode value)
      {
        _cur_mode = value;
        if (mode != nullptr)
          mode->publish_state_(value);
        if (climate != nullptr)
          calc_and_publish_mode();
      }

      void update_water_heater_mode(WaterHeaterMode value)
      {
        _cur_water_heater_mode = value;
        if (waterheatermode != nullptr)
          waterheatermode->publish_state_(value);
      }

      void update_fanmode(FanMode value)
      {
        if (climate != nullptr)
        {
          climate->apply_fanmode_from_device(value);
          climate->publish_state();
        }
      }

      void update_altmode(AltMode value)
      {
        if (climate != nullptr)
        {
          auto supported = get_supported_alt_modes();
          auto mode = std::find_if(supported->begin(), supported->end(), [&value](const AltModeDesc &x)
                                   { return x.value == value; });
          if (mode == supported->end())
          {
            ESP_LOGW(TAG, "Unsupported alt_mode %d", value);
            return;
          }

          climate->apply_altmode_from_device(*mode);

          climate->publish_state();
        }
      }

      void update_swing_vertical(bool value)
      {
        if (climate != nullptr)
        {
          update_swing(climate->swing_mode, 1, value);
        }
      }

      void update_swing_horizontal(bool value)
      {
        if (climate != nullptr)
        {
          update_swing(climate->swing_mode, 2, value);
        }
      }

      void publish_request(ProtocolRequest &request)
      {
        protocol->publish_request(target, address, request);
      }

      bool supports_fan_modes()
      {
        return supports_fan_modes_;
      }

      bool supports_horizontal_swing()
      {
        return supports_horizontal_swing_;
      }

      bool supports_vertical_swing()
      {
        return supports_vertical_swing_;
      }

      void set_supports_fan_modes(bool value)
      {
        supports_fan_modes_ = value;
      }

      void set_supports_horizontal_swing(bool value)
      {
        supports_horizontal_swing_ = value;
      }

      void set_supports_vertical_swing(bool value)
      {
        supports_vertical_swing_ = value;
      }

      void add_alt_mode(const AltModeName &name, AltMode value)
      {
        AltModeDesc desc;
        desc.name = name;
        desc.value = value;
        alt_modes.push_back(std::move(desc));
      }

      const std::vector<AltModeDesc> *get_supported_alt_modes()
      {
        return &alt_modes;
      }

      void set_room_temperature_offset(float value)
      {
        room_temperature_offset = value;
      }

      void protocol_update(MessageTarget *target)
      {
        if (protocol != nullptr)
        {
          protocol->protocol_update(target);
        }
      }
      void set_outdoor_operation_odu_mode_text_sensor(text_sensor::TextSensor *sensor)
      {
        outdoor_operation_odu_mode_text = sensor;
      }

      void set_outdoor_operation_heatcool_text_sensor(text_sensor::TextSensor *sensor)
      {
        outdoor_operation_heatcool_text = sensor;
      }
      void update_enum_text(uint16_t message_number, int value)
      {
        // 0x8003 -> ENUM_out_operation_heatcool
        if (message_number == 0x8003)
        {
          if (outdoor_operation_heatcool_text != nullptr)
          {
            std::string s;
            switch (value)
            {
            case 0:
              s = "Undefined";
              break;
            case 1:
              s = "Cool";
              break;
            case 2:
              s = "Heat";
              break;
            case 3:
              s = "CoolMain";
              break;
            case 4:
              s = "HeatMain";
              break;
            default:
              s = std::string("Unknown(") + std::to_string(value) + ")";
              break;
            }
            outdoor_operation_heatcool_text->publish_state(s);
          }
          return;
        }

        // 0x8001 -> ENUM_out_operation_odu_mode
        if (message_number == 0x8001)
        {
          if (outdoor_operation_odu_mode_text != nullptr)
          {
            std::string s;
            switch (value)
            {
            case 0:
              s = "OP_STOP";
              break;
            case 1:
              s = "OP_SAFETY";
              break;
            case 2:
              s = "OP_NORMAL";
              break;
            case 3:
              s = "OP_BALANCE";
              break;
            case 4:
              s = "OP_RECOVERY";
              break;
            case 5:
              s = "OP_DEICE";
              break;
            case 6:
              s = "OP_COMPDOWN";
              break;
            case 7:
              s = "OP_PROHIBIT";
              break;
            case 8:
              s = "OP_LINEJIG";
              break;
            case 9:
              s = "OP_PCBJIG";
              break;
            case 10:
              s = "OP_TEST";
              break;
            case 11:
              s = "OP_CHARGE";
              break;
            case 12:
              s = "OP_PUMPDOWN";
              break;
            case 13:
              s = "OP_PUMPOUT";
              break;
            case 14:
              s = "OP_VACCUM";
              break;
            case 15:
              s = "OP_CALORYJIG";
              break;
            case 16:
              s = "OP_PUMPDOWNSTOP";
              break;
            case 17:
              s = "OP_SUBSTOP";
              break;
            case 18:
              s = "OP_CHECKPIPE";
              break;
            case 19:
              s = "OP_CHECKREF";
              break;
            case 20:
              s = "OP_FPTJIG";
              break;
            case 21:
              s = "OP_NONSTOP_HEAT_COOL_CHANGE";
              break;
            case 22:
              s = "OP_AUTO_INSPECT";
              break;
            case 23:
              s = "OP_ELECTRIC_DISCHARGE";
              break;
            case 24:
              s = "OP_SPLIT_DEICE";
              break;
            case 25:
              s = "OP_INVETER_CHECK";
              break;
            case 26:
              s = "OP_NONSTOP_DEICE";
              break;
            case 27:
              s = "OP_REM_TEST";
              break;
            case 28:
              s = "OP_RATING";
              break;
            case 29:
              s = "OP_PC_TEST";
              break;
            case 30:
              s = "OP_PUMPDOWN_THERMOOFF";
              break;
            case 31:
              s = "OP_3PHASE_TEST";
              break;
            case 32:
              s = "OP_SMARTINSTALL_TEST";
              break;
            case 33:
              s = "OP_DEICE_PERFORMANCE_TEST";
              break;
            case 34:
              s = "OP_INVERTER_FAN_PBA_CHECK";
              break;
            case 35:
              s = "OP_AUTO_PIPE_PAIRING";
              break;
            case 36:
              s = "OP_AUTO_CHARGE";
              break;
            default:
              s = std::string("Unknown(") + std::to_string(value) + ")";
              break;
            }
            outdoor_operation_odu_mode_text->publish_state(s);
          }
          return;
        }
      }

    protected:
      bool supports_fan_modes_{true};
      bool map_auto_to_heat_cool_{false};

      bool supports_horizontal_swing_{false};
      bool supports_vertical_swing_{false};
      std::vector<AltModeDesc> alt_modes;

      Protocol *protocol{nullptr};
      MessageTarget *target{nullptr};

      climate::ClimateSwingMode combine(climate::ClimateSwingMode climateSwingMode, uint8_t mask, bool value)
      {
        uint8_t swingMode = static_cast<uint8_t>(climateswingmode_to_swingmode(climateSwingMode));
        return swingmode_to_climateswingmode(static_cast<SwingMode>(value ? (swingMode | mask) : (swingMode & ~mask)));
      }

      void calc_and_publish_mode()
      {
        if (!_cur_power.has_value())
          return;
        if (!_cur_mode.has_value())
          return;

        climate->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
        if (_cur_power.value() == true)
        {
          auto opt = mode_to_climatemode(_cur_mode.value());
          if (opt.has_value())
          {
            if (climate->get_map_auto_to_heat_cool() && _cur_mode.value() == Mode::Auto)
              climate->mode = climate::ClimateMode::CLIMATE_MODE_HEAT_COOL;
            else
              climate->mode = opt.value();
          }
        }

        climate->publish_state();
      }
    };
  } // namespace samsung_ac
} // namespace esphome
