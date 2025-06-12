#pragma once

#include <optional>
#include "esphome/components/climate/climate.h"
#include "protocol.h"

namespace esphome
{
  namespace samsung_ac
  {
    Mode str_to_mode(const std::string &value);
    std::string mode_to_str(Mode mode);
    
    WaterHeaterMode str_to_water_heater_mode(const std::string &value);
    std::string water_heater_mode_to_str(WaterHeaterMode waterheatermode);

    optional<climate::ClimateMode> mode_to_climatemode(Mode mode);
    Mode climatemode_to_mode(climate::ClimateMode mode);

    optional<climate::ClimateFanMode> fanmode_to_climatefanmode(FanMode fanmode);
    std::string fanmode_to_custom_climatefanmode(FanMode fanmode);
    FanMode climatefanmode_to_fanmode(climate::ClimateFanMode fanmode);
    FanMode customfanmode_to_fanmode(const std::string &value);

    AltModeName preset_to_altmodename(climate::ClimatePreset preset);
    optional<climate::ClimatePreset> altmodename_to_preset(const AltModeName& name);

    climate::ClimateSwingMode swingmode_to_climateswingmode(SwingMode swingMode);
    SwingMode climateswingmode_to_swingmode(climate::ClimateSwingMode swingMode);
  } // namespace samsung_ac
} // namespace esphome
