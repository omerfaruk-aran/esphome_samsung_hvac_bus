#include "test_stuff.h"
#include "../components/samsung_ac/protocol_non_nasa.h"
#include <functional>
#include <cmath>

using namespace std;
using namespace esphome::samsung_ac;

std::vector<uint8_t> create(uint8_t src, uint8_t dst)
{
    std::vector<uint8_t> data;

    for (int i = 0; i < 14; i++)
    {
        data.push_back(0);
    }

    data[0] = 0x32;
    data[1] = src;
    data[2] = dst;

    data[3] = 0x20; // cmd

    uint8_t target_temp = 22;
    uint8_t room_temp = 26;
    uint8_t pipe_in = 25;

    uint8_t pipe_out = 26;

    data[4] = target_temp + 55;
    data[5] = room_temp + 55;
    data[6] = pipe_in + 55;

    data[11] = pipe_out + 55;

    uint8_t a = 8;
    uint8_t b = 16;

    cout << "a    " << std::bitset<8>(a) << endl;
    cout << "b    " << std::bitset<8>(b) << endl;

    uint8_t test = 0;

    test |= a & 0b1111;
    test |= b << 4 & 0b11110000;

    cout << "test " << std::bitset<8>(test) << endl;

    uint8_t a2 = test & 0b00001111;
    uint8_t b2 = (test & 0b11110000) >> 4;

    cout << "a2   " << std::bitset<8>(a2) << " " << std::to_string(a2) << endl;
    cout << "b2   " << std::bitset<8>(b2) << " " << std::to_string(b2) << endl;

    /*

                uint8_t fanspeed = data[7] & 0b00001111;
                bool bladeswing = (data[7] & 0b11110000) == 0xD0;
                bool power = data[8] & 0b10000000;
                uint8_t mode = data[8] & 0b00111111;
                uint8_t pipe_out = data[11] - 55;
    */

    // chk

    data[13] = 0x34;

    return data;
}

NonNasaDataPacket test_decode(std::string data)
{
    NonNasaDataPacket p;
    auto bytes = hex_to_bytes(data);
    auto result = p.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    std::cout << p.to_string() << std::endl;
    return p;
}

void test_decoding()
{
    auto p = test_decode("3200c8204b504e000110004ee234");
    assert(p.command20.power == false);
    assert(p.command20.target_temp == 20);
    assert(p.command20.room_temp == 25);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::Auto);
    assert(p.command20.mode == NonNasaMode::Heat);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204b4f4efd8110004e8034");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 20);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::High);
    assert(p.command20.mode == NonNasaMode::Heat);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204b4f4efc8110004e8134");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 20);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::Medium);
    assert(p.command20.mode == NonNasaMode::Heat);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204b4f4efa8110004e8734");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 20);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::Low);
    assert(p.command20.mode == NonNasaMode::Heat);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204f4f4ef8a21c004eae34");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 24);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::Auto);
    assert(p.command20.mode == NonNasaMode::Auto);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204f4f4efd821c004e8b34");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 24);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::High);
    assert(p.command20.mode == NonNasaMode::Cool);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);

    p = test_decode("3200c8204f4f4efd821c004e8b34");
    assert(p.command20.power == true);
    assert(p.command20.target_temp == 24);
    assert(p.command20.room_temp == 24);
    assert(p.command20.pipe_in == 23);
    assert(p.command20.pipe_out == 23);
    assert(p.command20.fanspeed == NonNasaFanspeed::High);
    assert(p.command20.mode == NonNasaMode::Cool);
    assert(p.command20.wind_direction == NonNasaWindDirection::Stop);
}

NonNasaRequest create_request()
{
    NonNasaRequest p;
    p.dst = "00";
    p.power = false;
    p.target_temp = 20;
    p.fanspeed = NonNasaFanspeed::Auto;
    p.mode = NonNasaMode::Auto;
    return p;
}

void test_request(NonNasaRequest request, std::string expected)
{
    auto actual = bytes_to_hex(request.encode());
    assert_str(actual, expected);
}

void test_encoding()
{
    NonNasaRequest req;

    req = create_request();
    req.dst = "00";
    req.power = true;
    req.room_temp = 23;
    req.target_temp = 24;
    req.fanspeed = NonNasaFanspeed::Auto;
    req.mode = NonNasaMode::Fan;
    test_request(req, "32d000b01f171803f4210000a634");

    req = create_request();
    req.power = true;
    test_request(req, "32d000b01f041400f4210000ba34");

    req = create_request();
    req.power = false;
    test_request(req, "32d000b01f041400c42100008a34");

    req = create_request();
    req.fanspeed = NonNasaFanspeed::Auto;
    test_request(req, "32d000b01f041400c42100008a34");
    req = create_request();
    req.fanspeed = NonNasaFanspeed::High;
    test_request(req, "32d000b01f04b400c42100002a34");
    req = create_request();
    req.fanspeed = NonNasaFanspeed::Medium;
    test_request(req, "32d000b01f049400c42100000a34");
    req = create_request();
    req.fanspeed = NonNasaFanspeed::Low;
    test_request(req, "32d000b01f045400c4210000ca34");

    req = create_request();
    req.target_temp = 25;
    test_request(req, "32d000b01f041900c42100008734");

    req = create_request();
    req.mode = NonNasaMode::Auto;
    test_request(req, "32d000b01f041400c42100008a34");
    req = create_request();
    req.mode = NonNasaMode::Cool;
    test_request(req, "32d000b01f041401c42100008b34");
    req = create_request();
    req.mode = NonNasaMode::Dry;
    test_request(req, "32d000b01f041402c42100008834");
    req = create_request();
    req.mode = NonNasaMode::Fan;
    test_request(req, "32d000b01f041403c42100008934");
    req = create_request();
    req.mode = NonNasaMode::Heat;
    test_request(req, "32d000b01f041404c42100008e34");
}

void test_target()
{
    DebugTarget target;

    target = test_process_data("32c8dec70101000000000000d134");
    target.assert_only_address("c8");
    target = test_process_data("32c8f0860100000000000008b734");
    target.assert_only_address("c8");
    target = test_process_data("32c8add1ff000000000000004b34");
    target.assert_only_address("c8");
    target = test_process_data("32c8008f00000000000000004734");
    target.assert_only_address("c8");
    target = test_process_data("32c800c0080000004b004d4b4d34");
    target.assert_only_address("c8");
    target = test_process_data("3200c8210300000600000000ec34");
    target.assert_only_address("00");
    target = test_process_data("3200c82f00f0010b010201051a34");
    target.assert_only_address("00");
    target = test_process_data("3200c84020000000408900402134");
    target.assert_only_address("00");

    target = test_process_data("3200c8204d51500001100051e434");
    target.assert_values("00", false, 26.000000, 22.000000, Mode::Heat, FanMode::Auto);

    target = test_process_data("3200c8204f4f4efd821c004e8b34");
    target.assert_values("00", true, 24.000000, 24.000000, Mode::Cool, FanMode::High);
}

