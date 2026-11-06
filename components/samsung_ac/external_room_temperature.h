#pragma once

#include <cmath>
#include <functional>
#include "protocol_nasa.h"

namespace esphome
{
namespace samsung_ac
{
    enum class ExternalTemperaturePhase { Disabled, AwaitSample, Ready, Queued, AwaitEcho, AwaitReport, Following, Fault };

    struct ExternalTemperatureOptions
    {
        uint32_t update_interval = 45000;
        uint32_t max_age = 120000;
        uint32_t observe_for = 60000;
        uint32_t response_timeout = 5000;
        uint32_t verification_timeout = 90000;
        float minimum = 5;
        float maximum = 40;
        float max_step = 5;
        float tolerance = 0.5f;
        bool debug = false;
        bool debug_raw = false;
    };

    struct ExternalTemperatureState
    {
        ExternalTemperaturePhase phase = ExternalTemperaturePhase::Disabled;
        const char *reason = "startup_fallback_unverified";
        bool enabled = false;
        bool fresh = false;
        bool reported_match = false;
        float input = NAN;
        float last_accepted = NAN;
        float transmitted = NAN;
        float reported = NAN;
        uint32_t received_at = 0;
        uint32_t sent_at = 0;
        uint32_t verified_at = 0;
        uint32_t transmissions = 0;
        uint32_t failures = 0;
        Mode mode = Mode::Unknown;
        optional<bool> power;
    };

    // Independent of ESPHome entity publication, and exercised by native protocol tests.
    // No state in this class proves that the physical control loop uses the supplied value.
    class ExternalRoomTemperature
    {
    public:
        ExternalRoomTemperature(MessageTarget *target, std::string address, std::string source);
        ExternalTemperatureOptions options;
        std::function<void(const ExternalTemperatureState &)> on_change;

        void begin();
        void receive_sample(float value);
        bool enable(bool value);
        void update();
        void observe(Packet &packet, bool own_echo = false);
        bool owns_loopback(Packet &packet) const;
        const ExternalTemperatureState &state() const { return state_; }
        const std::string &address() const { return address_; }
        static const char *phase_name(ExternalTemperaturePhase phase);

    protected:
        MessageTarget *target_;
        std::string address_;
        std::string source_;
        ExternalTemperatureState state_;
        bool begun_ = false;
        bool config_valid_ = false;
        bool observing_ = false;
        bool conflict_ = false;
        bool sample_valid_ = false;
        bool have_sample_ = false;
        bool echo_seen_ = false;
        bool post_tx_report_ = false;
        uint32_t observe_started_ = 0;
        uint32_t sample_revision_ = 0;
        uint32_t arm_revision_ = 0;
        uint32_t generation_ = 0;
        uint32_t queued_at_ = 0;
        uint32_t mode_at_ = 0;
        uint32_t power_at_ = 0;
        uint8_t packet_number_ = 0;
        int16_t encoded_ = 0;
        uint8_t last_sent_packet_number_ = 0;
        int16_t last_sent_encoded_ = 0;
        std::vector<uint8_t> frame_;

        static bool valid_address(const std::string &address, uint8_t klass);
        bool valid_temperature(float value) const;
        bool fresh() const;
        const char *unsafe_reason() const;
        void transition(ExternalTemperaturePhase phase, const char *reason, bool important = false);
        void fault(const char *reason);
        void notify();
        void queue_write();
        bool can_send(uint32_t generation);
        void sent(uint32_t generation);
        void compare_report();
    };
}
}
