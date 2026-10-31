// Regression tests for the Non-NASA control request queue: merging Home Assistant calls into a
// single B0 control message per address, acknowledgement handling (Cmd54 and the Cmd20 backup
// ack), and commands that arrive across several protocol_update() cycles.
#include "test_stuff.h"
#include "../components/samsung_ac/protocol_non_nasa.h"

using namespace std;
using namespace esphome::samsung_ac;

// --- bus helpers ------------------------------------------------------------------------------

static const string C6_POLL = "32c8d0c60100040100000000da34"; // outdoor -> d0 request_control

static uint8_t checksum(const vector<uint8_t> &data)
{
    uint8_t crc = data[1];
    for (int i = 2; i < 12; i++)
        crc ^= data[i];
    return crc;
}

// Cmd20 status broadcast from indoor unit 00 (status encodings: NonNasaMode / NonNasaFanspeed)
static string cmd20(int target_temp, bool power, NonNasaMode mode, NonNasaFanspeed fan = NonNasaFanspeed::Auto,
                    NonNasaWindDirection wind = NonNasaWindDirection::Stop, int room_temp = 25)
{
    vector<uint8_t> data{0x32, 0x00, 0xc8, 0x20,
                         (uint8_t)(target_temp + 55), (uint8_t)(room_temp + 55), 0x50,
                         (uint8_t)(((uint8_t)wind << 3) | (uint8_t)fan),
                         (uint8_t)((power ? 0x80 : 0) | (uint8_t)mode),
                         0, 0, 0x50, 0, 0x34};
    data[12] = checksum(data);
    return bytes_to_hex(data);
}

// Cmd20 from an indoor unit that is off: wind/fan byte reads 0 (as seen on an AR09HSFSHWKNCV)
static string cmd20_off(int target_temp, NonNasaMode mode, int room_temp = 25)
{
    vector<uint8_t> data{0x32, 0x00, 0xc8, 0x20,
                         (uint8_t)(target_temp + 55), (uint8_t)(room_temp + 55), 0x50,
                         0, (uint8_t)mode, 0, 0, 0x50, 0, 0x34};
    data[12] = checksum(data);
    return bytes_to_hex(data);
}

// Cmd54 control acknowledgement from indoor unit 00: echoes data bytes 4-11 of the B0 it acknowledges
// (as seen on an AR09HSFSHWKNCV: b0 1f1c1801f4210000 -> 54 1f1c1801f4210000)
static string cmd54_for(const string &b0_hex)
{
    auto b0 = hex_to_bytes(b0_hex);
    vector<uint8_t> data{0x32, 0x00, 0xd0, 0x54, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x34};
    std::copy(b0.begin() + 4, b0.begin() + 12, data.begin() + 4);
    data[12] = checksum(data);
    return bytes_to_hex(data);
}

// Decoded B0 control message (request encodings)
struct Control
{
    bool power;
    int mode; // 0 auto, 1 cool, 2 dry, 3 fan, 4 heat
    int temp;
    int fan; // 0 auto, 64 low, 128 mid, 160 high, 224 turbo
    int wind;
};

static Control decode_b0(const string &hex)
{
    auto b = hex_to_bytes(hex);
    assert(b.size() == 14 && b[3] == 0xb0);
    return {(b[8] & 0xf0) == 0xf0, b[7], b[6] & 31, b[6] & 0xe0, b[4]};
}

static size_t seen = 0;
static vector<string> new_b0_frames(DebugTarget &target)
{
    vector<string> frames;
    for (; seen < target.all_publish_data.size(); seen++)
    {
        auto b = hex_to_bytes(target.all_publish_data[seen]);
        if (b.size() == 14 && b[3] == 0xb0)
            frames.push_back(target.all_publish_data[seen]);
    }
    return frames;
}

static Protocol *proto() { return get_protocol("00"); }

static void at(uint32_t ms) { esphome::test_millis_value = ms; }
static void advance(uint32_t ms) { esphome::test_millis_value += ms; }

// Let the outdoor unit poll us and send whatever is queued.
static vector<string> poll(DebugTarget &target)
{
    test_process_data(C6_POLL, target);
    proto()->protocol_update(&target);
    return new_b0_frames(target);
}