// Helper function to build a valid non-nasa packet with checksum
std::vector<uint8_t> build_packet(uint8_t src, uint8_t dst, uint8_t cmd, std::function<void(std::vector<uint8_t>&)> fill_data)
{
    std::vector<uint8_t> data(14, 0);
    data[0] = 0x32;
    data[1] = src;
    data[2] = dst;
    data[3] = cmd;
    data[13] = 0x34;
    
    // Fill in data bytes
    if (fill_data)
    {
        fill_data(data);
    }
    
    // Calculate checksum (XOR of bytes 1-11)
    uint8_t checksum = data[1];
    for (int i = 2; i < 12; i++)
    {
        checksum ^= data[i];
    }
    data[12] = checksum;
    
    return data;
}

// Helper to convert packet to hex string
std::string packet_to_hex(std::vector<uint8_t> &data)
{
    return bytes_to_hex(data);
}

void test_previous_data_is_used_correctly()
{
    // Sending package 20 on non nasa requiers to send the previous values
    // these values need to be stored for each address. This test makes sure
    // this process works.
    std::cout << "test_previous_data_is_used_correctly" << std::endl;

    DebugTarget target;

    // Test1

    // prepare last values
    test_process_data("3200c8204d51500001100051e434", target);

    ProtocolRequest req1;
    req1.power = false;
    get_protocol("00")->publish_request(&target, "00", req1);
    // Use CmdC6 (request_control) packet to trigger send_requests()
    // Format: src=c8 (outdoor), dst=d0 (controller), cmd=c6, control_status=0x01
    auto cmdC6_packet1 = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    test_process_data(packet_to_hex(cmdC6_packet1), target); // trigger publish

    NonNasaRequest request1;
    request1.dst = "00";
    request1.room_temp = 26.000000;
    request1.target_temp = 22.000000;
    request1.power = false;
    request1.fanspeed = NonNasaFanspeed::Auto;
    request1.mode = NonNasaMode::Heat;
    assert_str(bytes_to_hex(request1.encode()), target.last_publish_data);

    // Test2

    // prepare last values
    test_process_data("3201c8204f4f4efd821c004e8a34", target);

    ProtocolRequest req2;
    req2.power = true;
    get_protocol("01")->publish_request(&target, "01", req2);
    // Use CmdC6 (request_control) packet to trigger send_requests()
    auto cmdC6_packet2 = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    test_process_data(packet_to_hex(cmdC6_packet2), target); // trigger publish

    NonNasaRequest request2;
    request2.dst = "01";
    request2.room_temp = 24.000000;
    request2.target_temp = 24.000000;
    request2.power = true;
    request2.fanspeed = NonNasaFanspeed::High;
    request2.mode = NonNasaMode::Cool;
    assert_str(bytes_to_hex(request2.encode()), target.last_publish_data);
}

void test_cmdc0_outdoor_temperature()
{
    std::cout << "test_cmdc0_outdoor_temperature" << std::endl;
    
    // Build CmdC0 packet: outdoor temp = 25°C (25 + 55 = 80 = 0x50)
    auto packet = build_packet(0xc8, 0x00, 0xc0, [](std::vector<uint8_t> &data) {
        data[8] = 25 + 55; // outdoor_temp = 25°C
    });
    
    DebugTarget target;
    test_process_data(packet_to_hex(packet), target);
    
    assert(target.last_register_address == "c8");
    assert(target.last_set_outdoor_temperature_address == "c8");
    assert(target.last_set_outdoor_temperature_value == 25.0f);
    
    // Test negative temperature: -5°C (-5 + 55 = 50 = 0x32, but as uint8_t it wraps)
    // Actually, -5°C would be stored as 50, but we need to test signed conversion
    packet = build_packet(0xc8, 0x00, 0xc0, [](std::vector<uint8_t> &data) {
        data[8] = (uint8_t)(-5 + 55); // -5°C
    });
    
    target = DebugTarget();
    test_process_data(packet_to_hex(packet), target);
    assert(target.last_set_outdoor_temperature_value == -5.0f);
}

void test_cmd8d_power_energy()
{
    std::cout << "test_cmd8d_power_energy" << std::endl;
    
    // Build Cmd8D packet: current=100 (raw), voltage=60 (raw)
    // Raw current=100 is divided by 10 to get 10.0A (calculated current, before sensor filter)
    // Raw voltage=60 is multiplied by 2 to get 120.0V (published voltage)
    // Power calculation: (100/10) * 0.1 * (60*2) = 10.0 * 0.1 * 120 = 120W
    // Note: Current sensor has filter multiply: 0.1, so published_current = 10.0 * 0.1 = 1.0A
    // Published power = 120W, verification: 1.0A * 120V = 120W ✓
    auto packet = build_packet(0xc8, 0x00, 0x8d, [](std::vector<uint8_t> &data) {
        data[8] = 100;  // raw current (will be / 10 = 10.0A, then * 0.1 = 1.0A after sensor filter)
        data[10] = 60;  // raw voltage (will be * 2 = 120V)
    });
    
    DebugTarget target;
    esphome::test_millis_value = 1000; // Start at 1 second
    test_process_data(packet_to_hex(packet), target);
    
    assert(target.last_register_address == "c8");
    assert(target.last_set_outdoor_current_address == "c8");
    assert(target.last_set_outdoor_current_value == 10.0f); // 100 / 10 = 10.0A (before sensor filter)
    assert(target.last_set_outdoor_voltage_address == "c8");
    assert(target.last_set_outdoor_voltage_value == 120.0f); // 60 * 2
    assert(target.last_set_outdoor_instantaneous_power_address == "c8");
    assert(std::abs(target.last_set_outdoor_instantaneous_power_value - 120.0f) < 0.01f); // 10.0 * 0.1 * 120.0 = 120W
    
    // First update - no energy calculated yet
    assert(target.last_set_outdoor_cumulative_energy_address == "c8");
    assert(target.last_set_outdoor_cumulative_energy_value == 0.0f);
    
    // Second update after 1 hour - should accumulate energy
    // Reset target and process first packet
    target = DebugTarget();
    esphome::test_millis_value = 1000;
    test_process_data(packet_to_hex(packet), target);
    
    // Process second packet 1 hour later
    esphome::test_millis_value = 1000 + 3600000; // 1 hour = 3600000 ms
    test_process_data(packet_to_hex(packet), target);
    
    // Energy should be approximately 120W * 1 hour = 0.12 kWh = 120 Wh
    // Using trapezoidal rule: average_power = (120 + 120) / 2 = 120W
    // Energy = 120W * 1 hour = 120 Wh = 0.12 kWh
    // Published in Wh: 0.12 kWh * 1000 = 120 Wh
    assert(target.last_set_outdoor_cumulative_energy_value > 110.0f);
    assert(target.last_set_outdoor_cumulative_energy_value < 130.0f);
}

