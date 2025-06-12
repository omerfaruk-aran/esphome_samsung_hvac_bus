#pragma once
// Fake Hal for Local Testing

namespace esphome
{
    uint32_t millis();
    uint32_t micros();
    void delay(uint32_t ms);
} // namespace esphome
