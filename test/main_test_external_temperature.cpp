#include "test_stuff.h"
#include "../components/samsung_ac/external_room_temperature.h"
#include <limits>

class ExternalTarget : public DebugTarget
{
public:
    uint32_t now = 0;
    bool ready = true;
    bool full = false;
    struct Pending { vector<uint8_t> bytes; function<bool()> guard; function<void()> sent; };
    vector<Pending> pending;
    vector<Packet> sent;
    uint32_t get_miliseconds() override { return now; }
    bool is_bus_ready() override { return ready; }
    bool publish_data_once_guarded(uint8_t, vector<uint8_t> &&bytes, function<bool()> guard, function<void()> callback) override
    {
        if (full) return false;
        pending.push_back({std::move(bytes), std::move(guard), std::move(callback)});
        return true;
    }
    void deliver()
    {
        assert(!pending.empty());
        auto item = std::move(pending.front());
        pending.erase(pending.begin());
        if (!item.guard()) return;
        Packet packet;
        assert(packet.decode(item.bytes).type == DecodeResultType::Processed);
        sent.push_back(packet);
        item.sent();
    }
};

static Packet message(const string &source, MessageNumber number, int value, DataType type = DataType::Notification)
{
    auto p = Packet::create(Address::parse("b0.ff.ff"), type, number, value);
    p.sa = Address::parse(source);
    return p;
}

// All model input uses the real CRC-checked decoder, without scanning arbitrary byte offsets.
static void observe(ExternalRoomTemperature &model, Packet p, bool own = false)
{
    auto bytes = p.encode();
    Packet decoded;
    assert(decoded.decode(bytes).type == DecodeResultType::Processed);
    model.observe(decoded, own);
}

struct Fixture
{
    ExternalTarget target;
    ExternalRoomTemperature model{&target, "20.00.01", "50.00.00"};
    explicit Fixture(uint32_t start = 0, Mode mode = Mode::Cool)
    {
        target.now = start;
        model.options.observe_for = 30000;
        model.begin();
        model.update();
        target.now += 30000;
        status(mode);
        model.receive_sample(20.0f);
        model.update();
    }
    void status(Mode mode = Mode::Cool)
    {
        observe(model, message("20.00.01", MessageNumber::ENUM_in_operation_power, 1));
        observe(model, message("20.00.01", MessageNumber::ENUM_in_operation_mode, static_cast<int>(mode)));
    }
    void arm(float value = 20.0f)
    {
        assert(model.enable(true));
        model.receive_sample(value);
        model.update();
        assert(target.pending.size() == 1);
    }
    Packet send(float value = 20.0f)
    {
        arm(value);
        target.deliver();
        assert(target.sent.size() == 1);
        return target.sent.back();
    }
    Packet response(DataType type = DataType::Response)
    {
        assert(!target.sent.empty());
        auto p = target.sent.back();
        std::swap(p.sa, p.da);
        p.command.dataType = type;
        if (type == DataType::Ack || type == DataType::Nack) p.messages.clear();
        return p;
    }
    void echo() { observe(model, response()); }
    void report(int tenths = 200) { observe(model, message("20.00.01", MessageNumber::VAR_in_temp_room_f, tenths)); }
    void reason(const char *expected)
    {
        assert(!model.state().enabled);
        assert(string(model.state().reason) == expected);
        assert(!model.state().reported_match);
    }
};

static void disabled_and_manual_sample()
{
    Fixture f;
    f.target.now += 60000;
    f.model.update();
    assert(f.target.pending.empty());
    assert(f.model.enable(true));
    f.model.update();
    assert(f.target.pending.empty()); // Cached measurement is insufficient after arming.
    f.model.receive_sample(20);
    f.model.update();
    assert(f.target.pending.size() == 1);
    f.model.enable(false);
    f.target.deliver();
    assert(f.target.sent.empty());
    Fixture reboot;
    assert(!reboot.model.state().enabled);
}

static void wire_and_signed_encoding()
{
    Fixture f;
    auto p = f.send(21.26f);
    assert(p.sa.to_string() == "50.00.00");
    assert(p.da.to_string() == "20.00.01");
    assert(static_cast<int>(p.command.dataType) == 2);
    assert(p.messages.size() == 1 && p.messages[0].messageNumber == MessageNumber::VAR_in_temp_external_f);
    assert(p.messages[0].type == MessageSetType::Variable && p.messages[0].value == 213);
    auto bytes = p.encode();
    bytes[15] ^= 1;
    Packet invalid;
    assert(invalid.decode(bytes).type == DecodeResultType::Discard);
    Fixture negative;
    negative.model.options.minimum = -20;
    negative.model.options.max_step = 20;
    negative.model.receive_sample(0);
    negative.model.receive_sample(-5);
    negative.send(-5);
    assert(static_cast<int16_t>(negative.target.sent.back().messages[0].value) == -50);
    assert(Address::get_my_address().klass == AddressClass::JIGTester);
}