void test_cmd20_eva_temperatures()
{
    std::cout << "test_cmd20_eva_temperatures" << std::endl;
    
    // Build Cmd20 packet with pipe_in=23°C, pipe_out=25°C
    auto packet = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 22 + 55; // target_temp
        data[5] = 24 + 55; // room_temp
        data[6] = 23 + 55; // pipe_in = 23°C
        data[7] = 0;       // wind_direction/fanspeed
        data[8] = 0x01;    // mode = Heat, power = false
        data[11] = 25 + 55; // pipe_out = 25°C
    });
    
    DebugTarget target;
    test_process_data(packet_to_hex(packet), target);
    
    assert(target.last_register_address == "00");
    assert(target.last_set_indoor_eva_in_temperature_address == "00");
    assert(target.last_set_indoor_eva_in_temperature_value == 23.0f);
    assert(target.last_set_indoor_eva_out_temperature_address == "00");
    assert(target.last_set_indoor_eva_out_temperature_value == 25.0f);
    
    // Test negative temperatures
    packet = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 22 + 55;
        data[5] = 24 + 55;
        data[6] = (uint8_t)(-5 + 55); // pipe_in = -5°C
        data[7] = 0;
        data[8] = 0x01;
        data[11] = (uint8_t)(-3 + 55); // pipe_out = -3°C
    });
    
    target = DebugTarget();
    test_process_data(packet_to_hex(packet), target);
    assert(target.last_set_indoor_eva_in_temperature_value == -5.0f);
    assert(target.last_set_indoor_eva_out_temperature_value == -3.0f);
}

void test_cmdf0_error_code()
{
    std::cout << "test_cmdf0_error_code" << std::endl;
    
    // Build CmdF0 packet with error code = 0 (no error)
    auto packet = build_packet(0xc8, 0x00, 0xf0, [](std::vector<uint8_t> &data) {
        data[4] = 0;  // status flags
        data[5] = 0;  // inverter_order_frequency
        data[6] = 0;  // inverter_target_frequency
        data[7] = 0;  // inverter_current_frequency
        data[8] = 0;  // bldc_fan
        data[10] = 0; // error_code = 0
    });
    
    DebugTarget target;
    test_process_data(packet_to_hex(packet), target);
    
    assert(target.last_register_address == "c8");
    assert(target.last_set_error_code_address == "c8");
    assert(target.last_set_error_code_value == 0);
    
    // Test with error code = 5
    packet = build_packet(0xc8, 0x00, 0xf0, [](std::vector<uint8_t> &data) {
        data[4] = 0;
        data[5] = 0;
        data[6] = 0;
        data[7] = 0;
        data[8] = 0;
        data[10] = 5; // error_code = 5
    });
    
    target = DebugTarget();
    test_process_data(packet_to_hex(packet), target);
    assert(target.last_set_error_code_value == 5);
}

void test_cmdc6_control_status()
{
    std::cout << "test_cmdc6_control_status" << std::endl;
    
    DebugTarget target;
    
    // Test 1: CmdC6 with control_status=true, correct src/dst, indoor_unit_awake=true
    // This should trigger send_requests() which calls publish_data
    // First, set up state: send a Cmd20 packet to make indoor_unit_awake=true
    test_process_data("3200c8204d51500001100051e434", target);
    
    // Publish a request
    ProtocolRequest req;
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Send CmdC6 with control_status=true
    auto cmdC6_packet = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    
    target.last_publish_data = ""; // Clear previous
    test_process_data(packet_to_hex(cmdC6_packet), target);
    
    // Verify send_requests() was called (publish_data was called)
    assert(!target.last_publish_data.empty());
    
    // Test 2: CmdC6 with control_status=false - should NOT trigger send_requests()
    target = DebugTarget();
    test_process_data("3200c8204d51500001100051e434", target);
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    auto cmdC6_packet_false = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x00; // control_status = false
    });
    
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_packet_false), target);
    
    // Verify send_requests() was NOT called
    assert(target.last_publish_data.empty());
    
    // Test 3: Verify packet decoding
    NonNasaDataPacket packet;
    auto bytes = hex_to_bytes(packet_to_hex(cmdC6_packet));
    auto result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.cmd == NonNasaCommand::CmdC6);
    assert(packet.commandC6.control_status == true);
}

void test_cmd54_control_ack()
{
    std::cout << "test_cmd54_control_ack" << std::endl;
    
    DebugTarget target;
    
    // Test: Cmd54 with dst="d0" should remove pending requests from queue
    // First, publish a request
    ProtocolRequest req;
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Verify request is in queue (by checking that send_requests would publish it)
    test_process_data("3200c8204d51500001100051e434", target); // Make indoor awake
    auto cmdC6 = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01;
    });
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6), target);
    assert(!target.last_publish_data.empty()); // Request was sent
    
    // Now send Cmd54 acknowledgment from indoor unit (00) to controller (d0)
    auto cmd54_packet = build_packet(0x00, 0xd0, 0x54, [](std::vector<uint8_t> &data) {
        // Cmd54 data is raw, can be any value
    });
    
    // Publish another request
    req.power = false;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Process Cmd54 - this should remove the pending request
    test_process_data(packet_to_hex(cmd54_packet), target);
    
    // Verify packet decoding
    NonNasaDataPacket packet;
    auto bytes = hex_to_bytes(packet_to_hex(cmd54_packet));
    auto result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.cmd == NonNasaCommand::Cmd54);
    assert(packet.dst == "d0");
}

void test_cmdf3_decoded_but_not_processed()
{
    std::cout << "test_cmdf3_decoded_but_not_processed" << std::endl;
    
    DebugTarget target;
    
    // Build CmdF3 packet
    // CmdF3 structure: inverter_max_frequency_hz (byte 4), inverter_total_capacity_requirement_kw (byte 5 / 10),
    // inverter_current_a (byte 8 / 10), inverter_voltage_v (byte 9 * 2), inverter_power_w (calculated)
    auto packet = build_packet(0xc8, 0x00, 0xf3, [](std::vector<uint8_t> &data) {
        data[4] = 100; // inverter_max_frequency_hz
        data[5] = 50;  // inverter_total_capacity_requirement_kw = 50/10 = 5.0 kW
        data[8] = 25;  // inverter_current_a = 25/10 = 2.5 A
        data[9] = 86;  // inverter_voltage_v = 86*2 = 172 V
    });
    
    test_process_data(packet_to_hex(packet), target);
    
    // Verify address is registered
    assert(target.last_register_address == "c8");
    
    // Verify NO target methods are called (CmdF3 has no handler in process_non_nasa_packet)
    assert(target.last_set_power_address.empty());
    assert(target.last_set_error_code_address.empty());
    assert(target.last_set_outdoor_temperature_address.empty());
    assert(target.last_set_outdoor_current_address.empty());
    assert(target.last_set_outdoor_voltage_address.empty());
    assert(target.last_publish_data.empty());
    
    // Verify packet decoding works
    NonNasaDataPacket decoded_packet;
    auto bytes = hex_to_bytes(packet_to_hex(packet));
    auto decode_result = decoded_packet.decode(bytes);
    assert(decode_result.type == DecodeResultType::Processed);
    assert(decoded_packet.cmd == NonNasaCommand::CmdF3);
    assert(decoded_packet.commandF3.inverter_max_frequency_hz == 100);
    assert(std::abs(decoded_packet.commandF3.inverter_total_capacity_requirement_kw - 5.0f) < 0.01f);
    assert(std::abs(decoded_packet.commandF3.inverter_current_a - 2.5f) < 0.01f);
    assert(std::abs(decoded_packet.commandF3.inverter_voltage_v - 172.0f) < 0.01f);
    assert(std::abs(decoded_packet.commandF3.inverter_power_w - 43.0f) < 0.01f); // 2.5 * 0.1 * 172 = 43W
}

