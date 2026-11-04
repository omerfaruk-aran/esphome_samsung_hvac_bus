#include "test_stuff.h"
#include "../components/samsung_ac/protocol_nasa.h"

class DisplayTarget : public DebugTarget
{
public:
    uint32_t now = 0;
    bool delay_delivery = false;
    vector<function<void()>> deliveries;
    vector<uint8_t> single_transmissions;
    vector<pair<string, esphome::optional<bool>>> states;
    function<void()> on_invalid;
    unsigned acks = 0;
    uint32_t get_miliseconds() override { return now; }
    void ack_data(uint8_t) override { acks++; }
    void set_display_light(const string &address, esphome::optional<bool> value) override
    {
        states.emplace_back(address, value);
        if (!value.has_value() && on_invalid)
        {
            auto callback = std::move(on_invalid);
            on_invalid = {};
            callback();
        }
    }
    void publish_data_once(uint8_t id, vector<uint8_t> &&data, function<void()> sent) override
    {
        single_transmissions.push_back(id);
        publish_data(id, std::move(data));
        if (delay_delivery)
            deliveries.push_back(std::move(sent));
        else
            sent();
    }
    Packet last_packet()
    {
        auto data = hex_to_bytes(last_publish_data);
        Packet packet;
        assert(packet.decode(data).type == DecodeResultType::Processed);
        return packet;
    }
    Packet packet_at(unsigned index)
    {
        auto data = hex_to_bytes(all_publish_data.at(index));
        Packet packet;
        assert(packet.decode(data).type == DecodeResultType::Processed);
        return packet;
    }
    unsigned confirmed() const
    {
        unsigned count = 0;
        for (const auto &state : states)
            count += state.second.has_value();
        return count;
    }
};

static Packet reply(Packet request, DataType type, int value = 0)
{
    std::swap(request.sa, request.da);
    request.command.dataType = type;
    if (type == DataType::Ack || type == DataType::Nack)
        request.messages.clear();
    else
        request.messages[0].value = value;
    return request;
}

static void receive(DisplayTarget &target, Packet packet)
{
    protocol_processing = ProtocolProcessing::NASA;
    test_process_data(bytes_to_hex(packet.encode()), target);
}

// Local instances isolate scheduler tests; the decoder integration uses the shared protocol.
static void handle(NasaProtocol &protocol, DisplayTarget &target, Packet packet)
{
    auto bytes = packet.encode();
    Packet decoded;
    assert(decoded.decode(bytes).type == DecodeResultType::Processed);
    protocol.process_display_packet(&target, decoded);
}

static void test_encoding()
{
    auto read = Packet::create(Address::parse("20.00.02"), DataType::Read, MessageNumber::ENUM_in_display_light, 0);
    read.sa = Address::parse("62.ff.00");
    read.command.packetNumber = 1;
    assert_str(bytes_to_hex(read.encode()), "32001162ff00200002c0110101401600378b34");
    read.command.dataType = DataType::Request;
    read.command.packetNumber = 2;
    assert_str(bytes_to_hex(read.encode()), "32001162ff00200002c0130201401600521934");
    read.messages[0].value = 1;
    auto bytes = read.encode();
    Packet decoded;
    assert(decoded.decode(bytes).type == DecodeResultType::Processed);
    assert(decoded.messages[0].type == MessageSetType::Enum);
    assert(decoded.messages[0].value == 1);
    assert(decoded.da.to_string() == "20.00.02");
    bytes[15] ^= 1;
    assert(decoded.decode(bytes).type == DecodeResultType::Discard);
}

static void test_reads()
{
    NasaProtocol protocol;
    DisplayTarget target;
    protocol.enable_display_light("20.00.02");
    protocol.protocol_update(&target);
    auto read = target.last_packet();
    assert(read.command.dataType == DataType::Read);
    auto wrong_source = reply(read, DataType::Response, 1);
    wrong_source.sa = Address::parse("20.00.03");
    handle(protocol, target, wrong_source);
    auto wrong_dest = reply(read, DataType::Response, 1);
    wrong_dest.da = Address::parse("62.ff.00");
    handle(protocol, target, wrong_dest);
    auto wrong_id = reply(read, DataType::Response, 1);
    wrong_id.command.packetNumber++;
    handle(protocol, target, wrong_id);
    auto wrong_message = reply(read, DataType::Response, 1);
    wrong_message.messages[0] = MessageSet(MessageNumber::ENUM_in_operation_power);
    wrong_message.messages[0].value = 1;
    handle(protocol, target, wrong_message);
    for (auto type : {DataType::Notification, DataType::Request, DataType::Write, DataType::Ack})
        handle(protocol, target, reply(read, type, 1));
    assert(target.confirmed() == 0);
    handle(protocol, target, reply(read, DataType::Response, 0));
    assert(target.confirmed() == 1 && !target.states.back().second.value());
    handle(protocol, target, reply(read, DataType::Response, 1));
    assert(target.confirmed() == 1); // Duplicate cannot overwrite the completed read.
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 1);

    for (int value : {1, 0xff, 2})
    {
        NasaProtocol other;
        DisplayTarget device;
        other.enable_display_light("20.01.03");
        other.protocol_update(&device);
        handle(other, device, reply(device.last_packet(), DataType::Response, value));
        assert(device.confirmed() == (value == 1 ? 1u : 0u));
        assert(device.states.back().second.has_value() == (value == 1));
    }
}

