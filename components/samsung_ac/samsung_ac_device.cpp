#include "esphome/core/log.h"
#include "samsung_ac.h"
#include "util.h"
#include "conversions.h"
#include <vector>
#include <set>
#include <algorithm>

namespace esphome
{
  namespace samsung_ac
  {
    climate::ClimateTraits Samsung_AC_Climate::traits()
    {
      climate::ClimateTraits traits;

      traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

      traits.set_visual_temperature_step(1);
      traits.set_visual_min_temperature(16);
      traits.set_visual_max_temperature(30);

      traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                  climate::CLIMATE_MODE_AUTO,
                                  climate::CLIMATE_MODE_COOL,
                                  climate::CLIMATE_MODE_DRY,
                                  climate::CLIMATE_MODE_FAN_ONLY,
                                  climate::CLIMATE_MODE_HEAT});

      traits.set_supported_fan_modes({climate::CLIMATE_FAN_HIGH,
                                      climate::CLIMATE_FAN_MIDDLE,
                                      climate::CLIMATE_FAN_LOW,
                                      climate::CLIMATE_FAN_AUTO});

      {
        static const char *CUSTOM_FAN_MODES[] = {"Turbo"};
        traits.set_supported_custom_fan_modes(CUSTOM_FAN_MODES);
      }

      {
        auto supported = device->get_supported_alt_modes();
        if (!supported->empty())
        {
          std::vector<const char *> custom_presets;

          for (const AltModeDesc &mode : *supported)
          {
            auto preset = altmodename_to_preset(mode.name);
            if (preset.has_value())
            {
              traits.add_supported_preset(preset.value());
            }
            else
            {
              custom_presets.push_back(mode.name.c_str());
            }
          }

          if (!custom_presets.empty())
          {
            traits.set_supported_custom_presets(custom_presets);
          }
        }
      }

      {
        bool h = device->supports_horizontal_swing();
        bool v = device->supports_vertical_swing();

        if (h || v)
        {
          traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
          if (h)
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);
          if (v)
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
          if (h && v)
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_BOTH);
        }
      }

      return traits;
    }

    void Samsung_AC_Climate::control(const climate::ClimateCall &call)
    {
      traits();

      ProtocolRequest request;

      auto targetTempOpt = call.get_target_temperature();
      if (targetTempOpt.has_value())
        request.target_temp = targetTempOpt.value();

      auto modeOpt = call.get_mode();
      const bool mode_changed = modeOpt.has_value();

      if (mode_changed)
      {
        if (modeOpt.value() == climate::ClimateMode::CLIMATE_MODE_OFF)
        {
          request.power = false;
        }
        else
        {
          request.mode = climatemode_to_mode(modeOpt.value());
        }
      }

      auto fanmodeOpt = call.get_fan_mode();
      const char *custom_fan = call.get_custom_fan_mode();

      if (fanmodeOpt.has_value())
      {
        request.fan_mode = climatefanmode_to_fanmode(fanmodeOpt.value());
      }
      else if (custom_fan != nullptr && custom_fan[0] != '\0')
      {
        request.fan_mode = customfanmode_to_fanmode(custom_fan);
      }
      else if (mode_changed)
      {
        FanMode auto_fan = climatefanmode_to_fanmode(climate::CLIMATE_FAN_AUTO);
        request.fan_mode = auto_fan;
      }
      auto presetOpt = call.get_preset();
      if (presetOpt.has_value())
      {
        set_alt_mode_by_name(request, preset_to_altmodename(presetOpt.value()));
      }

      const char *custom_preset = call.get_custom_preset();
      if (custom_preset != nullptr && custom_preset[0] != '\0')
      {
        set_alt_mode_by_name(request, AltModeName(custom_preset));
      }

      auto swingModeOpt = call.get_swing_mode();
      if (swingModeOpt.has_value())
      {
        request.swing_mode = climateswingmode_to_swingmode(swingModeOpt.value());
      }

      device->publish_request(request);
    }

    void Samsung_AC_Climate::set_alt_mode_by_name(ProtocolRequest &request, const AltModeName &name)
    {
      auto supported = device->get_supported_alt_modes();
      auto mode = std::find_if(supported->begin(), supported->end(), [&name](const AltModeDesc &x)
                               { return x.name == name; });
      if (mode == supported->end())
      {
        ESP_LOGW(TAG, "Unsupported alt_mode %s", name.c_str());
        return;
      }
      request.alt_mode = mode->value;
    }

    void Samsung_AC_Climate::apply_fanmode_from_device(FanMode value)
    {
      auto fanmode = fanmode_to_climatefanmode(value);
      if (fanmode.has_value())
      {
        this->set_fan_mode_(*fanmode);
        this->clear_custom_fan_mode_();
      }
      else
      {
        this->clear_custom_fan_mode_();

        std::string custom = fanmode_to_custom_climatefanmode(value);
        if (!custom.empty())
        {
          this->set_custom_fan_mode_(custom.c_str());
        }
      }
    }

    void Samsung_AC_Climate::apply_altmode_from_device(const AltModeDesc &mode)
    {
      auto preset = altmodename_to_preset(mode.name);
      if (preset.has_value())
      {
        this->set_preset_(*preset);
        this->clear_custom_preset_();
      }
      else
      {
        this->clear_custom_preset_();
        this->set_custom_preset_(mode.name.c_str());
      }
    }

  } // namespace samsung_ac
} // namespace esphome
