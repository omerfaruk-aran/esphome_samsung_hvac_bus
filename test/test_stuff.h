#include <vector>
#include <iostream>
#include <bitset>
#include <cassert>
#include <optional>
#include <set>
#include "esphome/core/optional.h"

#include "../components/samsung_ac/util.h"
#include "../components/samsung_ac/protocol.h"

using namespace std;
using namespace esphome::samsung_ac;

class DebugTarget : public MessageTarget
{
public:
    uint32_t get_miliseconds() override
    {
        return 0;
    }

    std::string last_publish_data;
    void publish_data(uint8_t id, std::vector<uint8_t> &&data) override
    {
        last_publish_data = bytes_to_hex(data);
        cout << "> publish_data " << last_publish_data << endl;
    }

    void ack_data(uint8_t id) override
    {
        cout << "> ack_data id=" << (int)id << endl;
    }

    std::string last_register_address;
    void register_address(const std::string address) override
    {
        cout << "> register_address " << address << endl;
        last_register_address = address;
    }

    std::string last_set_power_address;
    bool last_set_power_value;
    void set_power(const std::string address, bool value) override
    {
        cout << "> " << address << " set_power=" << to_string(value) << endl;
        last_set_power_address = address;
        last_set_power_value = value;
    }

    std::string last_set_automatic_cleaning_address;
    bool last_set_automatic_cleaning_value;
    void set_automatic_cleaning(const std::string address, bool value) override
    {
        cout << "> " << address << " set_automatic_cleaning=" << to_string(value) << endl;
        last_set_automatic_cleaning_address = address;
        last_set_automatic_cleaning_value = value;
    }

    std::string last_set_water_heater_power_address;
    bool last_set_water_heater_power_value;
    void set_water_heater_power(const std::string address, bool value) override
    {
        cout << "> " << address << " set_water_heater_power=" << to_string(value) << endl;
        last_set_water_heater_power_address = address;
        last_set_water_heater_power_value = value;
    }

    std::string last_set_room_temperature_address;
    float last_set_room_temperature_value;
    void set_room_temperature(const std::string address, float value) override
    {
        cout << "> " << address << " set_room_temperature=" << to_string(value) << endl;
        last_set_room_temperature_address = address;
        last_set_room_temperature_value = value;
    }

    std::string last_set_water_temperature_address;
    float last_set_water_temperature_value;
    void set_water_temperature(const std::string address, float value)
    {
        cout << "> " << address << " set_water_temperature=" << to_string(value) << endl;
        last_set_water_temperature_address = address;
        last_set_water_temperature_value = value;
    }

    std::string last_set_target_temperature_address;
    float last_set_target_temperature_value;
    void set_target_temperature(const std::string address, float value) override
    {
        cout << "> " << address << " set_target_temperature=" << to_string(value) << endl;
        last_set_target_temperature_address = address;
        last_set_target_temperature_value = value;
    }

    std::string last_set_water_outlet_target_address;
    float last_set_water_outlet_target_value;
    void set_water_outlet_target(const std::string address, float value) override
    {
        cout << "> " << address << " set_water_outlet_target=" << to_string(value) << endl;
        last_set_water_outlet_target_address = address;
        last_set_water_outlet_target_value = value;
    }

    std::string last_set_outdoor_temperature_address;
    float last_set_outdoor_temperature_value;
    void set_outdoor_temperature(const std::string address, float value) override
    {
        cout << "> " << address << " set_outdoor_temperature=" << to_string(value) << endl;
        last_set_outdoor_temperature_address = address;
        last_set_outdoor_temperature_value = value;
    }

    std::string last_set_target_water_temperature_address;
    float last_set_target_water_temperature_value;
    void set_target_water_temperature(const std::string address, float value) override
    {
        cout << "> " << address << " set_target_water_temperature=" << to_string(value) << endl;
        last_set_target_water_temperature_address = address;
        last_set_target_water_temperature_value = value;
    }

    std::string last_set_room_humidity_address;
    float last_set_room_humidity_value;
    void set_room_humidity(const std::string address, float value)
    {
        cout << "> " << address << " set_room_humidity=" << to_string(value) << endl;
        last_set_room_humidity_address = address;
        last_set_room_humidity_value = value;
    }

    std::string last_set_mode_address;
    Mode last_set_mode_mode;
    void set_mode(const std::string address, Mode mode) override
    {
        cout << "> " << address << " set_mode=" << to_string((int)mode) << endl;
        last_set_mode_address = address;
        last_set_mode_mode = mode;
    }

    std::string last_set_water_heater_mode_address;
    WaterHeaterMode last_set_water_heater_mode_value;
    void set_water_heater_mode(const std::string address, WaterHeaterMode waterheatermode) override
    {
        cout << "> " << address << " set_water_heater_mode=" << to_string((int)waterheatermode) << endl;
        last_set_water_heater_mode_address = address;
        last_set_water_heater_mode_value = waterheatermode;
    }

    std::string last_set_fanmode_address;
    FanMode last_set_fanmode_mode;
    void set_fanmode(const std::string address, FanMode fanmode) override
        {
            cout << "> " << address << " set_fanmode=" << to_string((int)fanmode) << endl;
            last_set_fanmode_address = address;
            last_set_fanmode_mode = fanmode;
        }

    std::string last_set_altmode_address;
    AltMode last_set_altmode_value;
    void set_altmode(const std::string address, AltMode altmode) override
    {
        cout << "> " << address << " set_altmode=" << to_string((int)altmode) << endl;
        last_set_altmode_address = address;
        last_set_altmode_value = altmode;
    }

