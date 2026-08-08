#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../components/samsung_ac/power_allocator.h"

using namespace std;
using namespace esphome::samsung_ac;

static void assert_near(float actual, float expected, float tol = 0.01f)
{
    if (fabs(actual - expected) > tol)
    {
        cout << "assert_near failed: actual=" << actual << " expected=" << expected << endl;
        assert(false);
    }
}

static void assert_near_d(double actual, double expected, double tol = 1e-6)
{
    if (fabs(actual - expected) > tol)
    {
        cout << "assert_near_d failed: actual=" << actual << " expected=" << expected << endl;
        assert(false);
    }
}

static void test_weight_for()
{
    cout << "test_weight_for" << endl;
    assert_near(PowerAllocator::weight_for(10.0f, true), 10.0f);
    assert_near(PowerAllocator::weight_for(10.0f, false), 0.0f);
    assert_near(PowerAllocator::weight_for(0.0f, true), 0.0f);
    assert_near(PowerAllocator::weight_for(-1.0f, true), 0.0f);
    assert_near(PowerAllocator::weight_for(65535.0f, true), 0.0f);
}

static void test_allocate_capacity_ratio()
{
    cout << "test_allocate_capacity_ratio" << endl;
    vector<float> weights = {2.0f, 1.0f, 0.0f};
    vector<float> powers;
    PowerAllocator::allocate_powers(3000.0f, weights, powers);
    assert_near(powers[0], 2000.0f);
    assert_near(powers[1], 1000.0f);
    assert_near(powers[2], 0.0f);
    assert_near(powers[0] + powers[1] + powers[2], 3000.0f);
}

static void test_allocate_equal_when_all_zero()
{
    cout << "test_allocate_equal_when_all_zero" << endl;
    vector<float> weights = {0.0f, 0.0f, 0.0f};
    vector<float> powers;
    PowerAllocator::allocate_powers(300.0f, weights, powers);
    assert_near(powers[0] + powers[1] + powers[2], 300.0f);
}

static void test_recalculate_power_sum()
{
    cout << "test_recalculate_power_sum" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_power(3000.0f);

    vector<IndoorAllocParticipant> parts(3);
    parts[0].capacity_request_raw = 2.0f;
    parts[0].thermo_on = true;
    parts[1].capacity_request_raw = 1.0f;
    parts[1].thermo_on = true;
    parts[2].capacity_request_raw = 9.0f;
    parts[2].thermo_on = false;

    alloc.recalculate_power(parts, 0);
    assert_near(parts[0].estimated_power_w, 2000.0f);
    assert_near(parts[1].estimated_power_w, 1000.0f);
    assert_near(parts[2].estimated_power_w, 0.0f);
    assert(!parts[0].energy_updated);
    assert_near_d(parts[0].accumulated_energy_kwh, 0.0);
}

static void test_power_integration_fallback_without_meter()
{
    cout << "test_power_integration_fallback_without_meter" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_power(1000.0f);

    vector<IndoorAllocParticipant> parts(1);
    parts[0].capacity_request_raw = 1.0f;
    parts[0].thermo_on = true;

    alloc.recalculate_power(parts, 0);
    assert(!parts[0].energy_updated);

    // Frequent capacity updates must not stall the integrator
    for (uint32_t t = 10; t <= 3600000; t += 10)
        alloc.recalculate_power(parts, t);

    // 1000W for 1 hour => 1 kWh
    assert_near(static_cast<float>(parts[0].accumulated_energy_kwh), 1.0f);
}

static void test_meter_disables_power_integration()
{
    cout << "test_meter_disables_power_integration" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_power(1000.0f);
    alloc.set_outdoor_energy_wh(5000.0f);

    vector<IndoorAllocParticipant> parts(1);
    parts[0].capacity_request_raw = 1.0f;
    parts[0].thermo_on = true;
    parts[0].operation_time_h = 1.0f;
    parts[0].has_operation_time = true;

    assert(alloc.apply_outdoor_energy(parts, 0));
    assert_near_d(parts[0].accumulated_energy_kwh, 5.0);

    // Power samples must not keep integrating once the meter owns energy
    alloc.recalculate_power(parts, 0);
    alloc.recalculate_power(parts, 3600000);
    assert(!parts[0].energy_updated);
    assert_near_d(parts[0].accumulated_energy_kwh, 5.0);
}