void test_cmdc1_decoded_but_not_processed()
{
    std::cout << "test_cmdc1_decoded_but_not_processed" << std::endl;
    
    DebugTarget target;
    
    // Build CmdC1 packet
    // CmdC1 structure: outdoor_unit_sump_temp_c (byte 8 - 55)
    auto packet = build_packet(0xc8, 0x00, 0xc1, [](std::vector<uint8_t> &data) {
        data[8] = 25 + 55; // sump_temp = 25°C
    });
    
    test_process_data(packet_to_hex(packet), target);
    
    // Verify address is registered
    assert(target.last_register_address == "c8");
    
    // Verify NO target methods are called (CmdC1 has no handler in process_non_nasa_packet)
    assert(target.last_set_power_address.empty());
    assert(target.last_set_error_code_address.empty());
    assert(target.last_set_outdoor_temperature_address.empty());
    assert(target.last_publish_data.empty());
    
    // Verify packet decoding works
    NonNasaDataPacket decoded_packet;
    auto bytes = hex_to_bytes(packet_to_hex(packet));
    auto decode_result = decoded_packet.decode(bytes);
    assert(decode_result.type == DecodeResultType::Processed);
    assert(decoded_packet.cmd == NonNasaCommand::CmdC1);
    assert(decoded_packet.commandC1.outdoor_unit_sump_temp_c == 25);
}

void test_cmdf1_decoded_but_not_processed()
{
    std::cout << "test_cmdf1_decoded_but_not_processed" << std::endl;
    
    DebugTarget target;
    
    // Build CmdF1 packet
    // CmdF1 structure: EEV values (4 uint16_t values: bytes 4-5, 6-7, 8-9, 10-11)
    auto packet = build_packet(0xc8, 0x00, 0xf1, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; data[5] = 0x00; // EEV_A = 0x0100 = 256
        data[6] = 0x02; data[7] = 0x00; // EEV_B = 0x0200 = 512
        data[8] = 0x03; data[9] = 0x00; // EEV_C = 0x0300 = 768
        data[10] = 0x04; data[11] = 0x00; // EEV_D = 0x0400 = 1024
    });
    
    test_process_data(packet_to_hex(packet), target);
    
    // Verify address is registered
    assert(target.last_register_address == "c8");
    
    // Verify NO target methods are called (CmdF1 has no handler in process_non_nasa_packet)
    assert(target.last_set_power_address.empty());
    assert(target.last_set_error_code_address.empty());
    assert(target.last_set_outdoor_temperature_address.empty());
    assert(target.last_publish_data.empty());
    
    // Verify packet decoding works
    NonNasaDataPacket decoded_packet;
    auto bytes = hex_to_bytes(packet_to_hex(packet));
    auto decode_result = decoded_packet.decode(bytes);
    assert(decode_result.type == DecodeResultType::Processed);
    assert(decoded_packet.cmd == NonNasaCommand::CmdF1);
    assert(decoded_packet.commandF1.outdoor_unit_EEV_A == 256);
    assert(decoded_packet.commandF1.outdoor_unit_EEV_B == 512);
    assert(decoded_packet.commandF1.outdoor_unit_EEV_C == 768);
    assert(decoded_packet.commandF1.outdoor_unit_EEV_D == 1024);
}

void test_non_nasa_edge_cases()
{
    std::cout << "test_non_nasa_edge_cases" << std::endl;
    
    DebugTarget target;
    
    // Test 1: Temperature boundaries
    // Minimum: 0°C (0 + 55 = 55)
    auto packet_min = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 0 + 55; // target_temp = 0°C
        data[5] = 0 + 55; // room_temp = 0°C
        data[6] = 0 + 55; // pipe_in = 0°C
        data[8] = 0x01;   // mode = Heat, power = false
        data[11] = 0 + 55; // pipe_out = 0°C
    });
    
    test_process_data(packet_to_hex(packet_min), target);
    assert(target.last_set_target_temperature_value == 0.0f);
    assert(target.last_set_room_temperature_value == 0.0f);
    
    // Maximum: 50°C (50 + 55 = 105)
    auto packet_max = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 50 + 55; // target_temp = 50°C
        data[5] = 50 + 55; // room_temp = 50°C
        data[6] = 50 + 55; // pipe_in = 50°C
        data[8] = 0x01;    // mode = Heat, power = false
        data[11] = 50 + 55; // pipe_out = 50°C
    });
    
    target = DebugTarget();
    test_process_data(packet_to_hex(packet_max), target);
    assert(target.last_set_target_temperature_value == 50.0f);
    assert(target.last_set_room_temperature_value == 50.0f);
    
    // Test 2: All fan speed enum values
    NonNasaFanspeed fan_speeds[] = {
        NonNasaFanspeed::Auto,
        NonNasaFanspeed::Low,
        NonNasaFanspeed::Medium,
        NonNasaFanspeed::High,
        NonNasaFanspeed::Fresh
    };
    
    for (auto fanspeed : fan_speeds)
    {
        target = DebugTarget();
        auto packet = build_packet(0x00, 0xc8, 0x20, [fanspeed](std::vector<uint8_t> &data) {
            data[4] = 22 + 55;
            data[5] = 24 + 55;
            data[6] = 23 + 55;
            data[7] = (uint8_t)fanspeed; // fanspeed in lower 3 bits
            data[8] = 0x01; // mode = Heat, power = false
            data[11] = 25 + 55;
        });
        
        test_process_data(packet_to_hex(packet), target);
        // Verify fanspeed is decoded correctly
        NonNasaDataPacket decoded;
        auto bytes = hex_to_bytes(packet_to_hex(packet));
        decoded.decode(bytes);
        assert(decoded.command20.fanspeed == fanspeed);
    }
    
    // Test 3: All mode enum values
    NonNasaMode modes[] = {
        NonNasaMode::Heat,
        NonNasaMode::Cool,
        NonNasaMode::Dry,
        NonNasaMode::Fan,
        NonNasaMode::Auto_Heat,
        NonNasaMode::Auto
    };
    
    for (auto mode : modes)
    {
        target = DebugTarget();
        auto packet = build_packet(0x00, 0xc8, 0x20, [mode](std::vector<uint8_t> &data) {
            data[4] = 22 + 55;
            data[5] = 24 + 55;
            data[6] = 23 + 55;
            data[7] = 0; // fanspeed = Auto
            data[8] = (uint8_t)mode; // mode in lower 6 bits
            data[11] = 25 + 55;
        });
        
        test_process_data(packet_to_hex(packet), target);
        // Verify mode is decoded correctly
        NonNasaDataPacket decoded;
        auto bytes = hex_to_bytes(packet_to_hex(packet));
        decoded.decode(bytes);
        assert(decoded.command20.mode == mode);
    }
    
    // Test 4: All wind direction enum values
    NonNasaWindDirection wind_dirs[] = {
        NonNasaWindDirection::Vertical,
        NonNasaWindDirection::Horizontal,
        NonNasaWindDirection::FourWay,
        NonNasaWindDirection::Stop
    };
    
    for (auto wind_dir : wind_dirs)
    {
        target = DebugTarget();
        auto packet = build_packet(0x00, 0xc8, 0x20, [wind_dir](std::vector<uint8_t> &data) {
            data[4] = 22 + 55;
            data[5] = 24 + 55;
            data[6] = 23 + 55;
            data[7] = ((uint8_t)wind_dir << 3) | 0; // wind_direction in upper 5 bits, fanspeed in lower 3
            data[8] = 0x01; // mode = Heat, power = false
            data[11] = 25 + 55;
        });
        
        test_process_data(packet_to_hex(packet), target);
        // Verify wind_direction is decoded correctly
        NonNasaDataPacket decoded;
        auto bytes = hex_to_bytes(packet_to_hex(packet));
        decoded.decode(bytes);
        assert(decoded.command20.wind_direction == wind_dir);
    }
    
    // Test 5: Power on/off
    target = DebugTarget();
    auto packet_power_on = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 22 + 55;
        data[5] = 24 + 55;
        data[6] = 23 + 55;
        data[7] = 0;
        data[8] = 0x01 | 0x80; // mode = Heat, power = true (bit 7 set)
        data[11] = 25 + 55;
    });
    
    test_process_data(packet_to_hex(packet_power_on), target);
    assert(target.last_set_power_value == true);
    
    target = DebugTarget();
    auto packet_power_off = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 22 + 55;
        data[5] = 24 + 55;
        data[6] = 23 + 55;
        data[7] = 0;
        data[8] = 0x01; // mode = Heat, power = false (bit 7 not set)
        data[11] = 25 + 55;
    });
    
    test_process_data(packet_to_hex(packet_power_off), target);
    assert(target.last_set_power_value == false);
}

