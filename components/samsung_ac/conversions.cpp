#include "conversions.h"

namespace esphome
{
  namespace samsung_ac
  {
    Mode str_to_mode(const std::string &value)
    {
      if (value == "Auto")
        return Mode::Auto;
      if (value == "Cool")
        return Mode::Cool;
      if (value == "Dry")
        return Mode::Dry;
      if (value == "Fan")
        return Mode::Fan;
      if (value == "Heat")
        return Mode::Heat;
      return Mode::Unknown;
    }

    std::string mode_to_str(Mode mode)
    {
      switch (mode)
      {
      case Mode::Auto:
        return "Auto";
      case Mode::Cool:
        return "Cool";
      case Mode::Dry:
        return "Dry";
      case Mode::Fan:
        return "Fan";
      case Mode::Heat:
        return "Heat";
      default:
        return "Unknown";
      };
    }
    
    WaterHeaterMode str_to_water_heater_mode(const std::string &value)
    {
      if (value == "Eco")
        return WaterHeaterMode::Eco;
      if (value == "Standard")
        return WaterHeaterMode::Standard;
      if (value == "Power")
        return WaterHeaterMode::Power;
      if (value == "Force")
        return WaterHeaterMode::Force;
      return WaterHeaterMode::Unknown;
    }

    std::string water_heater_mode_to_str(WaterHeaterMode waterheatermode)
    {
      switch (waterheatermode)
      {
      case WaterHeaterMode::Eco:
        return "Eco";
      case WaterHeaterMode::Standard:
        return "Standard";
      case WaterHeaterMode::Power:
        return "Power";
      case WaterHeaterMode::Force:
        return "Force";
      default:
        return "Unknown";
      };
    }

    optional<climate::ClimateMode> mode_to_climatemode(Mode mode)
    {
      switch (mode)
      {
      case Mode::Auto:
        return climate::ClimateMode::CLIMATE_MODE_AUTO;
      case Mode::Cool:
        return climate::ClimateMode::CLIMATE_MODE_COOL;
      case Mode::Dry:
        return climate::ClimateMode::CLIMATE_MODE_DRY;
      case Mode::Fan:
        return climate::ClimateMode::CLIMATE_MODE_FAN_ONLY;
      case Mode::Heat:
        return climate::ClimateMode::CLIMATE_MODE_HEAT;
      default:
        return nullopt;
      }
    }

    Mode climatemode_to_mode(climate::ClimateMode mode)
    {
      switch (mode)
      {
      case climate::ClimateMode::CLIMATE_MODE_COOL:
        return Mode::Cool;
      case climate::ClimateMode::CLIMATE_MODE_HEAT:
        return Mode::Heat;
      case climate::ClimateMode::CLIMATE_MODE_FAN_ONLY:
        return Mode::Fan;
      case climate::ClimateMode::CLIMATE_MODE_DRY:
        return Mode::Dry;
      case climate::ClimateMode::CLIMATE_MODE_AUTO:
        return Mode::Auto;
      default:
        return Mode::Unknown;
      }
    }

    optional<climate::ClimateFanMode> fanmode_to_climatefanmode(FanMode fanmode)
    {
      switch (fanmode)
      {
      case FanMode::Low:
        return climate::ClimateFanMode::CLIMATE_FAN_LOW;
      case FanMode::Mid:
        return climate::ClimateFanMode::CLIMATE_FAN_MEDIUM;
      case FanMode::High:
        return climate::ClimateFanMode::CLIMATE_FAN_HIGH;
      case FanMode::Turbo:
        return nullopt;
      case FanMode::Auto:
      default:
        return climate::ClimateFanMode::CLIMATE_FAN_AUTO;
      }
    }

    std::string fanmode_to_custom_climatefanmode(FanMode fanmode)
    {
      switch (fanmode)
      {
      case FanMode::Turbo:
        return "Turbo";
      default:
        return "";
      }
    }

    FanMode climatefanmode_to_fanmode(climate::ClimateFanMode fanmode)
    {
      switch (fanmode)
      {
      case climate::ClimateFanMode::CLIMATE_FAN_LOW:
        return FanMode::Low;
      case climate::ClimateFanMode::CLIMATE_FAN_MEDIUM:
        return FanMode::Mid;
      case climate::ClimateFanMode::CLIMATE_FAN_HIGH:
        return FanMode::High;
      case climate::ClimateFanMode::CLIMATE_FAN_AUTO:
      default:
        return FanMode::Auto;
      }
    }

    FanMode customfanmode_to_fanmode(const std::string &value)
    {
      if (value == "Turbo")
        return FanMode::Turbo;
      return FanMode::Auto;
    }

    AltModeName preset_to_altmodename(climate::ClimatePreset preset)
    {
      switch (preset)
      {
      case climate::ClimatePreset::CLIMATE_PRESET_ECO:
        return "Eco";
      case climate::ClimatePreset::CLIMATE_PRESET_AWAY:
        return "Away";
      case climate::ClimatePreset::CLIMATE_PRESET_BOOST:
        return "Boost";
      case climate::ClimatePreset::CLIMATE_PRESET_COMFORT:
        return "Comfort";
      case climate::ClimatePreset::CLIMATE_PRESET_HOME:
        return "Home";
      case climate::ClimatePreset::CLIMATE_PRESET_SLEEP:
        return "Sleep";
      case climate::ClimatePreset::CLIMATE_PRESET_ACTIVITY:
        return "Activity";
      case climate::ClimatePreset::CLIMATE_PRESET_NONE:
      default:
        return "None";
      }
    }

    optional<climate::ClimatePreset> altmodename_to_preset(const AltModeName& name)
    {
      if (str_equals_case_insensitive(name, "ECO"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_ECO);
      if (str_equals_case_insensitive(name, "AWAY"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_AWAY);
      if (str_equals_case_insensitive(name, "BOOST"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_BOOST);
      if (str_equals_case_insensitive(name, "COMFORT"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_COMFORT);
      if (str_equals_case_insensitive(name, "HOME"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_HOME);
      if (str_equals_case_insensitive(name, "SLEEP"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_SLEEP);
      if (str_equals_case_insensitive(name, "ACTIVITY"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_ACTIVITY);
      if (str_equals_case_insensitive(name, "NONE"))
        return optional<climate::ClimatePreset>(climate::CLIMATE_PRESET_NONE);
      return nullopt;
    }

    climate::ClimateSwingMode swingmode_to_climateswingmode(SwingMode swingMode)
    {
      switch (swingMode)
      {
      case SwingMode::Horizontal:
        return climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL;
      case SwingMode::Vertical:
        return climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL;
      case SwingMode::All:
        return climate::ClimateSwingMode::CLIMATE_SWING_BOTH;
      case SwingMode::Fix:
      default:
        return climate::ClimateSwingMode::CLIMATE_SWING_OFF;
      }
    }

    SwingMode climateswingmode_to_swingmode(climate::ClimateSwingMode swingMode)
    {
      switch (swingMode)
      {
      case climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL:
        return SwingMode::Horizontal;
      case climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL:
        return SwingMode::Vertical;
      case climate::ClimateSwingMode::CLIMATE_SWING_BOTH:
        return SwingMode::All;
      case climate::ClimateSwingMode::CLIMATE_SWING_OFF:
      default:
        return SwingMode::Fix;
      }
    }

  } // namespace samsung_ac
} // namespace esphome