static void cool_echo_then_report()
{
    Fixture f;
    f.send();
    f.report(260);
    assert(f.model.state().phase == ExternalTemperaturePhase::AwaitEcho);
    f.echo();
    assert(f.model.state().phase == ExternalTemperaturePhase::AwaitReport);
    f.target.now += 29000;
    f.report();
    assert(f.model.state().phase == ExternalTemperaturePhase::Following);
    assert(f.model.state().reported_match && f.model.state().enabled);
    f.model.update();
    assert(f.target.pending.empty());
    f.target.now += 16000;
    f.model.receive_sample(20);
    f.model.update();
    assert(f.target.pending.size() == 1); // One refresh, no duplicate or retry.
}

static void report_then_echo()
{
    Fixture f;
    f.send();
    f.report();
    assert(f.model.state().phase == ExternalTemperaturePhase::AwaitEcho);
    f.echo();
    assert(f.model.state().phase == ExternalTemperaturePhase::Following);
    f.report(220);
    f.reason("reported_temperature_diverged");
}

static void ack_is_not_acceptance()
{
    Fixture f;
    f.send();
    observe(f.model, f.response(DataType::Ack));
    f.report();
    assert(f.model.state().phase == ExternalTemperaturePhase::AwaitEcho);
    f.target.now += 5000;
    f.model.update();
    f.reason("response_timeout");
    assert(f.target.sent.size() == 1 && f.target.pending.empty());
    f.model.receive_sample(20);
    f.model.update();
    assert(!f.model.state().enabled); // No automatic recovery.
}

static void correlation()
{
    for (unsigned mutation = 0; mutation < 8; ++mutation)
    {
        Fixture f;
        f.send();
        auto p = f.response();
        if (mutation == 0) p.sa = Address::parse("20.00.02");
        if (mutation == 1) p.da = Address::parse("80.00.00");
        if (mutation == 2) ++p.command.packetNumber;
        if (mutation == 3) p.command.dataType = DataType::Notification;
        if (mutation == 4) p.messages[0].messageNumber = MessageNumber::VAR_in_temp_target_f;
        if (mutation == 5) p.command.packetType = PacketType::Install;
        if (mutation == 6) p.command.protocolVersion = 1;
        if (mutation == 7) p.command.packetInformation = false;
        observe(f.model, p);
        assert(f.model.state().phase == ExternalTemperaturePhase::AwaitEcho);
        f.echo();
        f.report();
        assert(f.model.state().phase == ExternalTemperaturePhase::Following);
    }
}

static void rejected_and_unsupported()
{
    for (int value : {199, 0xFFFF})
    {
        Fixture f;
        f.send();
        auto p = f.response();
        p.messages[0].value = value;
        observe(f.model, p);
        f.reason("echo_mismatch_or_unsupported");
    }
    Fixture nack;
    nack.send();
    observe(nack.model, nack.response(DataType::Nack));
    nack.reason("nack");
}

static void late_response()
{
    Fixture f;
    f.send();
    f.target.now += 5000;
    f.echo(); // Must reject even before the component loop checks the timeout.
    f.reason("response_timeout");
}

static void heating_does_not_follow()
{
    Fixture f(0, Mode::Heat);
    f.send();
    f.echo();
    f.report(260);
    f.target.now += 90000;
    f.model.update();
    f.reason("reported_temperature_not_following");
    assert(f.target.sent.size() == 1); // No speculative sensor-selection/fallback writes.
}

static void late_report()
{
    Fixture f;
    f.send();
    f.echo();
    f.target.now += 90000;
    f.report();
    f.reason("reported_temperature_not_following");
}

static void invalid_measurements()
{
    for (float value : {NAN, INFINITY, -INFINITY, 4.9f, 40.1f, 26.0f})
    {
        Fixture f;
        f.send();
        f.model.receive_sample(value);
        assert(!f.model.state().enabled && !f.model.state().fresh);
        assert(f.model.state().failures == 1);
    }
    Fixture sentinel;
    sentinel.model.options.minimum = -20;
    sentinel.model.receive_sample(-0.1f);
    assert(!sentinel.model.state().fresh);
}

