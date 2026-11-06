#include "external_room_temperature.h"
#include "esphome/core/log.h"
#include <cstring>
#include <algorithm>

namespace esphome
{
namespace samsung_ac
{
    static const char *const EXTERNAL_TEMP_TAG = "samsung_ac.external_temp";
    static constexpr uint32_t QUEUE_TIMEOUT = 5000;

    ExternalRoomTemperature::ExternalRoomTemperature(MessageTarget *target, std::string address, std::string source)
        : target_(target), address_(std::move(address)), source_(std::move(source)) {}

    const char *ExternalRoomTemperature::phase_name(ExternalTemperaturePhase phase)
    {
        switch (phase)
        {
            case ExternalTemperaturePhase::Disabled: return "disabled";
            case ExternalTemperaturePhase::AwaitSample: return "awaiting_new_sample";
            case ExternalTemperaturePhase::Ready: return "ready";
            case ExternalTemperaturePhase::Queued: return "queued";
            case ExternalTemperaturePhase::AwaitEcho: return "awaiting_echo";
            case ExternalTemperaturePhase::AwaitReport: return "awaiting_report";
            case ExternalTemperaturePhase::Following: return "reported_match";
            case ExternalTemperaturePhase::Fault: return "fault";
        }
        return "unknown";
    }

    bool ExternalRoomTemperature::valid_address(const std::string &address, uint8_t klass)
    {
        if (address.size() != 8 || address[2] != '.' || address[5] != '.')
            return false;
        for (unsigned i : {0, 1, 3, 4, 6, 7})
        {
            const char c = address[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                return false;
        }
        auto parsed = Address::parse(address);
        return static_cast<uint8_t>(parsed.klass) == klass && parsed.channel != 0xFF && parsed.address != 0xFF;
    }

    void ExternalRoomTemperature::begin()
    {
        if (begun_)
            return;
        begun_ = true;
        const auto &o = options;
        config_valid_ = valid_address(address_, 0x20) && valid_address(source_, 0x50) &&
            o.update_interval >= 30000 && o.update_interval <= 300000 &&
            o.max_age >= o.update_interval && o.max_age <= 600000 &&
            o.observe_for >= 30000 && o.observe_for <= 300000 &&
            o.response_timeout >= 1000 && o.response_timeout <= 10000 &&
            o.verification_timeout >= 45000 && o.verification_timeout <= 300000 &&
            std::isfinite(o.minimum) && std::isfinite(o.maximum) && o.minimum >= -20 && o.maximum <= 60 &&
            o.minimum < o.maximum && std::isfinite(o.max_step) && o.max_step >= 0.1f && o.max_step <= 20 &&
            std::isfinite(o.tolerance) && o.tolerance >= 0.1f && o.tolerance <= 2;
        if (!config_valid_)
        {
            fault("invalid_configuration");
            return;
        }
        address_ = Address::parse(address_).to_string();
        source_ = Address::parse(source_).to_string();
        ESP_LOGI(EXTERNAL_TEMP_TAG, "device=%s event=start enabled=0 source=%s fallback=unverified supervised_only=1",
                 address_.c_str(), source_.c_str());
        notify();
    }

    bool ExternalRoomTemperature::valid_temperature(float value) const
    {
        if (!std::isfinite(value) || value < options.minimum || value > options.maximum)
            return false;
        const long encoded = std::lround(value * 10.0f);
        // 0xFFFF is ambiguous with an unsupported Variable response; never send that sentinel.
        return encoded != -1 && encoded / 10.0f >= options.minimum && encoded / 10.0f <= options.maximum;
    }

    bool ExternalRoomTemperature::fresh() const
    {
        return have_sample_ && sample_valid_ && target_->get_miliseconds() - state_.received_at <= options.max_age;
    }

    void ExternalRoomTemperature::notify()
    {
        state_.fresh = fresh();
        if (on_change)
            on_change(state_);
    }

    void ExternalRoomTemperature::transition(ExternalTemperaturePhase phase, const char *reason, bool important)
    {
        if (state_.phase == phase && std::strcmp(state_.reason, reason) == 0)
            return;
        state_.phase = phase;
        state_.reason = reason;
        if (important)
        {
            ESP_LOGI(EXTERNAL_TEMP_TAG, "device=%s ms=%lu state=%s reason=%s enabled=%d fallback=unverified tx=%lu failures=%lu",
                     address_.c_str(), static_cast<unsigned long>(target_->get_miliseconds()), phase_name(phase), reason,
                     state_.enabled, static_cast<unsigned long>(state_.transmissions), static_cast<unsigned long>(state_.failures));
        }
        else if (options.debug)
        {
            ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s ms=%lu state=%s reason=%s retry=0 control_loop=unverified",
                     address_.c_str(), static_cast<unsigned long>(target_->get_miliseconds()), phase_name(phase), reason);
        }
        notify();
    }