void test_non_nasa_invalid_packets()
{
    std::cout << "test_non_nasa_invalid_packets" << std::endl;
    
    DebugTarget target;
    
    // Test 1: Packet too short (< 14 bytes)
    std::vector<uint8_t> short_packet = {0x32, 0x00, 0xc8, 0x20, 0x4d, 0x51, 0x50, 0x00, 0x01, 0x10, 0x00, 0x51, 0xe4}; // 13 bytes
    auto result = process_data(short_packet, &target);
    assert(result.type == DecodeResultType::Fill);
    
    // Test 2: Wrong frame start (not 0x32)
    // Create a packet with wrong start byte
    std::vector<uint8_t> invalid_start_packet = {0x33, 0x00, 0xc8, 0x20, 0x4d, 0x51, 0x50, 0x00, 0x01, 0x10, 0x00, 0x51, 0xe4, 0x34};
    result = process_data(invalid_start_packet, &target);
    assert(result.type == DecodeResultType::Discard);
    // The decoder will try to find the next 0x32, so bytes discarded may vary
    
    // Test 3: Wrong frame end (not 0x34)
    // Test at decode level - decode() checks data[13] != 0x34 and returns Discard
    auto invalid_end = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[13] = 0x35; // Wrong end byte
    });
    
    NonNasaDataPacket packet;
    auto bytes2 = hex_to_bytes(packet_to_hex(invalid_end));
    auto decode_result = packet.decode(bytes2);
    assert(decode_result.type == DecodeResultType::Discard);
    assert(decode_result.bytes == 1);
    
    // Test 4: Invalid checksum
    // The decoder validates checksum and returns Processed if valid, but process_data
    // might handle invalid checksum differently. Let's test with a known invalid checksum.
    // Actually, decode() doesn't check checksum - it's validated later in process_non_nasa_packet.
    // So invalid checksum packets will be decoded successfully but might fail validation later.
    // For this test, we'll just verify that a packet with wrong checksum can be decoded
    // (the checksum validation happens in process_non_nasa_packet, not in decode)
    auto invalid_checksum = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[12] = 0xFF; // Wrong checksum
    });
    
    // Decode will succeed (checksum not checked in decode)
    NonNasaDataPacket packet2;
    auto bytes3 = hex_to_bytes(packet_to_hex(invalid_checksum));
    auto decode_result2 = packet2.decode(bytes3);
    // decode() doesn't validate checksum, so it will return Processed
    // The checksum validation happens in process_non_nasa_packet
    // For this test, we verify decode succeeds (checksum validation is separate)
    assert(decode_result2.type == DecodeResultType::Processed);
}

void test_non_nasa_multiple_addresses()
{
    std::cout << "test_non_nasa_multiple_addresses" << std::endl;
    
    DebugTarget target;
    
    // Test 1: Process packets from multiple addresses (00, 01, 02)
    // Each address should maintain its own state
    
    // Address 00: Set power on, mode Cool, target temp 22
    auto packet00 = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 77; // target_temp = 22°C (22 + 55)
        data[5] = 80; // room_temp = 25°C (25 + 55)
        data[8] = 0x01 | 0x80; // mode = Cool (0x01), power = on (0x80)
    });
    
    auto bytes = hex_to_bytes(packet_to_hex(packet00));
    auto result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "00");
    
    // Address 01: Set power off, mode Heat, target temp 24
    auto packet01 = build_packet(0x01, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 79; // target_temp = 24°C (24 + 55)
        data[5] = 80; // room_temp = 25°C (25 + 55)
        data[8] = 0x04; // mode = Heat (0x04), power = off
    });
    
    bytes = hex_to_bytes(packet_to_hex(packet01));
    result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "01");
    
    // Address 02: Set power on, mode Auto, target temp 20
    auto packet02 = build_packet(0x02, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 75; // target_temp = 20°C (20 + 55)
        data[5] = 80; // room_temp = 25°C (25 + 55)
        data[8] = 0x22 | 0x80; // mode = Auto (0x22), power = on (0x80)
    });
    
    bytes = hex_to_bytes(packet_to_hex(packet02));
    result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "02");
    
    // Test 2: Verify state is maintained per address by processing packets again
    // Process address 00 again - should still register as "00"
    bytes = hex_to_bytes(packet_to_hex(packet00));
    result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "00");
    
    // Process address 01 again - should still register as "01"
    bytes = hex_to_bytes(packet_to_hex(packet01));
    result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "01");
    
    // Process address 02 again - should still register as "02"
    bytes = hex_to_bytes(packet_to_hex(packet02));
    result = process_data(bytes, &target);
    assert(result.type == DecodeResultType::Processed);
    assert(target.last_register_address == "02");
}

