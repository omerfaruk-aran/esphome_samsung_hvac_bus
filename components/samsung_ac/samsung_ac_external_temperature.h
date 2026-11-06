#pragma once

#include <memory>
#include "external_room_temperature.h"
#include "samsung_ac.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome
{
namespace samsung_ac
{
    // The switch grants permission to send; it never claims the unit selected this sensor.
    class Samsung_AC_External_Temperature : public Component, public switch_::Switch
    {
    public:
        void configure(Samsung_AC *bus, const std::string &address, const std::string &source,
                       sensor::Sensor *input);
        void set_timing(uint32_t interval, uint32_t max_age, uint32_t observation, uint32_t response, uint32_t verification);
        void set_limits(float minimum, float maximum, float step, float tolerance);
        void set_logging(bool debug, bool raw);
        void set_input_sensor(sensor::Sensor *value) { input_diagnostic_ = value; }
        void set_sent_sensor(sensor::Sensor *value) { sent_diagnostic_ = value; }
        void set_reported_sensor(sensor::Sensor *value) { reported_diagnostic_ = value; }
        void set_failures_sensor(sensor::Sensor *value) { failures_ = value; }
        void set_fresh_sensor(binary_sensor::BinarySensor *value) { fresh_ = value; }
        void set_match_sensor(binary_sensor::BinarySensor *value) { match_ = value; }
        void set_status_sensor(text_sensor::TextSensor *value) { status_ = value; }
        void setup() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::DATA; }

    protected:
        std::unique_ptr<ExternalRoomTemperature> model_;
        sensor::Sensor *source_sensor_{nullptr};
        sensor::Sensor *input_diagnostic_{nullptr};
        sensor::Sensor *sent_diagnostic_{nullptr};
        sensor::Sensor *reported_diagnostic_{nullptr};
        sensor::Sensor *failures_{nullptr};
        binary_sensor::BinarySensor *fresh_{nullptr};
        binary_sensor::BinarySensor *match_{nullptr};
        text_sensor::TextSensor *status_{nullptr};
        uint32_t last_loop_{0};
        void write_state(bool value) override;
        void publish(const ExternalTemperatureState &state);
    };
}
}
