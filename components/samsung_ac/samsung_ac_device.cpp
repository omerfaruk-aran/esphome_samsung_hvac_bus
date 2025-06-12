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
      auto traits = climate::ClimateTraits();

      traits.set_supports_current_temperature(true);

      traits.set_visual_temperature_step(1);
      traits.set_visual_min_temperature(16);
      traits.set_visual_max_temperature(30);

      const std::set<climate::ClimateMode> modes = {
          climate::CLIMATE_MODE_OFF,
          climate::CLIMATE_MODE_AUTO,
          climate::CLIMATE_MODE_COOL,
          climate::CLIMATE_MODE_DRY,
          climate::CLIMATE_MODE_FAN_ONLY,
          climate::CLIMATE_MODE_HEAT};
      traits.set_supported_modes(modes);

      std::set<climate::ClimateFanMode> fan = {
          climate::ClimateFanMode::CLIMATE_FAN_HIGH,
          climate::ClimateFanMode::CLIMATE_FAN_MIDDLE,
          climate::ClimateFanMode::CLIMATE_FAN_LOW};

      if (this->mode != climate::CLIMATE_MODE_FAN_ONLY)
      {
        fan.insert(climate::ClimateFanMode::CLIMATE_FAN_AUTO);
      }

      if (is_nasa_address(device->address))
      {
        // fan.insert(climate::ClimateFanMode::CLIMATE_FAN_DIFFUSE);
      }

      traits.set_supported_fan_modes(fan);

      std::set<std::string> customFan;
      customFan.insert("Turbo");
      traits.set_supported_custom_fan_modes(customFan);

      auto supported = device->get_supported_alt_modes();
      if (!supported->empty())
      {
        std::set<climate::ClimatePreset> presets;
        std::set<std::string> custom_presets;
        for (const AltModeDesc &mode : *supported)
        {
          auto preset = altmodename_to_preset(mode.name);
          if (preset)
            presets.insert(preset.value());
          else
            custom_presets.insert(mode.name);
        };
        traits.set_supported_presets(presets);
        traits.set_supported_custom_presets(custom_presets);
      }

      bool h = device->supports_horizontal_swing();
      bool v = device->supports_vertical_swing();
      if (h || v)
      {
        std::set<climate::ClimateSwingMode> swingMode;
        swingMode.insert(climate::ClimateSwingMode::CLIMATE_SWING_OFF);
        if (h)
          swingMode.insert(climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL);
        if (v)
          swingMode.insert(climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL);
        if (h && v)
          swingMode.insert(climate::ClimateSwingMode::CLIMATE_SWING_BOTH);
        traits.set_supported_swing_modes(swingMode);
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

      auto customFanmodeOpt = call.get_custom_fan_mode();
      if (customFanmodeOpt.has_value())
      {
        request.fan_mode = customfanmode_to_fanmode(customFanmodeOpt.value());
      }

      auto presetOpt = call.get_preset();
      if (presetOpt.has_value())
      {
        set_alt_mode_by_name(request, preset_to_altmodename(presetOpt.value()));
      }

      auto customPresetOpt = call.get_custom_preset();
      if (customPresetOpt.has_value())
      {
        set_alt_mode_by_name(request, customPresetOpt.value());
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