// Fresh state: unit 00 reports off / cool / 23 C / fan auto / swing stop.
static void reset(DebugTarget &target)
{
    advance(60000); // lets any acknowledged state from a previous test expire
    nonnasa_requests.clear();
    test_process_data(cmd20(23, false, NonNasaMode::Cool), target);
    poll(target);
    seen = target.all_publish_data.size();
}

static ProtocolRequest mode_request(Mode mode)
{
    ProtocolRequest r;
    r.mode = mode;
    return r;
}
static ProtocolRequest power_request(bool power)
{
    ProtocolRequest r;
    r.power = power;
    return r;
}
static ProtocolRequest temp_request(float temp)
{
    ProtocolRequest r;
    r.target_temp = temp;
    return r;
}
static ProtocolRequest fan_request(FanMode fan)
{
    ProtocolRequest r;
    r.fan_mode = fan;
    return r;
}
static ProtocolRequest swing_request(SwingMode swing)
{
    ProtocolRequest r;
    r.swing_mode = swing;
    return r;
}

static void publish(DebugTarget &target, ProtocolRequest request)
{
    proto()->publish_request(&target, "00", request);
}

// --- tests ------------------------------------------------------------------------------------

// HA scene: hvac_mode, temperature, swing and fan calls arrive back to back in one loop iteration.
void test_scene_single_cycle()
{
    cout << "test_scene_single_cycle" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    publish(target, temp_request(18));
    publish(target, swing_request(SwingMode::Vertical));
    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    assert(nonnasa_requests.size() == 1);

    auto frames = poll(target);
    assert(frames.size() == 1);
    auto c = decode_b0(frames[0]);
    assert(c.power && c.mode == 4 && c.temp == 18 && c.fan == 64 && c.wind == 0x1a);

    test_process_data(cmd54_for(frames[0]), target);
    assert(nonnasa_requests.empty());
}

// The same calls spread over several protocol_update() cycles before the next request_control poll.
void test_calls_across_cycles_before_send()
{
    cout << "test_calls_across_cycles_before_send" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    proto()->protocol_update(&target);
    advance(20);
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    advance(20);
    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    assert(nonnasa_requests.size() == 1);

    auto frames = poll(target);
    assert(frames.size() == 1);
    auto c = decode_b0(frames[0]);
    assert(c.power && c.mode == 4 && c.temp == 18 && c.fan == 64);
}

// A call arriving after the first B0 is on the wire, but before it is acknowledged. A second frame
// is unavoidable, but it must carry the complete merged state, and the late Cmd54 for the first
// frame must not remove the second.
void test_call_after_send_with_delayed_ack()
{
    cout << "test_call_after_send_with_delayed_ack" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    proto()->protocol_update(&target);
    auto first = poll(target);
    assert(first.size() == 1);

    advance(20);
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    assert(nonnasa_requests.size() == 1);
    assert(nonnasa_requests.front().time_sent == 0); // merged request waits for the next poll

    // Ack for the first frame arrives while the merged request is queued: nothing removed
    test_process_data(cmd54_for(first[0]), target);
    assert(nonnasa_requests.size() == 1);

    advance(900); // next request_control poll
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(c.power && c.mode == 4 && c.temp == 18);

    // A duplicate/late ack for the first frame after the second has been sent: still not removed
    test_process_data(cmd54_for(first[0]), target);
    assert(nonnasa_requests.size() == 1);

    // Only the ack for the merged frame removes it
    test_process_data(cmd54_for(second[0]), target);
    assert(nonnasa_requests.empty());
}

// A call arriving after the first B0 has been acknowledged, but before the indoor unit's Cmd20
// reflects it. The new frame must build on the acknowledged state, not the stale Cmd20.
void test_call_after_ack_before_cmd20()
{
    cout << "test_call_after_ack_before_cmd20" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    auto first = poll(target);
    test_process_data(cmd54_for(first[0]), target);
    assert(nonnasa_requests.empty());

    // Cmd20 broadcast sent before the unit applied the change (still off / cool / 23)
    advance(200);
    test_process_data(cmd20(23, false, NonNasaMode::Cool), target);

    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(c.power && c.mode == 4 && c.temp == 18 && c.fan == 64);
}

