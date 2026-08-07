#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esphome
{
  namespace samsung_ac
  {
    // Mutable participant state used by PowerAllocator::recalculate.
    // Samsung_AC copies device cache into these, then writes results back.
    struct IndoorAllocParticipant
    {
      std::string address;
      float capacity_request_raw = 0.0f;
      bool thermo_on = false;
      float last_estimated_power_w = 0.0f;
      double accumulated_energy_kwh = 0.0;

      // Outputs after recalculate()
      float estimated_power_w = 0.0f;
      bool energy_updated = false;
    };

    // Splits outdoor instantaneous power across indoor participants so that
    // sum(estimated_power) == outdoor_power. Cumulative energy is integrated
    // from allocated power with the trapezoidal rule.
    class PowerAllocator
    {
    public:
      static constexpr uint32_t MIN_DELTA_MS = 100;
      static constexpr uint32_t MAX_DELTA_MS = 3600000; // 1 hour

      void set_outdoor_power(float power_w);
      float outdoor_power_w() const { return outdoor_power_w_; }
      bool has_outdoor_power() const { return has_outdoor_power_; }

      // Weight: capacity when thermo is on and capacity > 0; otherwise 0.
      // When every weight is 0, allocate_powers falls back to equal shares.
      static float weight_for(float capacity_request_raw, bool thermo_on);

      // Fills out_powers with outdoor_power_w * w_i / sum(w). If sum(w)==0,
      // uses equal weights. Guarantees sum(out_powers) == outdoor_power_w
      // (within float tolerance) when outdoor_power_w >= 0 and N >= 1.
      static void allocate_powers(float outdoor_power_w, const std::vector<float> &weights,
                                  std::vector<float> &out_powers);

      // Updates estimated_power_w / energy fields on each participant in place.
      void recalculate(std::vector<IndoorAllocParticipant> &participants, uint32_t now_ms);

    private:
      float outdoor_power_w_ = 0.0f;
      bool has_outdoor_power_ = false;
      uint32_t last_update_ms_ = 0;
      bool has_previous_update_ = false;
    };

  } // namespace samsung_ac
} // namespace esphome