static void test_seed_by_operation_time()
{
    cout << "test_seed_by_operation_time" << endl;
    PowerAllocator alloc;
    // 3000 Wh outdoor cumulative
    alloc.set_outdoor_energy_wh(3000.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].operation_time_h = 20.0f;
    parts[0].has_operation_time = true;
    parts[1].operation_time_h = 10.0f;
    parts[1].has_operation_time = true;

    assert(alloc.apply_outdoor_energy(parts, 0));
    assert(alloc.energy_seeded());
    // 20:10 => 2000 Wh + 1000 Wh => 2.0 + 1.0 kWh
    assert_near_d(parts[0].accumulated_energy_kwh, 2.0);
    assert_near_d(parts[1].accumulated_energy_kwh, 1.0);
    assert_near_d(parts[0].accumulated_energy_kwh + parts[1].accumulated_energy_kwh, 3.0);
}

static void test_runtime_sentinel_is_not_a_runtime()
{
    cout << "test_runtime_sentinel_is_not_a_runtime" << endl;
    IndoorAllocParticipant p;
    p.has_operation_time = true;
    p.operation_time_h = 65535.0f;
    assert(!PowerAllocator::has_usable_runtime(p));
    p.operation_time_h = 120.0f;
    assert(PowerAllocator::has_usable_runtime(p));
}

static void test_seed_waits_within_grace_period()
{
    cout << "test_seed_waits_within_grace_period" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_energy_wh(3000.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].operation_time_h = 30.0f;
    parts[0].has_operation_time = true;
    parts[1].has_operation_time = false; // runtime not reported yet

    // Repeated outdoor samples inside the grace period must not lock in a seed
    assert(!alloc.apply_outdoor_energy(parts, 0));
    assert(!alloc.apply_outdoor_energy(parts, 1000));
    assert(!alloc.apply_outdoor_energy(parts, 60000));
    assert(!alloc.energy_seeded());

    // Late runtime still drives the seed
    parts[1].operation_time_h = 10.0f;
    parts[1].has_operation_time = true;
    assert(alloc.apply_outdoor_energy(parts, 61000));
    assert_near_d(parts[0].accumulated_energy_kwh, 2.25); // 30:10 of 3 kWh
    assert_near_d(parts[1].accumulated_energy_kwh, 0.75);
}

static void test_partial_runtime_seeds_equally()
{
    cout << "test_partial_runtime_seeds_equally" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_energy_wh(4000.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].operation_time_h = 100.0f;
    parts[0].has_operation_time = true;
    parts[1].has_operation_time = false; // never reports 0x4222

    assert(!alloc.apply_outdoor_energy(parts, 0));
    // Grace expired with only one runtime known: split equally instead of giving
    // the entire history to the single reporting unit.
    assert(alloc.apply_outdoor_energy(parts, PowerAllocator::SEED_GRACE_MS));
    assert_near_d(parts[0].accumulated_energy_kwh, 2.0);
    assert_near_d(parts[1].accumulated_energy_kwh, 2.0);

    // The silent unit still accrues its share of later outdoor deltas
    parts[1].capacity_request_raw = 1.0f;
    parts[1].thermo_on = true;
    alloc.set_outdoor_energy_wh(5000.0f);
    assert(alloc.apply_outdoor_energy(parts, PowerAllocator::SEED_GRACE_MS + 1000));
    assert_near_d(parts[1].accumulated_energy_kwh, 3.0);
    assert_near_d(parts[0].accumulated_energy_kwh + parts[1].accumulated_energy_kwh, 5.0);
}

