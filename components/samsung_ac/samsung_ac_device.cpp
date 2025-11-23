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
      traits.set_visual_temperature_step(1.0f);
      traits.set_visual_min_temperature(16.0f);
      traits.set_visual_max_temperature(30.0f);

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

      traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF,
                                        climate::CLIMATE_SWING_HORIZONTAL,
                                        climate::CLIMATE_SWING_VERTICAL,
                                        climate::CLIMATE_SWING_BOTH});

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
      if (modeOpt.has_value())
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
      if (fanmodeOpt.has_value())
      {
        request.fan_mode = climatefanmode_to_fanmode(fanmodeOpt.value());
      }

      const char *customFanmode = call.get_custom_fan_mode();
      if (customFanmode != nullptr)
      {
        request.fan_mode = customfanmode_to_fanmode(std::string(customFanmode));
      }

      auto presetOpt = call.get_preset();
      if (presetOpt.has_value())
      {
        set_alt_mode_by_name(request, preset_to_altmodename(presetOpt.value()));
      }

      const char *customPreset = call.get_custom_preset();
      if (customPreset != nullptr)
      {
        set_alt_mode_by_name(request, AltModeName(customPreset));
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
  } // namespace samsung_ac
} // namespace esphome
