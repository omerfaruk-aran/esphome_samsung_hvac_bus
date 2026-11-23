This page contains helpful troubleshooting tips for resolving common issues that may arise during the installation and operation of the ESPHome Samsung HVAC integration.

## Common Issues and Solutions

### Wiring and Connectivity Problems

- **Check Your Wiring**  
  Ensure that all connections are securely fastened. Loose wires can cause communication failures. Double-check that you have connected the appropriate pins or cables to the outdoor unit (usually F1/F2), and not to the pins or cables meant for a remote control unit.

- **Test with Reversed F1/F2 Connections**  
  If communication is not working, try swapping the F1 and F2 connections to see if it resolves the issue.

### Baud Rate Mismatch

- **Adjust the Baud Rate**  
  Some older Samsung HVAC units use a lower baud rate (such as 2400 instead of the default 9600). Change the `baud_rate` setting in your YAML configuration if you suspect a baud rate mismatch.

### Serial Logging Conflicts

- **Disable Serial Logging**  
  For some boards, like NodeMCU, serial logging may interfere with RS485 communication. You can disable serial logging by adding `baud_rate: 0` to the logger section of your YAML file.

### Debugging Data Reception

- **Dump Incoming Data**  
  Add the following YAML configuration to the UART section of your YAML file to log all received RS485 data. This can help you determine if any data is being received and assist in troubleshooting:

  ```yaml
  uart:
    rx_pin: GPIO3
    baud_rate: 9600
  debug:
    uart:
      direction: BOTH
      after:
        delimiter: "\n"
      sequence:
        - lambda: UARTDebug::log_hex(direction, bytes, ' ');
  ```

### Live NASA Diagnostics

To opt in, merge this snippet into your existing YAML, keeping your `devices` and other settings.
Rebuild, then open the ESPHome logs through the native API and look for `NASA_DIAG` under `samsung_ac.diagnostics`.
The flag defaults to `false` and does not apply to Non-NASA frames.

```yaml
logger:
  level: DEBUG

samsung_ac:
  debug_log_diagnostics: true
```

This passive log reports accepted NASA frames as they arrive, including repeats.
It sends no queries, creates no entities, and does not publish via component-specific MQTT debugging.
ESPHome can still forward these records if MQTT log forwarding is configured.
Legacy log options are independent: `debug_log_messages`, `debug_log_undefined_messages`, and `debug_log_messages_raw` can stay off.
`debug_log_messages_on_change` does not filter `NASA_DIAG`.

Use `dt` to distinguish notifications and responses from read, request, and write commands.
Empty ACK and NACK frames appear as `fields=0`.
A decoded field is a bus value, not proof of a physical measurement or command success.
Do not interpret unknown fields, raw-only values, or unverified markers as measurements.
Field availability and meaning can vary by model. Do not infer refrigerant charge, delivered capacity, or installation health from these logs.

#### Compare zones during a cooling cycle

With one indoor unit calling for cooling, compare records for each source address over several minutes:

- Compare the indoor setpoint (`0x4201`), room temperature (`0x4203`), and thermostat demand (`0x4028`).
  Demand is not the indoor unit's power state. The reported mode (`0x4002`) is distinct from the selected mode (`0x4001`).
  S-NET Pro 2 labels zero `Unknown` for reported mode and `Auto` for selected mode.
- Compare the two evaporator pipe temperatures (`0x4205`, `0x4206`). These are not air temperatures.
  S-NET Pro 2 hides outlet readings below `-41 °C`; the log still shows the decoded value and the raw bytes.
  A [capture from one EHS system](https://community.openenergymonitor.org/t/samsung-ashp-nasa-messages-updated-table/29510) shows `-50 °C` for a disconnected outlet sensor. Do not treat this as a universal rule.
- On the outdoor address, compare the operating state (`0x8001`), heating/cooling mode (`0x8003`), compressor frequency (`0x8238`), and reported power (`0x8413`).
  Compare readings over time rather than treating records from different addresses as simultaneous.

`0x8413` is reported electrical power in W. `0x8414` is reported accumulated energy in Wh, displayed in kWh.
The fields do not establish which loads are included, the power averaging interval, or the energy counter reset policy.
`0x4027` matches the filter warning on documented models. `0x4060` preset names depend on the model and controller.

For the source of the decoded values, see the [NASA protocol notes](nasa/samsung_nasa_protocol.md) and the [S-NET Pro 2 message dictionary](https://github.com/pantherale0/pysamsungnasa/blob/931280c03c37c15144d6952035462eb77d7be302/info/snet/NASA.ptc).

### NASA Service Telemetry in Home Assistant

Set `samsung_ac.telemetry: true` in your existing YAML to add these read-only diagnostic entities for configured NASA indoor and outdoor addresses.
This works with `debug_log_diagnostics` off and sends no queries.
Entities have no value until their unit reports a valid reading. A model may not report every field.
Numeric entities publish at most once every 10 seconds. Explicitly configured sensors for the same message take precedence.

- Indoor expansion valve position (`0x4217`, `0x4218`) uses the original counts, not a percentage of valve opening.
- Outdoor expansion valve position (`0x8229`–`0x822D`) uses original counts per channel. A channel number does not identify a room or indoor address.
- Indoor room sensor selection (`0x4076`) reports the configured source: indoor unit or wired controller. This does not change which temperature the integration uses.
- Hot-gas valve 1 drive (`0x8017`) reports an on/off signal, not physical valve position. Values other than `0` and `1` remain unmapped.
- Outdoor liquid line temperature (`0x821C`) uses tenths of a degree Celsius on the bus.
- Outdoor high and low pressures (`0x8206`, `0x8208`) use tenths of `kgf/cm²` on the bus, with no offset. They are not absolute-pressure or refrigerant-charge estimates.
- Outdoor test completion (`0x8046`) says whether the test completed, not whether it passed.
- Outdoor overload detection (`0x82F5`) shows the high-byte count and low-byte status as text. The status values have no assigned meanings.
- Indoor error history (`0x461E`) shows the last five-slot snapshot received in a valid indoor response or notification.
  The slots appear as three-digit hexadecimal codes in payload order, as displayed by Samsung S-NET Pro 2.
  The history does not identify empty slots, when errors occurred, or whether any error is active.

`NASA_DIAG` still preserves the raw values, including unrecognized states and the full error-history payload.
Automatic EEV and liquid-line entities use the display ranges from Samsung S-NET Pro 2: 0–3000 indoor counts, 0–10000 outdoor counts, and −41 to 150 °C.
These ranges are display rules, not fault thresholds.

## Identifying Indoor Device Addresses

- **Monitor for Discovered Devices**  
  Make sure your ESP device is successfully receiving packets. Check for log messages indicating discovered indoor addresses (e.g., `indoor: 20.00.00, 20.00.01`). If you don’t see these messages, there may be a problem with your hardware connections or configuration.

## Still Having Problems?

If none of the above steps resolve your issue, please consider the following:

- **Recheck Your Hardware Installation:** Verify that all connections and components are correctly assembled.
- **Refer to the [FAQ](Frequently-Asked-Questions-(FAQ).md) Page:** Review the FAQ for answers to common questions and additional guidance.

If you’re still experiencing issues, feel free to open a discussion in our [GitHub Discussions](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/discussions) for further assistance.

---

By following these troubleshooting steps, you should be able to resolve most issues encountered with the ESPHome Samsung HVAC integration.