static void freshness_and_rollover()
{
    Fixture f;
    f.arm();
    f.target.now += 120001;
    f.target.deliver();
    f.reason("sensor_stale");
    assert(f.target.sent.empty());
    Fixture wrap(UINT32_MAX - 32000);
    wrap.send();
    wrap.target.now += 3000; // millis wraps while waiting for the response.
    wrap.echo();
    wrap.report();
    assert(wrap.model.state().phase == ExternalTemperaturePhase::Following);
    wrap.target.now += 120001;
    wrap.model.update();
    wrap.reason("sensor_stale");
}

static void queue_expiry_and_capacity()
{
    Fixture f;
    f.arm();
    f.target.now += 5000;
    f.target.deliver();
    f.reason("queue_timeout");
    assert(f.target.sent.empty());
    Fixture full;
    full.target.full = true;
    assert(full.model.enable(true));
    full.model.receive_sample(20);
    full.model.update();
    full.reason("queue_full_or_transport_unsupported");
    Fixture stalled;
    stalled.arm();
    stalled.target.now += 5000;
    stalled.model.update();
    stalled.reason("queue_timeout");
    stalled.target.deliver();
    assert(stalled.target.sent.empty());
}

static void queued_sample_and_delayed_delivery()
{
    Fixture f;
    f.arm();
    f.target.now += 4000;
    f.target.deliver();
    f.target.now += 1000;
    f.model.update();
    assert(f.model.state().phase == ExternalTemperaturePhase::AwaitEcho); // Deadline from UART, not enqueue.
    Fixture newer;
    newer.arm();
    newer.model.receive_sample(21);
    newer.target.deliver();
    assert(newer.target.sent.empty());
    newer.model.update();
    newer.target.deliver();
    assert(newer.target.sent.back().messages[0].value == 210);
}

static void bus_and_operation_gates()
{
    Fixture f;
    f.arm();
    f.target.ready = false;
    f.target.deliver();
    f.reason("bus_not_ready");
    for (auto mode : {Mode::Auto, Mode::Dry, Mode::Fan})
    {
        Fixture unsupported(0, mode);
        assert(!unsupported.model.enable(true));
        unsupported.reason("unsupported_mode");
    }
    Fixture unknown;
    unknown.target.now += 120001;
    unknown.model.receive_sample(20);
    assert(!unknown.model.enable(true));
    unknown.reason("operating_state_stale");
}

static void power_and_mode_changes()
{
    for (auto pair : {std::make_pair(MessageNumber::ENUM_in_operation_power, 0),
                      std::make_pair(MessageNumber::ENUM_in_operation_mode, 4),
                      std::make_pair(MessageNumber::ENUM_in_operation_power, 255),
                      std::make_pair(MessageNumber::ENUM_in_operation_mode, 255)})
    {
        Fixture f;
        f.arm();
        observe(f.model, message("20.00.01", pair.first, pair.second));
        assert(!f.model.state().enabled);
        f.target.deliver();
        assert(f.target.sent.empty());
        f.status();
        f.model.update();
        assert(!f.model.state().enabled);
    }
}

static void controller_conflicts()
{
    for (const string source : {"50.00.01", "62.00.64", "50.00.00"})
    {
        Fixture f;
        f.arm();
        observe(f.model, message(source, MessageNumber::VAR_in_temp_external_f, 200, DataType::Write));
        f.reason("controller_conflict");
        f.target.deliver();
        assert(f.target.sent.empty());
        f.model.enable(false);
        assert(!f.model.enable(true)); // Latched until reboot and manual source review.
    }
    Fixture passive;
    observe(passive.model, message("62.00.64", MessageNumber::ENUM_in_operation_power, 1));
    assert(!passive.model.enable(true));
    Fixture own;
    auto packet = own.send();
    assert(own.model.owns_loopback(packet));
    observe(own.model, packet, true);
    assert(own.model.state().enabled);
    packet.command.packetType = PacketType::Install;
    assert(!own.model.owns_loopback(packet));
}

static void observation_and_configuration()
{
    ExternalTarget t;
    ExternalRoomTemperature m(&t, "20.00.01", "50.00.00");
    m.begin();
    t.ready = false;
    m.update();
    t.now = 100000;
    t.ready = true;
    m.update();
    m.receive_sample(20);
    assert(!m.enable(true));
    assert(string(m.state().reason) == "observing");
    for (const auto &source : {"80.00.00", "50.ff.00", "50.00.ff", "bad"})
    {
        ExternalRoomTemperature invalid(&t, "20.00.01", source);
        invalid.begin();
        assert(!invalid.enable(true));
        assert(string(invalid.state().reason) == "invalid_configuration");
    }
    ExternalRoomTemperature non_nasa(&t, "00", "50.00.00");
    non_nasa.begin();
    assert(!non_nasa.enable(true));
}

