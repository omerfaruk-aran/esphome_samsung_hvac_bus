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

    void PowerAllocator::set_outdoor_energy_wh(float energy_wh)
    {
      if (energy_wh < 0.0f)
        energy_wh = 0.0f;
      outdoor_energy_wh_ = energy_wh;
      has_outdoor_energy_ = true;
    }

    bool PowerAllocator::has_usable_runtime(const IndoorAllocParticipant &p)
    {
      return p.has_operation_time && p.operation_time_h < INVALID_OPERATION_TIME_H;
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

    void PowerAllocator::allocate_powers(float total, const std::vector<float> &weights,
                                         std::vector<float> &out_values)
    {
      out_values.assign(weights.size(), 0.0f);
      if (weights.empty())
        return;

      if (total < 0.0f)
        total = 0.0f;

      float sum_w = 0.0f;
      for (float w : weights)
      {
        if (w > 0.0f)
          sum_w += w;
      }

      if (sum_w <= 0.0f)
      {
        const float share = total / static_cast<float>(weights.size());
        for (size_t i = 0; i < weights.size(); i++)
          out_values[i] = share;
        return;
      }

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
          out_values[i] = 0.0f;
          continue;
        }
        if (i == last_positive)
        {
          out_values[i] = total - allocated;
        }
        else
        {
          out_values[i] = total * (weights[i] / sum_w);
          allocated += out_values[i];
        }
      }
    }

    void PowerAllocator::recalculate_power(std::vector<IndoorAllocParticipant> &participants,
                                           uint32_t now_ms)
    {
      if (participants.empty() || !has_outdoor_power_)
        return;

      std::vector<float> weights;
      weights.reserve(participants.size());
      for (const auto &p : participants)
        weights.push_back(weight_for(p.capacity_request_raw, p.thermo_on));

      std::vector<float> new_powers;
      allocate_powers(outdoor_power_w_, weights, new_powers);

      // Prefer outdoor cumulative meter for energy. Only integrate instantaneous power
      // when that meter is unavailable on this system.
      const bool integrate = !has_outdoor_energy_;
      bool do_energy = false;
      uint32_t delta_ms = 0;
      if (integrate && has_previous_power_)
      {
        if (now_ms >= last_power_ms_)
          delta_ms = now_ms - last_power_ms_;
        else
          delta_ms = (UINT32_MAX - last_power_ms_) + now_ms + 1;

        if (delta_ms >= MIN_DELTA_MS)
        {
          if (delta_ms > MAX_DELTA_MS)
            delta_ms = MAX_DELTA_MS;
          do_energy = true;
        }
      }

      // Keep the integration anchor when the interval was too short, otherwise
      // frequent capacity/thermo updates would stall accumulation.
      const bool advance_anchor = !integrate || !has_previous_power_ || do_energy;
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
        last_power_ms_ = now_ms;
        has_previous_power_ = true;
      }
    }

    bool PowerAllocator::all_runtimes_known_(const std::vector<IndoorAllocParticipant> &participants) const
    {
      for (const auto &p : participants)
      {
        if (!has_usable_runtime(p))
          return false;
      }
      return !participants.empty();
    }

    void PowerAllocator::seed_energy_(std::vector<IndoorAllocParticipant> &participants, bool provisional)
    {
      // Runtime weights are only meaningful when every unit reported one. Mixing known
      // runtimes with unknown ones would hand the whole history to the reporting units,
      // so a provisional seed splits equally until the missing runtimes show up.
      std::vector<float> weights;
      weights.reserve(participants.size());
      for (const auto &p : participants)
      {
        float w = 0.0f;
        if (!provisional && p.operation_time_h > 0.0f)
          w = p.operation_time_h;
        weights.push_back(w);
      }

      std::vector<float> shares_wh;
      allocate_powers(outdoor_energy_wh_, weights, shares_wh);

      for (size_t i = 0; i < participants.size(); i++)
      {
        participants[i].accumulated_energy_kwh = static_cast<double>(shares_wh[i]) / 1000.0;
        participants[i].energy_updated = true;
      }

      last_outdoor_energy_wh_ = outdoor_energy_wh_;
      energy_seeded_ = true;
      seed_provisional_ = provisional;
      // Meter path owns energy from here on; stop integrating power samples
      has_previous_power_ = false;
    }

    void PowerAllocator::distribute_energy_delta_(std::vector<IndoorAllocParticipant> &participants,
                                                  float delta_wh)
    {
      std::vector<float> weights;
      weights.reserve(participants.size());
      for (const auto &p : participants)
        weights.push_back(weight_for(p.capacity_request_raw, p.thermo_on));

      std::vector<float> shares_wh;
      allocate_powers(delta_wh, weights, shares_wh);

      for (size_t i = 0; i < participants.size(); i++)
      {
        participants[i].accumulated_energy_kwh += static_cast<double>(shares_wh[i]) / 1000.0;
        participants[i].energy_updated = true;
      }

      // Keep Σ indoor == outdoor exactly (float drift / partial participation)
      double sum_kwh = 0.0;
      for (const auto &p : participants)
        sum_kwh += p.accumulated_energy_kwh;
      const double target_kwh = static_cast<double>(outdoor_energy_wh_) / 1000.0;
      const double drift = target_kwh - sum_kwh;
      if (!participants.empty() && std::fabs(drift) > 1e-9)
      {
        participants.back().accumulated_energy_kwh += drift;
        participants.back().energy_updated = true;
      }

      last_outdoor_energy_wh_ = outdoor_energy_wh_;
    }

    bool PowerAllocator::apply_outdoor_energy(std::vector<IndoorAllocParticipant> &participants,
                                              uint32_t now_ms)
    {
      if (participants.empty() || !has_outdoor_energy_)
        return false;

      for (auto &p : participants)
        p.energy_updated = false;

      if (!has_first_energy_ms_)
      {
        first_energy_ms_ = now_ms;
        has_first_energy_ms_ = true;
      }

      const bool runtimes_known = all_runtimes_known_(participants);

      if (!energy_seeded_)
      {
        // Runtimes drive the seed split, so wait for them. Fall back to whatever we
        // have once the grace period expires, otherwise units that never report
        // 0x4222 would never get an energy value at all.
        if (!runtimes_known)
        {
          const uint32_t waited_ms = now_ms - first_energy_ms_; // wraparound-safe
          if (waited_ms < SEED_GRACE_MS)
            return false;
        }
        seed_energy_(participants, !runtimes_known);
        return true;
      }

      // A provisional seed used incomplete runtimes; redo it once they all arrive so
      // the historical split reflects actual operation time.
      if (seed_provisional_ && runtimes_known)
      {
        seed_energy_(participants, false);
        return true;
      }

      const float delta_wh = outdoor_energy_wh_ - last_outdoor_energy_wh_;
      if (delta_wh < 0.0f)
      {
        // Outdoor meter reset / wrap: re-seed from the new absolute reading
        seed_energy_(participants, seed_provisional_);
        return true;
      }
      if (delta_wh == 0.0f)
        return false;

      distribute_energy_delta_(participants, delta_wh);
      return true;
    }

  } // namespace samsung_ac
} // namespace esphome