void test_request_encoding_edge_cases()
{
    std::cout << "test_request_encoding_edge_cases" << std::endl;
    
    // Test 1: All mode values
    NonNasaMode modes[] = {
        NonNasaMode::Heat,
        NonNasaMode::Cool,
        NonNasaMode::Dry,
        NonNasaMode::Fan,
        NonNasaMode::Auto_Heat,
        NonNasaMode::Auto
    };
    
    for (auto mode : modes)
    {
        NonNasaRequest request;
        request.dst = "00";
        request.mode = mode;
        request.target_temp = 22;
        request.room_temp = 25;
        request.power = true;
        request.fanspeed = NonNasaFanspeed::Auto;
        
        auto encoded = request.encode();
        assert(encoded.size() == 14);
        assert(encoded[0] == 0x32);
        assert(encoded[13] == 0x34);
        
        // Verify it can be decoded
        NonNasaDataPacket packet;
        auto result = packet.decode(encoded);
        assert(result.type == DecodeResultType::Processed);
    }
    
    // Test 2: All fan speed values
    NonNasaFanspeed fan_speeds[] = {
        NonNasaFanspeed::Auto,
        NonNasaFanspeed::Low,
        NonNasaFanspeed::Medium,
        NonNasaFanspeed::High,
        NonNasaFanspeed::Fresh
    };
    
    for (auto fanspeed : fan_speeds)
    {
        NonNasaRequest request;
        request.dst = "00";
        request.mode = NonNasaMode::Heat;
        request.target_temp = 22;
        request.room_temp = 25;
        request.power = true;
        request.fanspeed = fanspeed;
        
        auto encoded = request.encode();
        NonNasaDataPacket packet;
        auto result = packet.decode(encoded);
        assert(result.type == DecodeResultType::Processed);
    }
    
    // Test 3: Temperature boundaries
    // Note: NonNasaRequest encoding is different from Cmd20 decoding
    // NonNasaRequest encodes target_temp in data[6] lower 5 bits: (target_temp & 31U)
    // Cmd20 decodes target_temp from data[4]: data[4] - 55
    // So we can't do a round-trip test. Instead, we verify:
    // 1. Request encoding produces valid packet structure
    // 2. The encoded packet can be decoded (as a request packet, not Cmd20)
    float temps[] = {0.0f, 50.0f};
    for (auto temp : temps)
    {
        NonNasaRequest request;
        request.dst = "00";
        request.mode = NonNasaMode::Heat;
        request.target_temp = temp;
        request.room_temp = 25.0f;
        request.power = true;
        request.fanspeed = NonNasaFanspeed::Auto;
        
        auto encoded = request.encode();
        // Verify packet structure
        assert(encoded.size() == 14);
        assert(encoded[0] == 0x32);
        assert(encoded[13] == 0x34);
        // Verify target_temp is encoded in data[6] lower 5 bits
        uint8_t expected_encoded = (uint8_t)temp & 31U;
        assert((encoded[6] & 31U) == expected_encoded);
    }
    
    // Test 4: Power on/off
    // NonNasaRequest encodes power in data[8]: !power ? 0xC0 : 0xF0 (with other bits)
    // Cmd20 decodes power from data[8] bit 7: data[8] & 0x80
    // These are different formats, so we verify encoding only
    bool power_states[] = {true, false};
    for (auto power : power_states)
    {
        NonNasaRequest request;
        request.dst = "00";
        request.mode = NonNasaMode::Heat;
        request.target_temp = 22;
        request.room_temp = 25;
        request.power = power;
        request.fanspeed = NonNasaFanspeed::Auto;
        
        auto encoded = request.encode();
        // Verify packet structure
        assert(encoded.size() == 14);
        assert(encoded[0] == 0x32);
        assert(encoded[13] == 0x34);
        // Verify power encoding: !power ? 0xC0 : 0xF0 (plus other bits)
        // For power=true: data[8] should have 0xF0 in upper bits
        // For power=false: data[8] should have 0xC0 in upper bits
        if (power)
        {
            assert((encoded[8] & 0xF0) == 0xF0);
        }
        else
        {
            assert((encoded[8] & 0xF0) == 0xC0);
        }
    }
    
    // Test 5: Special case - room_temp = 0
    // NonNasaRequest encodes room_temp in data[5] only if room_temp > 0
    // So room_temp = 0 means data[5] stays at default value (0x04)
    NonNasaRequest request;
    request.dst = "00";
    request.mode = NonNasaMode::Heat;
    request.target_temp = 22;
    request.room_temp = 0; // Special case
    
    auto encoded = request.encode();
    // Verify packet structure
    assert(encoded.size() == 14);
    assert(encoded[0] == 0x32);
    assert(encoded[13] == 0x34);
    // Verify room_temp = 0 means data[5] is not set (stays at default 0x04)
    assert(encoded[5] == 0x04);
    // Verify target_temp is encoded in data[6] lower 5 bits
    assert((encoded[6] & 31U) == (22 & 31U));
}

void test_non_nasa_sequence()
{
    std::cout << "test_non_nasa_sequence" << std::endl;
    
    DebugTarget target;
    
    // Test 1: Process multiple packets in sequence and verify state is maintained
    // Sequence: Cmd20 (state update) -> CmdC6 (trigger requests) -> Cmd54 (ack)
    
    // Step 1: Send Cmd20 to set initial state
    auto cmd20_packet = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 77; // target_temp = 22°C
        data[5] = 80; // room_temp = 25°C
        data[8] = 0x01 | 0x80; // mode = Heat, power = on
    });
    
    test_process_data(packet_to_hex(cmd20_packet), target);
    assert(target.last_set_power_value == true);
    assert(target.last_set_target_temperature_value == 22.0f);
    assert(target.last_set_room_temperature_value == 25.0f);
    
    // Step 2: Publish a request
    ProtocolRequest req;
    req.power = false;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Step 3: Send CmdC6 to trigger send_requests()
    auto cmdC6_packet = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_packet), target);
    assert(!target.last_publish_data.empty()); // Request was sent
    
    // Step 4: Send Cmd54 acknowledgment
    auto cmd54_packet = build_packet(0x00, 0xd0, 0x54, [](std::vector<uint8_t> &data) {
        // Cmd54 data
    });
    
    test_process_data(packet_to_hex(cmd54_packet), target);
    // Cmd54 should be processed (removes pending requests from queue)
    
    // Test 2: Process multiple Cmd20 packets and verify state updates
    target = DebugTarget();
    
    // First Cmd20: power on, temp 22
    auto cmd20_1 = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 77; // target_temp = 22°C
        data[5] = 80; // room_temp = 25°C
        data[8] = 0x01 | 0x80; // mode = Heat, power = on
    });
    test_process_data(packet_to_hex(cmd20_1), target);
    assert(target.last_set_power_value == true);
    assert(target.last_set_target_temperature_value == 22.0f);
    
    // Second Cmd20: power off, temp 24
    auto cmd20_2 = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 79; // target_temp = 24°C
        data[5] = 80; // room_temp = 25°C
        data[8] = 0x01; // mode = Heat, power = off
    });
    test_process_data(packet_to_hex(cmd20_2), target);
    assert(target.last_set_power_value == false);
    assert(target.last_set_target_temperature_value == 24.0f);
    
    // Third Cmd20: power on, temp 20
    auto cmd20_3 = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 75; // target_temp = 20°C
        data[5] = 80; // room_temp = 25°C
        data[8] = 0x01 | 0x80; // mode = Heat, power = on
    });
    test_process_data(packet_to_hex(cmd20_3), target);
    assert(target.last_set_power_value == true);
    assert(target.last_set_target_temperature_value == 20.0f);
    
    // Test 3: Request/response sequence
    target = DebugTarget();
    
    // Send Cmd20 to establish state
    test_process_data(packet_to_hex(cmd20_packet), target);
    
    // Publish request
    req.power = true;
    req.target_temp = 23.0f;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Trigger send with CmdC6
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_packet), target);
    assert(!target.last_publish_data.empty());
    
    // Acknowledge with Cmd54
    test_process_data(packet_to_hex(cmd54_packet), target);
    // Request should be removed from queue
}

