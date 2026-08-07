#include "power_allocator.h"

#include <cmath>
#include <cstdint>

namespace esphome
{
  namespace samsung_ac
  {
    void PowerAllocator::set_outdoor_power(float power_w)
    {
      if (power_w < 0.0f)
        power_w = 0.0f;
      outdoor_power_w_ = power_w;
      has_outdoor_power_ = true;
    }

    float PowerAllocator::weight_for(float capacity_request_raw, bool thermo_on)
    {
      if (!thermo_on)
        return 0.0f;
      if (capacity_request_raw <= 0.0f)
        return 0.0f;
      if (capacity_request_raw >= INVALID_CAPACITY_RAW)
        return 0.0f;
      return capacity_request_raw;
    }

    void PowerAllocator::allocate_powers(float outdoor_power_w, const std::vector<float> &weights,
                                         std::vector<float> &out_powers)
    {
      out_powers.assign(weights.size(), 0.0f);
      if (weights.empty())
        return;

      if (outdoor_power_w < 0.0f)
        outdoor_power_w = 0.0f;

      float sum_w = 0.0f;
      for (float w : weights)
      {
        if (w > 0.0f)
          sum_w += w;
      }

      if (sum_w <= 0.0f)
      {
        // Equal split among all participants so sum still equals outdoor.
        const float share = outdoor_power_w / static_cast<float>(weights.size());
        for (size_t i = 0; i < weights.size(); i++)
          out_powers[i] = share;
        return;
      }

      // Allocate proportionally; put remainder on the last positive-weight unit
      // so the sum matches outdoor_power_w exactly (float drift).
      float allocated = 0.0f;
      size_t last_positive = 0;
      for (size_t i = 0; i < weights.size(); i++)
      {
        if (weights[i] > 0.0f)
          last_positive = i;
      }

      for (size_t i = 0; i < weights.size(); i++)
      {
        if (weights[i] <= 0.0f)
        {
          out_powers[i] = 0.0f;
          continue;
        }
        if (i == last_positive)
        {
          out_powers[i] = outdoor_power_w - allocated;
        }
        else
        {
          out_powers[i] = outdoor_power_w * (weights[i] / sum_w);
          allocated += out_powers[i];
        }
      }
    }

    void PowerAllocator::recalculate(std::vector<IndoorAllocParticipant> &participants, uint32_t now_ms)
    {
      if (participants.empty() || !has_outdoor_power_)
        return;

      std::vector<float> weights;
      weights.reserve(participants.size());
      for (const auto &p : participants)
        weights.push_back(weight_for(p.capacity_request_raw, p.thermo_on));

      std::vector<float> new_powers;
      allocate_powers(outdoor_power_w_, weights, new_powers);

      const bool first_update = !has_previous_update_;
      bool do_energy = false;
      uint32_t delta_ms = 0;
      if (!first_update)
      {
        if (now_ms >= last_update_ms_)
          delta_ms = now_ms - last_update_ms_;
        else
          delta_ms = (UINT32_MAX - last_update_ms_) + now_ms + 1;

        if (delta_ms >= MIN_DELTA_MS)
        {
          if (delta_ms > MAX_DELTA_MS)
            delta_ms = MAX_DELTA_MS;
          do_energy = true;
        }
      }

      // Keep the integration anchor when the interval was too short. Capacity and
      // thermo updates can call this far more often than every MIN_DELTA_MS, and
      // moving the anchor on those calls would stall energy accumulation forever.
      const bool advance_anchor = first_update || do_energy;
      const double time_hours = static_cast<double>(delta_ms) / 3600000.0;

      for (size_t i = 0; i < participants.size(); i++)
      {
        auto &p = participants[i];
        p.energy_updated = false;
        const float new_power = new_powers[i];

        if (do_energy)
        {
          const double average_power_w =
              (static_cast<double>(p.last_estimated_power_w) + static_cast<double>(new_power)) / 2.0;
          p.accumulated_energy_kwh += (average_power_w * time_hours) / 1000.0;
          p.energy_updated = true;
        }

        p.estimated_power_w = new_power;
        if (advance_anchor)
          p.last_estimated_power_w = new_power;
      }

      if (advance_anchor)
      {
        last_update_ms_ = now_ms;
        has_previous_update_ = true;
      }
    }

  } // namespace samsung_ac
} // namespace esphome