static void test_commands_and_merging()
{
    for (bool on : {false, true})
    {
        NasaProtocol protocol;
        DisplayTarget target;
        protocol.enable_display_light("20.00.02");
        ProtocolRequest display, climate, fan;
        display.display_light = !on;
        protocol.publish_request(&target, "20.00.02", display);
        display.display_light = on;
        protocol.publish_request(&target, "20.00.02", display);
        climate.mode = Mode::Cool;
        climate.target_temp = 23;
        protocol.publish_request(&target, "20.00.02", climate);
        fan.fan_mode = FanMode::Mid;
        fan.automatic_cleaning = false;
        fan.filter_reset = true;
        fan.alt_mode = 3;
        fan.swing_mode = SwingMode::All;
        protocol.publish_request(&target, "20.00.02", fan);
        protocol.protocol_update(&target);
        auto command = target.packet_at(0);
        assert(target.all_publish_data.size() == 2);
        assert(target.single_transmissions.size() == 1); // Climate keeps the standard ACK/retry transport.
        assert(command.command.dataType == DataType::Request);
        assert(command.da.to_string() == "20.00.02");
        assert(command.messages.size() == 1);
        assert(command.messages[0].messageNumber == MessageNumber::ENUM_in_display_light);
        assert(command.messages[0].value == (on ? 1 : 0));
        auto merged = target.packet_at(1);
        assert(merged.messages.size() == 9);
        assert(merged.messages[0].messageNumber == MessageNumber::ENUM_in_operation_mode);
        assert(merged.messages[1].messageNumber == MessageNumber::ENUM_in_operation_power);
        assert(merged.messages[1].value == 1);
        assert(merged.messages[2].value == 0); // False cleaning optional survives merging.
        handle(protocol, target, reply(command, DataType::Ack));
        assert(target.confirmed() == 0);
        target.now = 499;
        protocol.protocol_update(&target);
        assert(target.all_publish_data.size() == 2);
        target.now = 500;
        protocol.protocol_update(&target);
        auto read = target.last_packet();
        assert(read.command.dataType == DataType::Read);
        handle(protocol, target, reply(command, DataType::Response, on ? 1 : 0));
        assert(target.confirmed() == 0);
        handle(protocol, target, reply(read, DataType::Response, on ? 0 : 1));
        assert(target.states.back().second.value() == !on); // Actual device value wins.
    }
}

static void test_timeouts_and_delivery()
{
    NasaProtocol protocol;
    DisplayTarget target;
    target.now = UINT32_MAX - 1000;
    target.delay_delivery = true;
    protocol.enable_display_light("20.00.02");
    protocol.protocol_update(&target);
    const auto read = target.last_packet();
    target.now += 10000;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 1);
    handle(protocol, target, reply(read, DataType::Response, 1));
    assert(target.confirmed() == 0); // Not transmitted yet.
    target.deliveries[0]();
    target.now += 2000;
    handle(protocol, target, reply(read, DataType::Response, 1));
    assert(target.confirmed() == 0);
    for (int i = 0; i < 100; i++)
    {
        target.now += 10000;
        protocol.protocol_update(&target);
    }
    assert(target.all_publish_data.size() == 1); // No timeout retry or polling.

    NasaProtocol rollover;
    DisplayTarget wrapping;
    wrapping.now = UINT32_MAX - 100;
    rollover.enable_display_light("20.00.01");
    rollover.protocol_update(&wrapping);
    wrapping.now += 150;
    handle(rollover, wrapping, reply(wrapping.last_packet(), DataType::Response, 1));
    assert(wrapping.confirmed() == 1);
}

static void test_nack_and_superseded()
{
    NasaProtocol protocol;
    DisplayTarget target;
    protocol.enable_display_light("20.00.02");
    protocol.protocol_update(&target);
    auto old_read = target.last_packet();
    ProtocolRequest request;
    request.display_light = false;
    protocol.publish_request(&target, "20.00.02", request);
    handle(protocol, target, reply(old_read, DataType::Response, 1));
    assert(target.confirmed() == 0);
    protocol.protocol_update(&target);
    auto command = target.last_packet();
    handle(protocol, target, reply(command, DataType::Nack));
    target.now = 3000;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 2); // Rejected command does not trigger a read/retry.
    protocol.publish_request(&target, "20.00.02", request);
    protocol.protocol_update(&target);
    target.now += 500; // Lost ACK still permits one read of the actual state.
    protocol.protocol_update(&target);
    auto read = target.last_packet();
    handle(protocol, target, reply(read, DataType::Nack));
    target.now += 10000;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 4);
    assert(target.confirmed() == 0);
}

