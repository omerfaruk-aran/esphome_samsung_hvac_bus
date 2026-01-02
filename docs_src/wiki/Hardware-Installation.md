To use the ESPHome Samsung HVAC Bus component, you will need specific hardware to connect your ESP device to the communication bus of your Samsung AC unit. This guide will walk you through the recommended hardware and connection setup.

## Required Hardware

### ESP Device

It is recommended to use an **ESP32** for better performance, as it offers more CPU power and RAM compared to an ESP8266. The ESP device will act as the central controller for communication between your Samsung HVAC units and Home Assistant.

### RS-485 to TTL Adapter

You will need an RS-485 to TTL serial adapter to interface between the Samsung AC’s communication bus (typically labeled F1/F2) and your ESP device.

### Recommended Setup: M5STACK ATOM Lite with RS-485 Base

For the simplest setup, we recommend using the **M5STACK ATOM Lite** combined with the **M5STACK RS-485 Base**. This combination is affordable, comes with a compact case that fits inside most indoor units, and allows direct use of the 12V provided by the V1/V2 lines on some AC units.

- **M5STACK ATOM Lite** - [Aliexpress](https://a.aliexpress.com/_mO88aeK), [M5STACK Store](https://shop.m5stack.com/products/atom-lite-esp32-development-kit), [Documentation](https://docs.m5stack.com/en/core/ATOM%20Lite)
- **M5STACK ATOM RS-485 Base** - [Aliexpress](https://a.aliexpress.com/_mLhOZQA), [M5STACK Store](https://shop.m5stack.com/products/atomic-rs485-base), [Documentation](https://docs.m5stack.com/en/atom/atomic485)

## Connection Instructions

1. **Connect the RS-485 Wires:**
   - **F1 (AC Unit)** → **A (M5STACK RS-485 Base)**
   - **F2 (AC Unit)** → **B (M5STACK RS-485 Base)**

2. **Connect Power Lines (if available):**
   - **V1 (AC Unit)** → **DC (M5STACK RS-485 Base)**
   - **V2 (AC Unit)** → **GND (M5STACK RS-485 Base)**

> **Note:** Some AC units provide power through the V1/V2 lines, which you can use to power your ESP device. However, if your AC unit does not have these lines, you will need to use an external power source.

> **Note:** If your AC unit does not have the V1/V2 lines, and you are using a NASA-based device, you can obtain 12V power internally from the unit’s connector as demonstrated in [this discussion post](https://github.com/lanwin/esphome_samsung_ac/discussions/39#discussioncomment-8383733).

![M5STACK Wiring Diagram](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/assets/32042186/42a6757d-bfcf-4a29-be87-cf1b204e248a)

## Important Considerations

- **Wiring:** Ensure that your wiring connections are tight and secure. Loose connections can lead to unreliable communication between your ESP device and the AC units.
- **Pin Selection:** Double-check that you are connecting to the correct pins (F1/F2) on your AC unit and not to the pins for a remote control unit or another system.

For additional wiring tips and information, visit our [Troubleshooting](Troubleshooting.md) page.
