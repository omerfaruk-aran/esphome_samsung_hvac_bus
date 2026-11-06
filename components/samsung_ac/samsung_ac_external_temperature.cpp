#include "samsung_ac_external_temperature.h"
#include "esphome/core/hal.h"

namespace esphome
{
namespace samsung_ac
{
    void Samsung_AC_External_Temperature::configure(Samsung_AC *bus, const std::string &address,
                                                   const std::string &source, sensor::Sensor *input)
    {
        source_sensor_ = input;
        model_ = std::make_unique<ExternalRoomTemperature>(bus, address, source);
        model_->on_change = [this](const ExternalTemperatureState &state) { publish(state); };
        static_cast<NasaProtocol *>(get_protocol(address))->register_external_temperature(model_.get());
    }

    void Samsung_AC_External_Temperature::set_timing(uint32_t interval, uint32_t max_age, uint32_t observation,
                                                    uint32_t response, uint32_t verification)
    {
        auto &o = model_->options;
        o.update_interval = interval;
        o.max_age = max_age;
        o.observe_for = observation;
        o.response_timeout = response;
        o.verification_timeout = verification;
    }

    void Samsung_AC_External_Temperature::set_limits(float minimum, float maximum, float step, float tolerance)
    {
        auto &o = model_->options;
        o.minimum = minimum;
        o.maximum = maximum;
        o.max_step = step;
        o.tolerance = tolerance;
    }

    void Samsung_AC_External_Temperature::set_logging(bool debug, bool raw)
    {
        model_->options.debug = debug;
        model_->options.debug_raw = raw;
    }

    void Samsung_AC_External_Temperature::setup()
    {
        model_->begin();
        // Do not seed from cached state, and do not let heartbeat/filter publications refresh age.
        // The raw source must already be Celsius. A new raw event after manual enable is required.
        source_sensor_->add_on_raw_state_callback([this](float value) { model_->receive_sample(value); });
    }

    void Samsung_AC_External_Temperature::loop()
    {
        const uint32_t now = millis();
        if (now - last_loop_ >= 20)
        {
            last_loop_ = now;
            model_->update();
        }
    }

    void Samsung_AC_External_Temperature::write_state(bool value)
    {
        model_->enable(value);
        // A rejected ON command must still correct the switch state if the reason is unchanged.
        this->publish_state(model_->state().enabled);
    }

    static void publish_changed(sensor::Sensor *entity, float value)
    {
        if (entity != nullptr && (!entity->has_state() ||
            (std::isnan(entity->state) != std::isnan(value)) ||
            (!std::isnan(value) && entity->state != value)))
            entity->publish_state(value);
    }

    void Samsung_AC_External_Temperature::publish(const ExternalTemperatureState &state)
    {
        if (!this->has_state() || this->state != state.enabled)
            this->publish_state(state.enabled);
        publish_changed(input_diagnostic_, state.input);
        publish_changed(sent_diagnostic_, state.transmitted);
        publish_changed(reported_diagnostic_, state.reported);
        publish_changed(failures_, state.failures);
        if (fresh_ != nullptr && (!fresh_->has_state() || fresh_->state != state.fresh))
            fresh_->publish_state(state.fresh);
        if (match_ != nullptr && (!match_->has_state() || match_->state != state.reported_match))
            match_->publish_state(state.reported_match);
        if (status_ != nullptr)
        {
            const std::string value = std::string(ExternalRoomTemperature::phase_name(state.phase)) + ":" + state.reason;
            if (!status_->has_state() || status_->state != value)
                status_->publish_state(value);
        }
    }
}
}