static void test_provisional_seed_is_corrected_by_late_runtime()
{
    cout << "test_provisional_seed_is_corrected_by_late_runtime" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_energy_wh(4000.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].has_operation_time = false;
    parts[1].has_operation_time = false;

    // Grace period starts at the first outdoor energy sample
    assert(!alloc.apply_outdoor_energy(parts, 0));
    assert(!alloc.energy_seeded());

    // Grace period expired with no runtimes: seed equally so values still appear
    assert(alloc.apply_outdoor_energy(parts, PowerAllocator::SEED_GRACE_MS));
    assert(alloc.energy_seeded());
    assert_near_d(parts[0].accumulated_energy_kwh, 2.0);
    assert_near_d(parts[1].accumulated_energy_kwh, 2.0);

    // Runtimes arriving afterwards must re-seed instead of being ignored forever
    parts[0].operation_time_h = 30.0f;
    parts[0].has_operation_time = true;
    parts[1].operation_time_h = 10.0f;
    parts[1].has_operation_time = true;
    assert(alloc.apply_outdoor_energy(parts, PowerAllocator::SEED_GRACE_MS + 1000));
    assert_near_d(parts[0].accumulated_energy_kwh, 3.0);
    assert_near_d(parts[1].accumulated_energy_kwh, 1.0);
    assert_near_d(parts[0].accumulated_energy_kwh + parts[1].accumulated_energy_kwh, 4.0);

    // And only once: later calls with no outdoor change do nothing
    assert(!alloc.apply_outdoor_energy(parts, PowerAllocator::SEED_GRACE_MS + 2000));
}

static void test_delta_keeps_sum_equal_to_outdoor()
{
    cout << "test_delta_keeps_sum_equal_to_outdoor" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_energy_wh(3000.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].operation_time_h = 1.0f;
    parts[0].has_operation_time = true;
    parts[0].capacity_request_raw = 2.0f;
    parts[0].thermo_on = true;
    parts[1].operation_time_h = 1.0f;
    parts[1].has_operation_time = true;
    parts[1].capacity_request_raw = 1.0f;
    parts[1].thermo_on = true;

    assert(alloc.apply_outdoor_energy(parts, 0)); // seed 1.5 + 1.5 kWh

    // Outdoor grows by 900 Wh; distribute 2:1 by capacity/thermo
    alloc.set_outdoor_energy_wh(3900.0f);
    assert(alloc.apply_outdoor_energy(parts, 1000));
    assert_near_d(parts[0].accumulated_energy_kwh, 1.5 + 0.6); // +600 Wh
    assert_near_d(parts[1].accumulated_energy_kwh, 1.5 + 0.3); // +300 Wh
    assert_near_d(parts[0].accumulated_energy_kwh + parts[1].accumulated_energy_kwh, 3.9);
}

static void test_meter_reset_reseeds()
{
    cout << "test_meter_reset_reseeds" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_energy_wh(5000.0f);

    vector<IndoorAllocParticipant> parts(1);
    parts[0].operation_time_h = 5.0f;
    parts[0].has_operation_time = true;
    assert(alloc.apply_outdoor_energy(parts, 0));
    assert_near_d(parts[0].accumulated_energy_kwh, 5.0);

    alloc.set_outdoor_energy_wh(100.0f); // reset
    assert(alloc.apply_outdoor_energy(parts, 1000));
    assert_near_d(parts[0].accumulated_energy_kwh, 0.1);
}

int main()
{
    test_weight_for();
    test_allocate_capacity_ratio();
    test_allocate_equal_when_all_zero();
    test_recalculate_power_sum();
    test_power_integration_fallback_without_meter();
    test_meter_disables_power_integration();
    test_seed_by_operation_time();
    test_runtime_sentinel_is_not_a_runtime();
    test_seed_waits_within_grace_period();
    test_partial_runtime_seeds_equally();
    test_provisional_seed_is_corrected_by_late_runtime();
    test_delta_keeps_sum_equal_to_outdoor();
    test_meter_reset_reseeds();
    cout << "All power allocator tests passed." << endl;
    return 0;
}