    std::string last_set_swing_vertical_address;
    bool last_set_swing_vertical_value;
    void set_swing_vertical(const std::string address, bool vertical) override
    {
        cout << "> " << address << " set_swing_vertical=" << to_string((int)vertical) << endl;
        last_set_swing_vertical_address = address;
        last_set_swing_vertical_value = vertical;
    }

    std::string last_set_swing_horizontal_address;
    bool last_set_swing_horizontal_value;
    void set_swing_horizontal(const std::string address, bool horizontal) override
    {
        cout << "> " << address << " set_swing_horizontal=" << to_string((int)horizontal) << endl;
        last_set_swing_horizontal_address = address;
        last_set_swing_horizontal_value = horizontal;
    }

    std::set<uint16_t> last_custom_sensors;
    void set_custom_sensor(const std::string address, uint16_t message_number, float value) override
        {
            last_custom_sensors.insert(message_number);
        }

    // Cmd8D: Power/Energy methods
    std::string last_set_outdoor_current_address;
    float last_set_outdoor_current_value;
    void set_outdoor_current(const std::string &address, float value) override
        {
            cout << "> " << address << " set_outdoor_current=" << to_string(value) << endl;
            last_set_outdoor_current_address = address;
            last_set_outdoor_current_value = value;
        }

    std::string last_set_outdoor_voltage_address;
    float last_set_outdoor_voltage_value;
    void set_outdoor_voltage(const std::string &address, float value) override
        {
            cout << "> " << address << " set_outdoor_voltage=" << to_string(value) << endl;
            last_set_outdoor_voltage_address = address;
            last_set_outdoor_voltage_value = value;
        }

    std::string last_set_outdoor_instantaneous_power_address;
    float last_set_outdoor_instantaneous_power_value;
    void set_outdoor_instantaneous_power(const std::string &address, float value) override
        {
            cout << "> " << address << " set_outdoor_instantaneous_power=" << to_string(value) << endl;
            last_set_outdoor_instantaneous_power_address = address;
            last_set_outdoor_instantaneous_power_value = value;
        }

    std::string last_set_outdoor_cumulative_energy_address;
    float last_set_outdoor_cumulative_energy_value;
    void set_outdoor_cumulative_energy(const std::string &address, float value) override
        {
            cout << "> " << address << " set_outdoor_cumulative_energy=" << to_string(value) << endl;
            last_set_outdoor_cumulative_energy_address = address;
            last_set_outdoor_cumulative_energy_value = value;
        }

    std::string last_set_indoor_eva_in_temperature_address;
    float last_set_indoor_eva_in_temperature_value;
    void set_indoor_eva_in_temperature(const std::string address, float value) override
        {
            cout << "> " << address << " set_indoor_eva_in_temperature=" << to_string(value) << endl;
            last_set_indoor_eva_in_temperature_address = address;
            last_set_indoor_eva_in_temperature_value = value;
        }

    std::string last_set_indoor_eva_out_temperature_address;
    float last_set_indoor_eva_out_temperature_value;
    void set_indoor_eva_out_temperature(const std::string address, float value) override
        {
            cout << "> " << address << " set_indoor_eva_out_temperature=" << to_string(value) << endl;
            last_set_indoor_eva_out_temperature_address = address;
            last_set_indoor_eva_out_temperature_value = value;
        }

    std::string last_set_error_code_address;
    int last_set_error_code_value;
    void set_error_code(const std::string address, int error_code) override
    {
        cout << "> " << address << " set_error_code=" << to_string(error_code) << endl;
        last_set_error_code_address = address;
        last_set_error_code_value = error_code;
    }

        void assert_only_address(const std::string address)
        {
            assert(last_register_address == address);
            assert(last_set_power_address == "");
            assert(last_set_room_temperature_address == "");
            assert(last_set_target_temperature_address == "");
            assert(last_set_mode_address == "");
            assert(last_set_fanmode_address == "");
        }

        void assert_values(const std::string address, bool power, float room_temp, float target_temp, Mode mode, FanMode fanmode)
        {
            assert(last_register_address == address);

            assert(last_set_power_address == address);
            assert(last_set_power_value == power);

            assert(last_set_room_temperature_address == address);
            assert(last_set_room_temperature_value == room_temp);

            assert(last_set_target_temperature_address == address);
            assert(last_set_target_temperature_value == target_temp);

            assert(last_set_mode_address == address);
            assert(last_set_mode_mode == mode);

            assert(last_set_fanmode_address == address);
            assert(last_set_fanmode_mode == fanmode);
        }

        void assert_values(const std::string address, bool power, float room_temp, float target_temp, Mode mode, FanMode fanmode, float humidity)
        {
            assert_values(address, power, room_temp, target_temp, mode, fanmode);

            assert(last_set_room_humidity_address == address);
            assert(last_set_room_humidity_value == humidity);
        }
    };

    void test_process_data(const std::string &hex, DebugTarget &target)
    {
        cout << "test: " << hex << std::endl;
        auto bytes = hex_to_bytes(hex);
        auto result = process_data(bytes, &target);
        assert(result.type == DecodeResultType::Processed);
    }

    DebugTarget test_process_data(const std::string &hex)
    {
        DebugTarget target;
        test_process_data(hex, target);
        return target;
    }

    void assert_str(const std::string actual, const std::string expected)
    {
        if (actual != expected)
        {
            cout << "actual:   " << actual << std::endl;
            cout << "expected: " << expected << std::endl;
        }
        assert(actual == expected);
    }

    namespace esphome
    {
        uint32_t test_millis_value = 0;
        uint32_t millis()
        {
            return test_millis_value;
        }
        uint32_t micros()
        {
            return 0;
        }
        void delay(uint32_t ms) {}
    } // namespace esphome
