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

static void test_weight_for()
{
    cout << "test_weight_for" << endl;
    assert_near(PowerAllocator::weight_for(10.0f, true), 10.0f);
    assert_near(PowerAllocator::weight_for(10.0f, false), 0.0f);
    assert_near(PowerAllocator::weight_for(0.0f, true), 0.0f);
    assert_near(PowerAllocator::weight_for(-1.0f, true), 0.0f);
}

static void test_allocate_capacity_ratio()
{
    cout << "test_allocate_capacity_ratio" << endl;
    // weights 2:1:0 -> powers 2000:1000:0 for 3000W outdoor
    vector<float> weights = {2.0f, 1.0f, 0.0f};
    vector<float> powers;
    PowerAllocator::allocate_powers(3000.0f, weights, powers);
    assert(powers.size() == 3);
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
    assert_near(powers[0], 100.0f);
    assert_near(powers[1], 100.0f);
    assert_near(powers[2], 100.0f);
    assert_near(powers[0] + powers[1] + powers[2], 300.0f);
}

static void test_allocate_single()
{
    cout << "test_allocate_single" << endl;
    vector<float> weights = {5.0f};
    vector<float> powers;
    PowerAllocator::allocate_powers(1234.5f, weights, powers);
    assert_near(powers[0], 1234.5f);
}

static void test_recalculate_sum_and_energy()
{
    cout << "test_recalculate_sum_and_energy" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_power(3000.0f);

    vector<IndoorAllocParticipant> parts(3);
    parts[0].address = "20.00.00";
    parts[0].capacity_request_raw = 2.0f;
    parts[0].thermo_on = true;
    parts[1].address = "20.00.01";
    parts[1].capacity_request_raw = 1.0f;
    parts[1].thermo_on = true;
    parts[2].address = "20.00.02";
    parts[2].capacity_request_raw = 9.0f;
    parts[2].thermo_on = false; // excluded from weighted share

    // First update: powers only, no energy
    alloc.recalculate(parts, 1000);
    assert_near(parts[0].estimated_power_w, 2000.0f);
    assert_near(parts[1].estimated_power_w, 1000.0f);
    assert_near(parts[2].estimated_power_w, 0.0f);
    assert_near(parts[0].estimated_power_w + parts[1].estimated_power_w + parts[2].estimated_power_w,
                3000.0f);
    assert(!parts[0].energy_updated);
    assert(parts[0].accumulated_energy_kwh == 0.0);

    // Second update 1 hour later at same power -> energy = P_avg * 1h / 1000
    alloc.recalculate(parts, 1000 + 3600000);
    assert(parts[0].energy_updated);
    assert(parts[1].energy_updated);
    // 2000W * 1h / 1000 = 2 kWh; 1000W * 1h / 1000 = 1 kWh
    assert_near(static_cast<float>(parts[0].accumulated_energy_kwh), 2.0f);
    assert_near(static_cast<float>(parts[1].accumulated_energy_kwh), 1.0f);
    assert_near(static_cast<float>(parts[2].accumulated_energy_kwh), 0.0f);
    assert_near(static_cast<float>(parts[0].accumulated_energy_kwh + parts[1].accumulated_energy_kwh +
                                   parts[2].accumulated_energy_kwh),
                3.0f); // == outdoor 3000W * 1h / 1000
}

static void test_recalculate_equal_fallback()
{
    cout << "test_recalculate_equal_fallback" << endl;
    PowerAllocator alloc;
    alloc.set_outdoor_power(90.0f);

    vector<IndoorAllocParticipant> parts(2);
    parts[0].address = "00";
    parts[0].thermo_on = false;
    parts[1].address = "01";
    parts[1].thermo_on = false;

    alloc.recalculate(parts, 0);
    assert_near(parts[0].estimated_power_w, 45.0f);
    assert_near(parts[1].estimated_power_w, 45.0f);
}

int main()
{
    test_weight_for();
    test_allocate_capacity_ratio();
    test_allocate_equal_when_all_zero();
    test_allocate_single();
    test_recalculate_sum_and_energy();
    test_recalculate_equal_fallback();
    cout << "All power allocator tests passed." << endl;
    return 0;
}
