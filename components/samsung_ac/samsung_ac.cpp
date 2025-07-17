#include "samsung_ac.h"
#include "debug_mqtt.h"
#include "util.h"
#include "log.h"
#include <vector>

namespace esphome
{
  namespace samsung_ac
  {
    void Samsung_AC::setup()
    {
      if (debug_log_messages)
      {
        LOGW("setup");
      }
      if (this->flow_control_pin_ != nullptr)
      {
        this->flow_control_pin_->setup();
      }
    }

    void Samsung_AC::update()
    {
      if (debug_log_messages)
      {
        LOGW("update");
      }

      for (const auto &pair : devices_)
      {
        optional<Mode> current_value = pair.second->_cur_mode;
        std::string address = pair.second->address;

        if (current_value.has_value())
        {
          state_tracker_.update(address, current_value.value());
        }
      }

      debug_mqtt_connect(debug_mqtt_host, debug_mqtt_port, debug_mqtt_username, debug_mqtt_password);

      // Waiting for first update before beginning processing data
      if (data_processing_init)
      {
        LOGC("Data Processing starting");
        data_processing_init = false;
      }

      std::string devices;
      for (const auto &pair : devices_)
      {
        if (!devices.empty())
          devices += ", ";
        devices += pair.second->address;
      }
      LOGC("Configured devices: %s", devices.c_str());

      std::string knownIndoor, knownOutdoor, knownOther;
      for (const auto &address : addresses_)
      {
        auto &target = (get_address_type(address) == AddressType::Outdoor) ? knownOutdoor : (get_address_type(address) == AddressType::Indoor) ? knownIndoor
                                                                                                                                               : knownOther;
        if (!target.empty())
          target += ", ";
        target += address;
      }

      LOGC("Discovered devices:");
      LOGC("  Outdoor: %s", (knownOutdoor.length() == 0 ? "-" : knownOutdoor.c_str()));
      LOGC("  Indoor:  %s", (knownIndoor.length() == 0 ? "-" : knownIndoor.c_str()));
      if (knownOther.length() > 0)
      {
        LOGC("  Other:   %s", knownOther.c_str());
      }
    }

    void Samsung_AC::register_device(Samsung_AC_Device *device)
    {
      if (find_device(device->address) != nullptr)
      {
        LOGW("There is already and device for address %s registered.", device->address.c_str());
        return;
      }

      devices_.insert({device->address, device});
    }

    void Samsung_AC::dump_config()
    {
      LOGC("Samsung_AC:");
      LOG_PIN("  Flow Control Pin: ", this->flow_control_pin_);
    }

    void Samsung_AC::publish_data(uint8_t id, std::vector<uint8_t> &&data) 
    {
      const uint32_t now = millis();

      if (id == 0)
      {
        LOG_RAW_SEND(now-last_transmission_, data);
        last_transmission_ = now;
        this->before_write();
        this->write_array(data);
        this->flush();
        this->after_write();
        return;
      }

      OutgoingData outData;
      outData.id = id;
      outData.data = std::move(data);
      outData.nextRetry = 0;
      outData.retries = 0;
      outData.timeout = now + sendTimeout;
      send_queue_.push_back(std::move(outData));
    }

    void Samsung_AC::ack_data(uint8_t id)
    {
        if (!send_queue_.empty())
        {
          auto senddata = &send_queue_.front();
          if (senddata->id == id) {
            send_queue_.pop_front();
          }
        }
    }

    void Samsung_AC::loop()
    {
      if (data_processing_init)
        return;

      // if more data is expected, do not allow anything to be written
      if (!read_data())
        return;

      // If there is no data we use the time to send
      // And if written, break the loop
      if (write_data())
        return;

      // Allow device protocols to perform recurring tasks when idle (at most every 200ms)
      const uint32_t now = millis();
      if (now - last_protocol_update_ >= 200)
      {
        last_protocol_update_ = now;
        for (const auto &pair : devices_)
        {
          Samsung_AC_Device *device = pair.second;
          device->protocol_update(this);
        }
      }
    }

    bool Samsung_AC::read_data()
    {
      // read as long as there is anything to read
      while (available())
      {
        uint8_t c;
        if (read_byte(&c))
        {
          data_.push_back(c);
        }
      }

      if (data_.empty())
        return true;

      const uint32_t now = millis();

      auto result = process_data(data_, this);
      if (result.type == DecodeResultType::Fill)
        return false;

      if (result.type == DecodeResultType::Discard)
      {
        // collect more so that we can log all discarded bytes at once, but don't wait for too long
        if (result.bytes == data_.size() && now-last_transmission_ < 1000)
          return false;
        LOG_RAW_DISCARDED(now-last_transmission_, data_, 0, result.bytes);
      }
      else
      {
        LOG_RAW(now-last_transmission_, data_, 0, result.bytes);
      }

      if (result.bytes == data_.size())
        data_.clear();
      else
      {
        std::move(data_.begin() + result.bytes, data_.end(), data_.begin());
        data_.resize(data_.size() - result.bytes);
      }
      last_transmission_ = now;
      return false;
    }

    bool Samsung_AC::write_data()
    {
      if (send_queue_.empty())
        return false;

      auto senddata = &send_queue_.front();

      const uint32_t now = millis();
      if (senddata->timeout <= now && senddata->retries >= minRetries) {
        LOGE("Packet sending timeout %d after %d retries", senddata->id, senddata->retries);
        send_queue_.pop_front();
        return true;
      }

      if (now-last_transmission_ > silenceInterval && senddata->nextRetry < now)
      {
        if (senddata->nextRetry > 0){
          LOGW("Retry sending packet %d", senddata->id);
          senddata->retries++;
        }

        LOG_RAW_SEND(now-last_transmission_,  senddata->data);

        last_transmission_ = now;
        senddata->nextRetry = now + retryInterval;
        this->before_write();
        this->write_array(senddata->data);
        this->flush();
        this->after_write();
      }
      return true;
    }

    void Samsung_AC::before_write()
    {
      if (this->flow_control_pin_ != nullptr) {
        LOGD("switching flow_control_pin to write");
        this->flow_control_pin_->digital_write(true);
      }
    }

    void Samsung_AC::after_write()
    {
      if (this->flow_control_pin_ != nullptr) {
        LOGD("switching flow_control_pin to read");
        this->flow_control_pin_->digital_write(false);
      }
    }

    float Samsung_AC::get_setup_priority() const { return setup_priority::DATA; }
  } // namespace samsung_ac
} // namespace esphome
