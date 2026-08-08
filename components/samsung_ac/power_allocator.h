#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esphome
{
  namespace samsung_ac
  {
    // Mutable participant state used by PowerAllocator.
    // Samsung_AC copies device cache into these, then writes results back.
    struct IndoorAllocParticipant
    {
      std::string address;
      float capacity_request_raw = 0.0f;
      bool thermo_on = false;
      // NASA 0x4222 cumulative operation time (hours); used only for reboot seed
      float operation_time_h = 0.0f;
      bool has_operation_time = false;
      // Power at the last integration point (instantaneous fallback only)
      float last_estimated_power_w = 0.0f;
      double accumulated_energy_kwh = 0.0;

      // Outputs
      float estimated_power_w = 0.0f;
      bool energy_updated = false;
    };

    // Splits outdoor instantaneous power / cumulative energy across indoor units so
    // sum(indoor) always matches the outdoor totals.
    //
    // Power: capacity request + thermo (equal fallback).
    // Energy (preferred): seed once from outdoor cumulative by operation-time share,
    //         then distribute each outdoor cumulative delta by capacity/thermo weights.
    // Energy (fallback): if outdoor cumulative never arrives, integrate allocated
    //         instantaneous power with the trapezoidal rule.
    class PowerAllocator
    {
    public:
      // 0x4211 reports 65535 when no capacity value is available
      static constexpr float INVALID_CAPACITY_RAW = 65535.0f;
      // 0x4222 uses the same sentinel for "no runtime available"
      static constexpr float INVALID_OPERATION_TIME_H = 65535.0f;
      // How long to wait for indoor runtimes before seeding without them
      static constexpr uint32_t SEED_GRACE_MS = 300000; // 5 minutes
      static constexpr uint32_t MIN_DELTA_MS = 100;
      static constexpr uint32_t MAX_DELTA_MS = 3600000; // 1 hour

      void set_outdoor_power(float power_w);
      float outdoor_power_w() const { return outdoor_power_w_; }
      bool has_outdoor_power() const { return has_outdoor_power_; }

      // Outdoor cumulative from the bus / Non-NASA tracker, in Wh
      void set_outdoor_energy_wh(float energy_wh);
      float outdoor_energy_wh() const { return outdoor_energy_wh_; }
      bool has_outdoor_energy() const { return has_outdoor_energy_; }
      bool energy_seeded() const { return energy_seeded_; }

      // Weight: capacity when thermo is on and capacity > 0; otherwise 0.
      // When every weight is 0, allocate_powers falls back to equal shares.
      static float weight_for(float capacity_request_raw, bool thermo_on);

      // Fills out_values with total * w_i / sum(w). If sum(w)==0, equal shares.
      // Guarantees sum(out_values) == total (within float remainder on last share).
      static void allocate_powers(float total, const std::vector<float> &weights,
                                  std::vector<float> &out_values);

      // Updates estimated_power_w. When outdoor cumulative is unavailable, also
      // integrates energy from allocated power (trapezoidal rule).
      void recalculate_power(std::vector<IndoorAllocParticipant> &participants, uint32_t now_ms);

      // Seeds / advances estimated energy from outdoor cumulative Wh.
      // Returns true when participant energy fields were updated.
      bool apply_outdoor_energy(std::vector<IndoorAllocParticipant> &participants, uint32_t now_ms);

      static bool has_usable_runtime(const IndoorAllocParticipant &p);

    private:
      bool all_runtimes_known_(const std::vector<IndoorAllocParticipant> &participants) const;
      void seed_energy_(std::vector<IndoorAllocParticipant> &participants, bool provisional);
      void distribute_energy_delta_(std::vector<IndoorAllocParticipant> &participants, float delta_wh);

      float outdoor_power_w_ = 0.0f;
      bool has_outdoor_power_ = false;

      float outdoor_energy_wh_ = 0.0f;
      bool has_outdoor_energy_ = false;
      float last_outdoor_energy_wh_ = 0.0f;
      bool energy_seeded_ = false;
      // Seeded before every indoor reported runtime; re-seeded once they all arrive
      bool seed_provisional_ = false;
      uint32_t first_energy_ms_ = 0;
      bool has_first_energy_ms_ = false;

      // Instantaneous-power integration fallback (used only while !has_outdoor_energy_)
      uint32_t last_power_ms_ = 0;
      bool has_previous_power_ = false;
    };

  } // namespace samsung_ac
} // namespace esphome