// Once Cmd20 reflects the acknowledged state, Cmd20 is the base again, so a later change made
// with the IR remote isn't reverted by the next Home Assistant command.
void test_remote_change_after_ack_is_kept()
{
    cout << "test_remote_change_after_ack_is_kept" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    auto first = poll(target);
    test_process_data(cmd54_for(first[0]), target);

    advance(1000);
    test_process_data(cmd20(18, true, NonNasaMode::Heat), target); // applied
    advance(1500);
    test_process_data(cmd20(26, true, NonNasaMode::Heat), target); // IR remote: 26

    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(c.power && c.mode == 4 && c.temp == 26 && c.fan == 64);
}

// An indoor unit that is off reports its fan/swing byte as 0 (fan auto, swing stop), so an "off"
// request with another fan speed never matches Cmd20. Any change in Cmd20 after the acknowledgement
// still makes Cmd20 the base again, so turning the unit back on with the remote isn't undone by the
// next Home Assistant command.
void test_off_then_remote_on()
{
    cout << "test_off_then_remote_on" << endl;
    DebugTarget target;
    reset(target);
    test_process_data(cmd20(23, true, NonNasaMode::Heat, NonNasaFanspeed::Low), target);

    publish(target, power_request(false));
    proto()->protocol_update(&target);
    auto first = poll(target);
    assert(first.size() == 1 && !decode_b0(first[0]).power && decode_b0(first[0]).fan == 64);
    test_process_data(cmd54_for(first[0]), target);

    advance(300);
    test_process_data(cmd20(23, true, NonNasaMode::Heat, NonNasaFanspeed::Low), target); // not applied yet
    advance(1500);
    test_process_data(cmd20_off(23, NonNasaMode::Heat), target); // off
    advance(1500);
    test_process_data(cmd20(25, true, NonNasaMode::Heat, NonNasaFanspeed::Low), target); // remote: on, 25

    publish(target, temp_request(21));
    proto()->protocol_update(&target);
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(c.power && c.mode == 4 && c.temp == 21 && c.fan == 64);
}

// If Cmd20 neither reflects the acknowledged state nor changes at all, the acknowledged state stops
// being used as the base after a timeout.
void test_acknowledged_state_expires()
{
    cout << "test_acknowledged_state_expires" << endl;
    DebugTarget target;
    reset(target); // off / cool / 23

    publish(target, mode_request(Mode::Heat));
    proto()->protocol_update(&target);
    auto first = poll(target);
    test_process_data(cmd54_for(first[0]), target);

    for (int i = 0; i < 4; i++)
    {
        advance(1500);
        test_process_data(cmd20(23, false, NonNasaMode::Cool), target); // unit reports no change
    }

    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(!c.power && c.mode == 1 && c.temp == 23 && c.fan == 64);
}

// No Cmd54 at all (lost or not sent by this unit): the Cmd20 backup ack still clears the request.
void test_cmd20_backup_ack()
{
    cout << "test_cmd20_backup_ack" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    auto frames = poll(target);
    assert(frames.size() == 1);

    advance(1500);
    test_process_data(cmd20(23, false, NonNasaMode::Cool), target); // not applied yet
    assert(nonnasa_requests.size() == 1);
    advance(1500);
    test_process_data(cmd20(18, true, NonNasaMode::Heat), target);
    assert(nonnasa_requests.empty());
}

// A request that was only ever sent in one version accepts any Cmd54 from the device, as before, in
// case a unit doesn't echo the control message exactly.
void test_non_echo_ack_clears_request()
{
    cout << "test_non_echo_ack_clears_request" << endl;
    DebugTarget target;
    reset(target);

    publish(target, temp_request(19));
    proto()->protocol_update(&target);
    auto frames = poll(target);
    assert(frames.size() == 1);

    test_process_data(cmd54_for("32d000b01f1c1801f4210000af34"), target); // payload doesn't match
    assert(nonnasa_requests.empty());
}