    void ExternalRoomTemperature::fault(const char *reason)
    {
        if (state_.phase == ExternalTemperaturePhase::Fault && std::strcmp(state_.reason, reason) == 0)
            return;
        ++generation_;
        state_.enabled = false;
        state_.reported_match = false;
        ++state_.failures;
        ESP_LOGW(EXTERNAL_TEMP_TAG, "device=%s event=fault reason=%s last_sent=%.1f fallback=unverified latched_possible=1 manual_rearm=required",
                 address_.c_str(), reason, state_.transmitted);
        transition(ExternalTemperaturePhase::Fault, reason, true);
    }

    void ExternalRoomTemperature::receive_sample(float value)
    {
        begin();
        const bool changed = !have_sample_ || !std::isfinite(value) || value != state_.input;
        have_sample_ = true;
        state_.received_at = target_->get_miliseconds();
        ++sample_revision_;
        state_.input = value;
        sample_valid_ = config_valid_ && valid_temperature(value);
        const bool implausible = sample_valid_ && std::isfinite(state_.last_accepted) &&
            std::fabs(value - state_.last_accepted) > options.max_step;
        if (implausible)
            sample_valid_ = false;
        if (sample_valid_)
            state_.last_accepted = value;
        if (options.debug && changed)
        {
            ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s ms=%lu event=input value=%.2f valid=%d age_ms=0 last_accepted=%.2f reason=%s",
                     address_.c_str(), static_cast<unsigned long>(state_.received_at), value, sample_valid_, state_.last_accepted,
                     sample_valid_ ? "measurement_received" : (implausible ? "implausible_step" : "invalid_measurement"));
        }
        if (!sample_valid_ && state_.enabled)
            fault(implausible ? "implausible_step" : "invalid_measurement");
        else
            notify();
    }

    const char *ExternalRoomTemperature::unsafe_reason() const
    {
        if (!config_valid_) return "invalid_configuration";
        if (conflict_) return "controller_conflict";
        if (!target_->is_bus_ready()) return "bus_not_ready";
        if (!observing_ || target_->get_miliseconds() - observe_started_ < options.observe_for) return "observing";
        if (!fresh()) return have_sample_ && sample_valid_ ? "sensor_stale" : "missing_valid_sensor";
        if (!state_.power.has_value() || state_.mode == Mode::Unknown) return "operating_state_unknown";
        if (!state_.power.value()) return "unit_off";
        if (state_.mode != Mode::Cool && state_.mode != Mode::Heat) return "unsupported_mode";
        if (target_->get_miliseconds() - mode_at_ > options.max_age ||
            target_->get_miliseconds() - power_at_ > options.max_age) return "operating_state_stale";
        return nullptr;
    }

    bool ExternalRoomTemperature::enable(bool value)
    {
        begin();
        if (!value)
        {
            ++generation_;
            state_.enabled = false;
            state_.reported_match = false;
            transition(ExternalTemperaturePhase::Disabled, "manual_disable_fallback_unverified", true);
            return false;
        }
        if (state_.enabled)
            return true;
        if (const char *reason = unsafe_reason())
        {
            if (conflict_ || !config_valid_)
                fault(reason);
            else
                transition(ExternalTemperaturePhase::Disabled, reason, true);
            return false;
        }
        ++generation_;
        arm_revision_ = sample_revision_;
        echo_seen_ = false;
        state_.reported_match = false;
        state_.enabled = true;
        // A startup or reconnect snapshot alone cannot trigger a write.
        transition(ExternalTemperaturePhase::AwaitSample, "manual_enable_requires_new_sample", true);
        return state_.enabled;
    }