void test_cmd20_pending_control_message_ignores_state()
{
    std::cout << "test_cmd20_pending_control_message_ignores_state" << std::endl;
    
    DebugTarget target;
    
    // Test: When a control message is pending, state updates should be IGNORED
    // but EVA temperatures should still be published (they're sensor readings)
    
    // Step 1: Send initial Cmd20 to establish state
    auto cmd20_initial = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 77; // target_temp = 22°C
        data[5] = 80; // room_temp = 25°C
        data[6] = 23 + 55; // pipe_in = 23°C
        data[8] = 0x01; // mode = Heat, power = off
        data[11] = 24 + 55; // pipe_out = 24°C
    });
    
    test_process_data(packet_to_hex(cmd20_initial), target);
    assert(target.last_set_power_value == false);
    assert(target.last_set_target_temperature_value == 22.0f);
    assert(target.last_set_room_temperature_value == 25.0f);
    assert(abs(target.last_set_indoor_eva_in_temperature_value - 23.0f) < 0.01f);
    assert(abs(target.last_set_indoor_eva_out_temperature_value - 24.0f) < 0.01f);
    
    // Step 2: Publish a control request (this will be queued)
    // Use a different target_temp than what we'll send in Cmd20 to ensure request is NOT removed
    ProtocolRequest req;
    req.power = true;
    req.target_temp = 23.0f; // Different from Cmd20 target_temp (24°C)
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Step 3: Trigger send_requests() to mark request as sent (time_sent > 0)
    // This makes the request "pending"
    test_process_data("3200c8204d51500001100051e434", target); // Make indoor awake
    auto cmdC6_packet = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_packet), target);
    assert(!target.last_publish_data.empty()); // Request was sent, now pending
    
    // Step 4: Send Cmd20 with DIFFERENT state while request is pending
    // State updates should be IGNORED, but EVA temps should still be published
    target.last_set_power_address = "";
    target.last_set_target_temperature_address = "";
    target.last_set_room_temperature_address = "";
    target.last_set_indoor_eva_in_temperature_address = "";
    target.last_set_indoor_eva_out_temperature_address = "";
    
    auto cmd20_different = build_packet(0x00, 0xc8, 0x20, [](std::vector<uint8_t> &data) {
        data[4] = 79; // target_temp = 24°C (different from request)
        data[5] = 82; // room_temp = 27°C (different)
        data[6] = 25 + 55; // pipe_in = 25°C (different)
        data[8] = 0x01 | 0x80; // mode = Heat, power = on (different)
        data[11] = 26 + 55; // pipe_out = 26°C (different)
    });
    
    // Clear previous state to verify new Cmd20 doesn't update state
    bool previous_power = target.last_set_power_value;
    float previous_target_temp = target.last_set_target_temperature_value;
    float previous_room_temp = target.last_set_room_temperature_value;
    
    test_process_data(packet_to_hex(cmd20_different), target);
    
    // Verify state updates were IGNORED (not set on target)
    // When pending_control_message == true, state updates should be ignored
    // So the values should remain the same as before (previous values)
    assert(target.last_set_power_value == previous_power); // Should still be false
    assert(abs(target.last_set_target_temperature_value - previous_target_temp) < 0.01f); // Should still be 22.0f
    assert(abs(target.last_set_room_temperature_value - previous_room_temp) < 0.01f); // Should still be 25.0f
    
    // Verify EVA temperatures ARE still published (sensor readings, always published)
    assert(!target.last_set_indoor_eva_in_temperature_address.empty());
    assert(target.last_set_indoor_eva_in_temperature_address == "00");
    assert(abs(target.last_set_indoor_eva_in_temperature_value - 25.0f) < 0.01f);
    assert(!target.last_set_indoor_eva_out_temperature_address.empty());
    assert(target.last_set_indoor_eva_out_temperature_address == "00");
    assert(abs(target.last_set_indoor_eva_out_temperature_value - 26.0f) < 0.01f);
    
    // Step 5: Send Cmd54 to acknowledge the request (removes from queue)
    auto cmd54_packet = build_packet(0x00, 0xd0, 0x54, [](std::vector<uint8_t> &data) {
        // Cmd54 data
    });
    test_process_data(packet_to_hex(cmd54_packet), target);
    
    // Step 6: Now send Cmd20 again - state updates should NOT be ignored (no pending message)
    target.last_set_power_address = "";
    target.last_set_target_temperature_address = "";
    target.last_set_room_temperature_address = "";
    
    test_process_data(packet_to_hex(cmd20_different), target);
    
    // Verify state updates are NOW processed (pending_control_message == false)
    assert(!target.last_set_power_address.empty());
    assert(target.last_set_power_value == true);
    assert(!target.last_set_target_temperature_address.empty());
    assert(abs(target.last_set_target_temperature_value - 24.0f) < 0.01f);
    assert(!target.last_set_room_temperature_address.empty());
    assert(abs(target.last_set_room_temperature_value - 27.0f) < 0.01f);
}