static void session_cancellation_and_reentrancy()
{
    Fixture f;
    f.arm();
    f.model.enable(false);
    assert(f.model.enable(true));
    f.model.receive_sample(21);
    f.model.update();
    assert(f.target.pending.size() == 2);
    f.target.deliver(); // An old generation must not send after rearm.
    assert(f.target.sent.empty());
    f.target.deliver();
    assert(f.target.sent.size() == 1 && f.target.sent[0].messages[0].value == 210);
    Fixture callback;
    callback.model.on_change = [&callback](const ExternalTemperatureState &state) {
        if (state.phase == ExternalTemperaturePhase::Queued) callback.model.enable(false);
    };
    assert(callback.model.enable(true));
    callback.model.receive_sample(20);
    callback.model.update();
    assert(callback.target.pending.empty() && !callback.model.state().enabled);
}

static void operating_fields_before_evidence()
{
    for (const auto number : {MessageNumber::ENUM_in_operation_power, MessageNumber::ENUM_in_operation_mode})
    {
        Fixture f;
        f.send();
        f.echo();
        unsigned following_events = 0;
        f.model.on_change = [&](const ExternalTemperatureState &state) {
            if (state.phase == ExternalTemperaturePhase::Following) ++following_events;
        };
        auto p = message("20.00.01", MessageNumber::VAR_in_temp_room_f, 200);
        MessageSet operation(number);
        operation.value = 0; // OFF or mode Auto. Even though temperature appears first on the wire.
        p.messages.push_back(operation);
        observe(f.model, p);
        assert(!f.model.state().enabled && !f.model.state().reported_match && following_events == 0);
    }
}

static void multi_device_and_decoder()
{
    Fixture f;
    ExternalRoomTemperature second(&f.target, "20.00.02", "50.00.00");
    second.begin();
    auto p = f.send();
    NasaProtocol protocol;
    protocol.register_external_temperature(&f.model);
    protocol.register_external_temperature(&second);
    protocol.process_external_packet(p); // Own echo is recognized across configured devices.
    assert(f.model.state().enabled && second.state().failures == 0);
    observe(f.model, message("20.00.02", MessageNumber::VAR_in_temp_room_f, 200));
    assert(std::isnan(f.model.state().reported));
    // End-to-end process_data observer hook is only reached for valid NASA frames.
    auto *shared = static_cast<NasaProtocol *>(get_protocol("20.00.01"));
    shared->register_external_temperature(&f.model);
    auto bytes = message("62.00.64", MessageNumber::ENUM_in_operation_power, 1).encode();
    protocol_processing = ProtocolProcessing::NASA;
    bytes[15] ^= 1;
    assert(process_data(bytes, &f.target).type == DecodeResultType::Discard);
    assert(f.model.state().enabled);
    bytes[15] ^= 1;
    assert(process_data(bytes, &f.target).type == DecodeResultType::Processed);
    f.reason("controller_conflict");
    // This is the final case: the shared protocol holds the fixture pointer until process exit.
}

int main()
{
    using Test = pair<const char *, void (*)()>;
    const Test tests[] = {
        {"disabled/manual sample/reboot", disabled_and_manual_sample}, {"wire/signed encoding/CRC", wire_and_signed_encoding},
        {"cool echo/report/cadence", cool_echo_then_report}, {"report first/divergence", report_then_echo},
        {"ACK/no retry/no recovery", ack_is_not_acceptance}, {"strict correlation", correlation},
        {"NACK/unsupported/mismatch", rejected_and_unsupported}, {"late echo", late_response},
        {"heating does not follow", heating_does_not_follow}, {"late report", late_report},
        {"invalid measurements", invalid_measurements}, {"freshness/rollover", freshness_and_rollover},
        {"queue expiry/capacity", queue_expiry_and_capacity}, {"send deadline/superseded sample", queued_sample_and_delayed_delivery},
        {"bus/mode/status gates", bus_and_operation_gates}, {"power/mode changes", power_and_mode_changes},
        {"controller conflicts/own echo", controller_conflicts}, {"observation/configuration", observation_and_configuration},
        {"session cancellation/reentrancy", session_cancellation_and_reentrancy},
        {"operating fields precede evidence", operating_fields_before_evidence},
        {"multiple units/decoder integration", multi_device_and_decoder},
    };
    for (const auto &test : tests) { test.second(); cout << "PASS " << test.first << endl; }
    cout << "PASS " << std::size(tests) << " external temperature regression groups" << endl;
}