    void ExternalRoomTemperature::update()
    {
        begin();
        const uint32_t now = target_->get_miliseconds();
        if (!target_->is_bus_ready())
            observing_ = false;
        else if (!observing_)
        {
            observing_ = true;
            observe_started_ = now;
        }
        if (state_.fresh != fresh())
        {
            if (options.debug)
            {
                ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s event=freshness fresh=%d age_ms=%lu",
                         address_.c_str(), fresh(), static_cast<unsigned long>(now - state_.received_at));
            }
            notify();
        }
        if (!state_.enabled)
            return;
        if (const char *reason = unsafe_reason())
        {
            fault(reason);
            return;
        }
        if (state_.phase == ExternalTemperaturePhase::Queued && now - queued_at_ >= QUEUE_TIMEOUT)
            fault("queue_timeout");
        else if (state_.phase == ExternalTemperaturePhase::AwaitEcho && now - state_.sent_at >= options.response_timeout)
            fault("response_timeout");
        else if (state_.phase == ExternalTemperaturePhase::AwaitReport && now - state_.sent_at >= options.verification_timeout)
            fault("reported_temperature_not_following");
        else if (state_.phase == ExternalTemperaturePhase::AwaitSample && sample_revision_ != arm_revision_)
            transition(ExternalTemperaturePhase::Ready, "new_measurement");