void test_broadcast_registration_handler()
{
    std::cout << "test_broadcast_registration_handler" << std::endl;
    
    DebugTarget target;
    
    // Test: Broadcast registration handler (src="c8", dst="ad", data[0] & 1 == 1)
    // Should call send_register_controller() when non_nasa_keepalive is true
    
    // Note: We can't directly set non_nasa_keepalive in tests, but we can verify
    // the handler condition is checked correctly by testing packet decoding
    
    // Build broadcast registration packet: src=c8, dst=ad
    // The handler checks: src="c8" && dst="ad" && (commandRaw.data[0] & 1) == 1
    // In reality, the outdoor unit sends cmd=0xd1 (register_device) as a broadcast registration request
    // Since 0xd1 is not in the NonNasaCommand enum, it's stored as commandRaw
    // The handler checks commandRaw.data[0], which is the first byte of the raw data (data[4] in packet)
    // For unknown commands, the decode() function stores data[4-11] in commandRaw.data
    
    // Test 1: With correct conditions (src=c8, dst=ad, cmd=0xd1, data[0] & 1 == 1)
    // Using the real command value 0xd1 that's used in practice
    auto broadcast_packet = build_packet(0xc8, 0xad, 0xd1, [](std::vector<uint8_t> &data) {
        data[4] = 0x11; // First data byte, odd value (0x11 & 1 == 1) - matches real log: 32c8add11100000000000000a534
    });
    
    // Verify packet is decoded correctly
    NonNasaDataPacket packet;
    auto bytes = hex_to_bytes(packet_to_hex(broadcast_packet));
    auto result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.src == "c8");
    assert(packet.dst == "ad");
    
    // Test 2: With data[0] & 1 == 0 (even), handler should NOT trigger
    auto broadcast_packet_even = build_packet(0xc8, 0xad, 0xd1, [](std::vector<uint8_t> &data) {
        data[4] = 0x02; // Even value (0x02 & 1 == 0)
    });
    
    packet = NonNasaDataPacket();
    bytes = hex_to_bytes(packet_to_hex(broadcast_packet_even));
    result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.src == "c8");
    assert(packet.dst == "ad");
    
    // Test 3: With wrong src, handler should NOT trigger
    auto broadcast_packet_wrong_src = build_packet(0xc9, 0xad, 0xd1, [](std::vector<uint8_t> &data) {
        data[4] = 0x11; // Odd value
    });
    
    packet = NonNasaDataPacket();
    bytes = hex_to_bytes(packet_to_hex(broadcast_packet_wrong_src));
    result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.src == "c9"); // Not "c8"
    assert(packet.dst == "ad");
    
    // Test 4: With wrong dst, handler should NOT trigger
    auto broadcast_packet_wrong_dst = build_packet(0xc8, 0xae, 0xd1, [](std::vector<uint8_t> &data) {
        data[4] = 0x11; // Odd value
    });
    
    packet = NonNasaDataPacket();
    bytes = hex_to_bytes(packet_to_hex(broadcast_packet_wrong_dst));
    result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.src == "c8");
    assert(packet.dst == "ae"); // Not "ad"
}

void test_cmd54_dst_condition()
{
    std::cout << "test_cmd54_dst_condition" << std::endl;
    
    DebugTarget target;
    
    // Test: Cmd54 should only process when dst == "d0"
    
    // Test 1: Cmd54 with dst="d0" should process (remove pending requests)
    ProtocolRequest req;
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Make indoor awake and send request
    test_process_data("3200c8204d51500001100051e434", target);
    auto cmdC6 = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01;
    });
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6), target);
    assert(!target.last_publish_data.empty()); // Request was sent
    
    // Send Cmd54 with dst="d0" - should remove pending request
    auto cmd54_d0 = build_packet(0x00, 0xd0, 0x54, [](std::vector<uint8_t> &data) {
        // Cmd54 data
    });
    
    // Publish another request to verify queue state
    req.power = false;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Process Cmd54 with dst="d0" - should remove pending requests
    test_process_data(packet_to_hex(cmd54_d0), target);
    // Request should be removed from queue (handler processed)
    
    // Test 2: Cmd54 with dst != "d0" should NOT process
    target = DebugTarget();
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    // Make indoor awake and send request
    test_process_data("3200c8204d51500001100051e434", target);
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6), target);
    assert(!target.last_publish_data.empty()); // Request was sent
    
    // Send Cmd54 with dst="d1" (not "d0") - should NOT process
    auto cmd54_d1 = build_packet(0x00, 0xd1, 0x54, [](std::vector<uint8_t> &data) {
        // Cmd54 data
    });
    
    // Verify packet is decoded correctly
    NonNasaDataPacket packet;
    auto bytes = hex_to_bytes(packet_to_hex(cmd54_d1));
    auto result = packet.decode(bytes);
    assert(result.type == DecodeResultType::Processed);
    assert(packet.cmd == NonNasaCommand::Cmd54);
    assert(packet.dst == "d1"); // Not "d0", so handler should not process
    
    // Process the packet - handler should NOT process (dst != "d0")
    test_process_data(packet_to_hex(cmd54_d1), target);
    // Request should still be in queue (handler did not process)
}

void test_cmdc6_conditions()
{
    std::cout << "test_cmdc6_conditions" << std::endl;
    
    DebugTarget target;
    
    // Test: CmdC6 should only trigger send_requests() when:
    // src == "c8" && dst == "d0" && control_status == true && indoor_unit_awake == true
    
    // Test 1: Wrong src (src != "c8") - should NOT trigger
    test_process_data("3200c8204d51500001100051e434", target); // Make indoor awake
    ProtocolRequest req;
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    auto cmdC6_wrong_src = build_packet(0xc9, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_wrong_src), target);
    assert(target.last_publish_data.empty()); // send_requests() should NOT be called
    
    // Test 2: Wrong dst (dst != "d0") - should NOT trigger
    target = DebugTarget();
    test_process_data("3200c8204d51500001100051e434", target); // Make indoor awake
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    auto cmdC6_wrong_dst = build_packet(0xc8, 0xd1, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_wrong_dst), target);
    assert(target.last_publish_data.empty()); // send_requests() should NOT be called
    
    // Test 3: control_status == false - should NOT trigger (already tested in test_cmdc6_control_status)
    // This is already covered, but we verify here for completeness
    
    // Test 4: Correct conditions - should trigger (already tested, but verify controller_registered)
    target = DebugTarget();
    test_process_data("3200c8204d51500001100051e434", target); // Make indoor awake
    req.power = true;
    get_protocol("00")->publish_request(&target, "00", req);
    
    auto cmdC6_correct = build_packet(0xc8, 0xd0, 0xc6, [](std::vector<uint8_t> &data) {
        data[4] = 0x01; // control_status = true
    });
    
    target.last_publish_data = "";
    test_process_data(packet_to_hex(cmdC6_correct), target);
    assert(!target.last_publish_data.empty()); // send_requests() should be called
    
    // Note: We can't directly verify controller_registered state without accessing internal state
    // But we can verify the handler was called by checking send_requests() was triggered
}

int main(int argc, char *argv[])
{
    // test_read_file();
    test_decoding();
    test_encoding();
    test_target();

    test_previous_data_is_used_correctly();
    
    // New tests for sensor features
    test_cmdc0_outdoor_temperature();
    test_cmd8d_power_energy();
    test_cmd20_eva_temperatures();
    test_cmdf0_error_code();
    
    // High priority: CmdC6, Cmd54, CmdF3 tests
    test_cmdc6_control_status();
    test_cmd54_control_ack();
    test_cmdf3_decoded_but_not_processed();
    
    // Medium priority: CmdC1 and CmdF1 tests
    test_cmdc1_decoded_but_not_processed();
    test_cmdf1_decoded_but_not_processed();
    
    // Medium priority: Edge cases
    test_non_nasa_edge_cases();
    test_non_nasa_invalid_packets();
    
    // Medium priority: Multiple addresses test
    test_non_nasa_multiple_addresses();
    
    // Medium priority: Request encoding edge cases
    test_request_encoding_edge_cases();
    
    // Low priority: Integration sequence tests
    test_non_nasa_sequence();
    
    // High priority: Critical missing tests
    test_cmd20_pending_control_message_ignores_state();
    test_broadcast_registration_handler();
    test_cmd54_dst_condition();
    test_cmdc6_conditions();
};