// Request A is sent and acknowledged, then request B is sent. A delayed or duplicate Cmd54 for A
// arriving while B awaits acknowledgement must not remove B.
void test_stale_ack_after_previous_request_acked()
{
    cout << "test_stale_ack_after_previous_request_acked" << endl;
    DebugTarget target;
    reset(target);

    publish(target, temp_request(19));
    proto()->protocol_update(&target);
    auto a = poll(target);
    test_process_data(cmd54_for(a[0]), target);
    assert(nonnasa_requests.empty());

    advance(800);
    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    auto b = poll(target);
    assert(b.size() == 1 && b[0] != a[0]);

    test_process_data(cmd54_for(a[0]), target); // duplicate ack for A
    assert(nonnasa_requests.size() == 1);
    test_process_data(cmd54_for(b[0]), target);
    assert(nonnasa_requests.empty());
}

// A unit that doesn't echo: its Cmd54 matches nothing we sent, so it is accepted for the request
// awaiting acknowledgement even after that request was replaced, rather than leaving it to resends.
void test_non_echo_ack_after_replace()
{
    cout << "test_non_echo_ack_after_replace" << endl;
    DebugTarget target;
    reset(target);

    publish(target, temp_request(19));
    proto()->protocol_update(&target);
    poll(target);
    advance(20);
    publish(target, fan_request(FanMode::Low));
    proto()->protocol_update(&target);
    advance(700);
    auto second = poll(target);
    assert(second.size() == 1);

    test_process_data(cmd54_for("32d000b00000000000000000000034"), target); // no echo
    assert(nonnasa_requests.empty());
}

// Cmd20 reports an intermediate state (power and mode applied, temperature not yet). Fields Cmd20
// hasn't changed yet keep the acknowledged value, so the next command doesn't revert the temperature.
void test_intermediate_cmd20_keeps_acknowledged_fields()
{
    cout << "test_intermediate_cmd20_keeps_acknowledged_fields" << endl;
    DebugTarget target;
    reset(target); // off / cool / 23

    publish(target, mode_request(Mode::Heat));
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    auto first = poll(target);
    test_process_data(cmd54_for(first[0]), target);

    advance(1500);
    test_process_data(cmd20(23, true, NonNasaMode::Heat), target); // on + heat applied, still 23

    publish(target, swing_request(SwingMode::Vertical));
    proto()->protocol_update(&target);
    auto second = poll(target);
    assert(second.size() == 1);
    auto c = decode_b0(second[0]);
    assert(c.power && c.mode == 4 && c.temp == 18 && c.wind == 0x1a);
}

// Unacknowledged frames are resent with the merged state.
void test_resend_carries_merged_state()
{
    cout << "test_resend_carries_merged_state" << endl;
    DebugTarget target;
    reset(target);

    publish(target, mode_request(Mode::Heat));
    proto()->protocol_update(&target);
    poll(target);
    advance(20);
    publish(target, temp_request(18));
    proto()->protocol_update(&target);
    advance(500);
    auto second = poll(target);
    assert(second.size() == 1);

    // No ack and no matching Cmd20: resent after 4.5 s, still with the merged state
    advance(4600);
    test_process_data(cmd20(23, false, NonNasaMode::Cool), target);
    proto()->protocol_update(&target);
    auto resent = poll(target);
    assert(resent.size() == 1);
    assert(resent[0] == second[0]);
}

int main(int argc, char *argv[])
{
    at(1000);
    test_scene_single_cycle();
    test_calls_across_cycles_before_send();
    test_call_after_send_with_delayed_ack();
    test_call_after_ack_before_cmd20();
    test_remote_change_after_ack_is_kept();
    test_off_then_remote_on();
    test_acknowledged_state_expires();
    test_cmd20_backup_ack();
    test_non_echo_ack_clears_request();
    test_stale_ack_after_previous_request_acked();
    test_non_echo_ack_after_replace();
    test_intermediate_cmd20_keeps_acknowledged_fields();
    test_resend_carries_merged_state();
    cout << "ALL NON NASA QUEUE TESTS PASSED" << endl;
    return 0;
}