        if (state_.enabled && (state_.phase == ExternalTemperaturePhase::Ready || state_.phase == ExternalTemperaturePhase::Following) &&
            (!state_.transmissions || now - state_.sent_at >= options.update_interval))
            queue_write();
    }

    void ExternalRoomTemperature::queue_write()
    {
        auto packet = Packet::create(Address::parse(address_), DataType::Write, MessageNumber::VAR_in_temp_external_f,
                                     static_cast<int16_t>(std::lround(state_.last_accepted * 10.0f)));
        packet.sa = Address::parse(source_); // Per packet: never change the normal JIGTester source.
        encoded_ = static_cast<int16_t>(packet.messages[0].value);
        packet_number_ = packet.command.packetNumber;
        frame_ = packet.encode();
        queued_at_ = target_->get_miliseconds();
        const uint32_t generation = ++generation_;
        transition(ExternalTemperaturePhase::Queued, "write_scheduled");
        if (generation != generation_ || !state_.enabled)
            return;
        const bool accepted = target_->publish_data_once_guarded(packet_number_, std::vector<uint8_t>(frame_),
            [this, generation]() { return can_send(generation); }, [this, generation]() { sent(generation); });
        if (!accepted && generation == generation_ && state_.enabled)
            fault("queue_full_or_transport_unsupported");
    }

    bool ExternalRoomTemperature::can_send(uint32_t generation)
    {
        if (generation != generation_ || !state_.enabled || state_.phase != ExternalTemperaturePhase::Queued)
            return false;
        if (const char *reason = unsafe_reason())
        {
            fault(reason);
            return false;
        }
        if (target_->get_miliseconds() - queued_at_ >= QUEUE_TIMEOUT)
        {
            fault("queue_timeout");
            return false;
        }
        if (std::lround(state_.last_accepted * 10.0f) != encoded_)
        {
            transition(ExternalTemperaturePhase::Ready, "queued_sample_superseded");
            return false;
        }
        return true;
    }

    void ExternalRoomTemperature::sent(uint32_t generation)
    {
        if (generation != generation_ || !state_.enabled || state_.phase != ExternalTemperaturePhase::Queued)
            return;
        state_.sent_at = target_->get_miliseconds();
        last_sent_packet_number_ = packet_number_;
        last_sent_encoded_ = encoded_;
        state_.transmitted = encoded_ / 10.0f;
        ++state_.transmissions;
        echo_seen_ = false;
        post_tx_report_ = false;
        state_.reported_match = false;
        if (options.debug)
        {
            ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s ms=%lu event=sent source=%s class=50 type=Write msg=4203 encoded=%d value=%.1f pn=%u age_ms=%lu retry=0",
                     address_.c_str(), static_cast<unsigned long>(state_.sent_at), source_.c_str(), encoded_,
                     state_.transmitted, packet_number_, static_cast<unsigned long>(state_.sent_at - state_.received_at));
        }
        if (options.debug_raw)
        {
            ESP_LOGVV(EXTERNAL_TEMP_TAG, "device=%s TX=%s", address_.c_str(), bytes_to_hex(frame_).c_str());
        }
        transition(ExternalTemperaturePhase::AwaitEcho, "write_sent_no_acceptance_yet");
    }

    bool ExternalRoomTemperature::owns_loopback(Packet &packet) const
    {
        return state_.transmissions && target_->get_miliseconds() - state_.sent_at <= QUEUE_TIMEOUT &&
            packet.sa.to_string() == source_ && packet.da.to_string() == address_ &&
            packet.command.dataType == DataType::Write && packet.command.packetNumber == last_sent_packet_number_ &&
            packet.command.packetType == PacketType::Normal && packet.command.packetInformation &&
            packet.command.protocolVersion == 2 && packet.command.retryCount == 0 &&
            packet.messages.size() == 1 && packet.messages[0].messageNumber == MessageNumber::VAR_in_temp_external_f &&
            static_cast<int16_t>(packet.messages[0].value) == last_sent_encoded_;
    }

    void ExternalRoomTemperature::compare_report()
    {
        if (!state_.enabled || !post_tx_report_)
            return;
        if (const char *reason = unsafe_reason())
        {
            fault(reason);
            return;
        }
        const uint32_t elapsed = target_->get_miliseconds() - state_.sent_at;
        if ((state_.phase == ExternalTemperaturePhase::AwaitEcho && elapsed >= options.response_timeout) ||
            (state_.phase == ExternalTemperaturePhase::AwaitReport && elapsed >= options.verification_timeout))
        {
            fault(state_.phase == ExternalTemperaturePhase::AwaitEcho ? "response_timeout" : "reported_temperature_not_following");
            return;
        }
        const float difference = state_.reported - state_.transmitted;
        state_.reported_match = std::fabs(difference) <= options.tolerance;
        if (options.debug)
        {
            ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s requested=%.1f reported=%.1f difference=%.1f echo_correlated=%d mode=%d power=%d control_loop=unverified",
                     address_.c_str(), state_.transmitted, state_.reported, difference, echo_seen_,
                     static_cast<int>(state_.mode), state_.power.value_or(false));
        }
        if (state_.phase == ExternalTemperaturePhase::Following && !state_.reported_match)
            fault("reported_temperature_diverged");
        else if (echo_seen_ && state_.reported_match &&
                 (state_.phase == ExternalTemperaturePhase::AwaitEcho || state_.phase == ExternalTemperaturePhase::AwaitReport))
        {
            state_.verified_at = target_->get_miliseconds();
            transition(ExternalTemperaturePhase::Following, "echo_and_report_match_not_control_proof");
        }
    }

    void ExternalRoomTemperature::observe(Packet &packet, bool own_echo)
    {
        begin();
        const auto source = packet.sa.to_string();
        const auto dest = packet.da.to_string();
        if (!own_echo && (packet.sa.klass == AddressClass::WiredRemote || packet.sa.klass == AddressClass::WiFiKit))
        {
            if (!conflict_)
            {
                conflict_ = true;
                ESP_LOGW(EXTERNAL_TEMP_TAG, "device=%s event=controller_observed source=%s reason=controller_conflict restart_after_manual_review=required",
                         address_.c_str(), source.c_str());
                fault("controller_conflict");
            }
            return;
        }
        if (source != address_)
            return;
        if (packet.command.packetType != PacketType::Normal || !packet.command.packetInformation ||
            packet.command.protocolVersion != 2)
            return;
        const uint32_t now = target_->get_miliseconds();
        const auto type = packet.command.dataType;
        if (options.debug_raw)
        {
            auto copy = packet;
            ESP_LOGVV(EXTERNAL_TEMP_TAG, "device=%s RX_validated=%s", address_.c_str(), bytes_to_hex(copy.encode()).c_str());
        }
        const bool correlated = state_.enabled && dest == source_ && packet.command.packetNumber == packet_number_ &&
            (state_.phase == ExternalTemperaturePhase::AwaitEcho || state_.phase == ExternalTemperaturePhase::AwaitReport);
        const bool relevant = std::any_of(packet.messages.begin(), packet.messages.end(), [](const MessageSet &m) {
            return m.messageNumber == MessageNumber::VAR_in_temp_external_f || m.messageNumber == MessageNumber::VAR_in_temp_room_f;
        });
        if (options.debug && (correlated || relevant))
        {
            ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s ms=%lu event=rx source=%s dest=%s type=%u pn=%u correlated=%d",
                     address_.c_str(), static_cast<unsigned long>(now), source.c_str(), dest.c_str(),
                     static_cast<unsigned>(type), packet.command.packetNumber, correlated);
        }
        if (correlated && type == DataType::Nack)
        {
            fault("nack");
            return;
        }
        // ACK cannot satisfy the response deadline or establish sensor selection.
        if (type != DataType::Notification && type != DataType::Response)
            return;
        // Apply all operating fields before temperature evidence, regardless of their wire order.
        for (unsigned stage = 0; stage < 2; ++stage)
        for (const auto &message : packet.messages)
        {
            const bool operating = message.messageNumber == MessageNumber::ENUM_in_operation_power ||
                                   message.messageNumber == MessageNumber::ENUM_in_operation_mode;
            if (operating != (stage == 0))
                continue;
            if (options.debug && (message.messageNumber == MessageNumber::VAR_in_temp_external_f ||
                                 message.messageNumber == MessageNumber::VAR_in_temp_room_f))
            {
                ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s event=rx_value msg=%04x encoded=%ld correlated=%d",
                         address_.c_str(), static_cast<unsigned>(message.messageNumber), message.value, correlated);
            }
            if (type == DataType::Notification)
            {
                if (message.messageNumber == MessageNumber::ENUM_in_operation_power)
                {
                    if (message.value != 0 && message.value != 1)
                    {
                        state_.power.reset();
                        if (state_.enabled) fault("invalid_power_report");
                        continue;
                    }
                    if (options.debug && (!state_.power.has_value() || state_.power.value() != (message.value == 1)))
                    {
                        ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s event=operating_power value=%ld", address_.c_str(), message.value);
                    }
                    state_.power = message.value == 1;
                    power_at_ = now;
                    if (state_.enabled && !state_.power.value()) fault("unit_off");
                }
                else if (message.messageNumber == MessageNumber::ENUM_in_operation_mode)
                {
                    if (message.value < 0 || message.value > 4)
                    {
                        state_.mode = Mode::Unknown;
                        if (state_.enabled) fault("invalid_mode_report");
                        continue;
                    }
                    const Mode next = static_cast<Mode>(message.value);
                    if (options.debug && state_.mode != next)
                    {
                        ESP_LOGD(EXTERNAL_TEMP_TAG, "device=%s event=operating_mode value=%ld", address_.c_str(), message.value);
                    }
                    if (state_.enabled && state_.mode != next) fault("mode_changed");
                    state_.mode = next;
                    mode_at_ = now;
                }
                else if (message.messageNumber == MessageNumber::VAR_in_temp_room_f)
                {
                    const float value = static_cast<int16_t>(message.value) / 10.0f;
                    if (static_cast<uint16_t>(message.value) == 0xFFFF || !valid_temperature(value))
                    {
                        if (state_.enabled) fault("invalid_reported_temperature");
                        continue;
                    }
                    state_.reported = value;
                    if (state_.phase == ExternalTemperaturePhase::AwaitEcho || state_.phase == ExternalTemperaturePhase::AwaitReport ||
                        state_.phase == ExternalTemperaturePhase::Following)
                        post_tx_report_ = true;
                    compare_report();
                }
            }
            else if (correlated && state_.phase == ExternalTemperaturePhase::AwaitEcho &&
                     message.messageNumber == MessageNumber::VAR_in_temp_external_f)
            {
                if (now - state_.sent_at >= options.response_timeout)
                {
                    fault("response_timeout");
                    return;
                }
                if (static_cast<int16_t>(message.value) != encoded_)
                {
                    fault("echo_mismatch_or_unsupported");
                    return;
                }
                echo_seen_ = true;
                transition(ExternalTemperaturePhase::AwaitReport, "echo_received_control_unverified");
                compare_report();
            }
        }
        notify();
    }
}
}