static void test_pending_commands_do_not_block_climate()
{
    NasaProtocol protocol;
    DisplayTarget target;
    target.delay_delivery = true;
    protocol.enable_display_light("20.00.02");
    ProtocolRequest display;
    display.display_light = false;
    protocol.publish_request(&target, "20.00.02", display);
    protocol.protocol_update(&target);
    auto first_command = target.last_packet();
    target.now = 10000;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 1); // No read before actual command delivery.
    auto wrong_ack = reply(first_command, DataType::Ack);
    wrong_ack.sa = Address::parse("20.00.03");
    assert(!protocol.process_display_packet(&target, wrong_ack));

    display.display_light = true;
    protocol.publish_request(&target, "20.00.02", display);
    ProtocolRequest climate;
    climate.target_temp = 24;
    protocol.publish_request(&target, "20.00.02", climate);
    protocol.protocol_update(&target);
    auto independent = target.last_packet();
    assert(independent.messages.size() == 1);
    assert(independent.messages[0].messageNumber == MessageNumber::VAR_in_temp_target_f);
    target.deliveries[0]();
    target.now += 499;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.size() == 2);
    target.now++;
    protocol.protocol_update(&target);
    auto latest = target.last_packet();
    assert(latest.command.dataType == DataType::Request);
    assert(latest.messages.size() == 1 && latest.messages[0].value == 1);
    target.deliveries[1]();
    target.delay_delivery = false;
    target.now += 500;
    protocol.protocol_update(&target);
    auto read = target.last_packet();
    assert(read.command.dataType == DataType::Read);
    handle(protocol, target, reply(first_command, DataType::Ack));
    assert(target.confirmed() == 0);
    handle(protocol, target, reply(read, DataType::Response, 1));
    assert(target.confirmed() == 1);
}

static void test_decoder_and_power_refresh()
{
    DisplayTarget target;
    auto protocol = static_cast<NasaProtocol *>(get_protocol("20.00.02"));
    protocol->enable_display_light("20.00.02");
    protocol->enable_display_light("20.00.03");
    protocol->protocol_update(&target);
    assert(target.all_publish_data.size() == 2);
    auto read = target.last_packet();
    receive(target, reply(read, DataType::Ack));
    assert(target.acks == 0 && target.confirmed() == 0);
    receive(target, reply(read, DataType::Response, 1));
    assert(target.states.back().first == "20.00.03");
    assert(target.confirmed() == 1);
    auto power = Packet::create(Address::get_my_address(), DataType::Notification, MessageNumber::ENUM_in_operation_power, 1);
    power.sa = Address::parse("20.00.03");
    receive(target, power);
    protocol->protocol_update(&target);
    read = target.last_packet();
    assert(read.command.dataType == DataType::Read);
    receive(target, reply(read, DataType::Response, 0xff));
    assert(target.confirmed() == 1 && !target.states.back().second.has_value());
    const auto count = target.all_publish_data.size();
    receive(target, power);
    protocol->protocol_update(&target);
    assert(target.all_publish_data.size() == count); // Identical power broadcasts do not poll.
}

static void test_callback_can_enqueue_next_command()
{
    NasaProtocol protocol;
    DisplayTarget target;
    protocol.enable_display_light("20.00.02");
    ProtocolRequest first;
    first.display_light = false;
    protocol.publish_request(&target, "20.00.02", first);
    target.on_invalid = [&]()
    {
        ProtocolRequest next;
        next.display_light = true;
        next.mode = Mode::Heat;
        protocol.publish_request(&target, "20.00.02", next);
    };
    protocol.protocol_update(&target);
    assert(target.last_packet().messages[0].value == 0);
    target.now = 500;
    protocol.protocol_update(&target);
    auto next = target.packet_at(1);
    assert(next.messages.size() == 1);
    assert(next.messages[0].messageNumber == MessageNumber::ENUM_in_display_light);
    assert(next.messages[0].value == 1);
    auto climate = target.last_packet();
    assert(climate.messages.size() == 2);
    assert(climate.messages[0].value == static_cast<int>(Mode::Heat));
    assert(climate.messages[1].value == 1);
}

static void test_absent_and_climate_only()
{
    NasaProtocol protocol;
    DisplayTarget target;
    protocol.protocol_update(&target);
    assert(target.all_publish_data.empty());
    ProtocolRequest mode, power;
    mode.mode = Mode::Heat;
    protocol.publish_request(&target, "20.00.01", mode);
    power.power = false;
    protocol.publish_request(&target, "20.00.01", power);
    protocol.protocol_update(&target);
    auto command = target.last_packet();
    assert(command.messages.size() == 2);
    assert(command.messages[0].messageNumber == MessageNumber::ENUM_in_operation_mode);
    assert(command.messages[1].messageNumber == MessageNumber::ENUM_in_operation_power);
    assert(command.messages[1].value == 0);
    assert(target.states.empty());
}

int main()
{
    test_encoding();
    test_reads();
    test_commands_and_merging();
    test_timeouts_and_delivery();
    test_nack_and_superseded();
    test_pending_commands_do_not_block_climate();
    test_callback_can_enqueue_next_command();
    test_absent_and_climate_only();
    test_decoder_and_power_refresh();
    cout << "NASA display tests passed" << endl;
}
