# External room temperature (NASA, experimental)

**HARDWARE INJECTION NOT TESTED by this implementation. Supervised experiments only.**
This optional feature sends an external Celsius measurement to one NASA F1/F2 indoor
unit. It is disabled by default, including after every ESP restart. It does not
change the climate entity's setpoint, temperature offset, display-light switch, or
other indoor units. Non-NASA and EHS/F3/F4 zone control are outside its scope.

**Stopping writes is not a verified fallback to the indoor NTC.** Community reports
describe the last injected value remaining latched, including continued heating,
until the air conditioner is power-cycled. Turning this feature OFF, disconnecting
Home Assistant, rebooting ESPHome, or a sensor fault only stops this sender. None
proves that the unit has returned to its physical sensor. Unattended/production use
is blocked by the absence of a proven fallback and physical regulation tests.
Do not configure automations to enable this switch or re-enable it after faults.

See [Internal sensor restoration investigation](External-Temperature-Recovery.md)
for the evidence matrix, threat model, research exclusions and physical recovery
prerequisites. No model-independent release method has been verified.

## Evidence and protocol boundaries

The primary investigation is [issue #338](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338),
with context in [issue #130](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/130)
and [discussion #261](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/discussions/261).

| Item | Interpretation and confidence |
| --- | --- |
| `0x4203` | Candidate external temperature, Variable, signed tenths °C. A community F1/F2 experiment reports successful `Write` (`DataType=2`) from `50.00.00` on AJ080TXJ4KG / R09TXEAAWKNEU. This is model-specific evidence, not a universal Samsung contract. |
| Source class `0x50` | WiredRemote. Configured explicitly per feature packet; normal JIG source `80.ff.00` and ordinary climate requests are preserved. An unused address must be established by bus inspection; no automatic controller impersonation. |
| `0x4204` | Unit-reported room/control temperature. Observed only. A post-send matching value is evidence of reported value adoption, not proof of physical regulation. |
| `0x4201` | Setpoint. Never used as the external temperature destination or fallback. |
| `0x4076`, `0x423A`, `0x42DA`, `0x4118` | Historical zone/source-selection experiments, especially EHS/F3/F4. No speculative write, read-to-write assumption, EEPROM change, or installer/service option is implemented. |

Community traces show cooling following the value while heating sometimes echoes
`0x4203` but retains the internal `0x4204`. Other models reportedly need installer
configuration; there is no validated universal setting. This feature deliberately
does not program those options. A successful protocol transaction does not establish
that either heating or cooling regulates safely on a particular unit.

The existing NASA encoder, CRC-checked decoder, UART silence arbitration, and send
queue are reused. There is no separate byte-scanning decoder. Every injection is a
single `Write`, with no retry or fixed duplicate send. Requests to other registers
retain their existing behavior.

## Configuration

Do not add this block to a live installation until bus conflicts and a supervised
recovery/power-isolation procedure have been reviewed. The acknowledgements are
required configuration gates, not evidence that an installation is safe.

This pilot configures **Office `20.00.01` only**. Living `20.00.02` and Kitchen
`20.00.00` must keep their existing definitions without an external-temperature
block. Do not change existing entity IDs when adapting it.

Example using a real Home Assistant temperature source, added to an otherwise
working ESPHome configuration with `api:` and the existing UART/bus definition:

```yaml
sensor:
  - platform: homeassistant
    id: office_external_temperature
    entity_id: sensor.office_temperature
    internal: true

samsung_ac:
  devices:
    - address: "20.00.01"
      # Keep the existing climate and other entity definitions here.
      external_room_temperature:
        experimental: true
        acknowledge_latched_temperature: true
        controller_absent_confirmed: true
        sensor: office_external_temperature
        source_address: "50.00.00"  # Example ONLY: verify it is unused on this bus.
        update_interval: 45s
        max_age: 120s
        observe_for: 60s
        response_timeout: 5s
        verification_timeout: 90s
        minimum: 5
        maximum: 40
        max_step: 5
        tolerance: 0.5
        debug: true
        debug_raw: false
        enable_switch:
          name: "Office Experimental External Temperature Sender"
          id: office_external_temperature_sender
        input_temperature:
          name: "Office External Temperature Input"
        sent_temperature:
          name: "Office External Temperature Last Sent"
        reported_temperature:
          name: "Office External Temperature Unit Report"
        fresh:
          name: "Office External Temperature Fresh"
        reported_match:
          name: "Office External Temperature Reported Match"
        status:
          name: "Office External Temperature Status"
        failures:
          name: "Office External Temperature Failures"
```

For a native ESPHome sensor, replace the `sensor:` section above with a real sensor
that publishes Celsius. For example, an externally wired SHT3x at I²C address `0x44`:

```yaml
i2c:
  sda: GPIO26  # Example pins: use the pins appropriate for your hardware.
  scl: GPIO32

sensor:
  - platform: sht3xd
    address: 0x44
    update_interval: 30s
    temperature:
      id: office_external_temperature
      internal: true
```

Use the source's **raw value in Celsius**: source filters, calibration filters,
heartbeats and throttle outputs are not used. Calibrate at the measurement source
before publishing its raw state. Direct use of a bus room-temperature entity or
this feature's own diagnostic output is rejected to prevent feedback. Indirect
feedback through HA/templates cannot be detected and must be excluded manually.

Freshness starts on an actual `add_on_raw_state_callback` event, never on polling a
cached `.state`, filter heartbeat, or enabling the feature. A **new raw event after
manual enable** is required before the first write. The HA integration can deliver
a cached snapshot on reconnect, and neither ESPHome nor this protocol proves the
age of the physical measurement behind that snapshot. Do not use an HA helper or
template that periodically republishes cached temperatures as a freshness source.
If the real sensor does not publish unchanged readings, this feature will expire
them; arrange genuine periodic measurement reports or keep it disabled.

| Option | Default / allowed range |
| --- | --- |
| `sensor`, `source_address`, `enable_switch` | Required. Source `50.cc.aa`, target `20.cc.aa`; `ff` channel/device addresses forbidden. |
| Three acknowledgement flags | Required `true`. |
| `update_interval` | 45 s; 30–300 s. |
| `max_age` | 120 s; at least update interval, at most 600 s. Applies to raw input and known unit power/mode. |
| `observe_for` | 60 s; 30–300 s after bus processing starts. Passive controller detection. |
| `response_timeout` | 5 s; 1–10 s from actual UART send. |
| `verification_timeout` | 90 s; 45–300 s from actual UART send for reported temperature to follow. |
| `minimum`, `maximum` | 5–40 °C software envelope; configurable within −20–60 °C, minimum below maximum. This is not a hardware rating. |
| `max_step` | 5 °C between accepted raw events; 0.1–20 °C. |
| `tolerance` | 0.5 °C for reported match; 0.1–2 °C. |
| `debug`, `debug_raw` | Both false by default. Raw logging additionally requires VERY_VERBOSE logger level. |
| Diagnostic entities | All optional. `enable_switch` always starts OFF; restore modes and inversion are rejected. |

## Runtime and diagnostics

1. Remain disabled at boot; observe the bus for the configured duration. Seeing
   **any** WiredRemote `0x50` or WiFiKit `0x62` source blocks the feature until an
   ESP restart and manual conflict review, even if it addresses another unit.
   This conservative detector is best effort: a silent controller can appear later,
   lost packets can hide one, and passive absence cannot prove address ownership.
   Exact recent loopback of one of this bridge's writes is excluded across configured
   units. Two independent bridges with identical addressing cannot be distinguished.
2. Require a fresh, finite, plausible measurement and recent indoor power ON plus
   Cool or Heat notifications. Auto, Dry, Fan, unknown/stale operating state, or a
   controller conflict prevents enable. Enabling does not change the AC mode/power.
3. After manual enable, wait for a new raw measurement. Queue one Write. Recheck
   freshness, power, mode, conflicts, session and latest rounded sample immediately
   before UART transmission. Canceled/superseded work cannot send later. Queue
   admission is limited to 16 total pending entries; injection waiting 5 s faults.
4. Require `Response` containing `0x4203` and the exact encoded value from the target
   indoor source, addressed to the configured source, with the sent packet number.
   ACK never confirms acceptance. NACK, unsupported `0xFFFF`, mismatch, or missing/
   late response faults without retry. Packet number correlation is bounded in time;
   NASA's 8-bit sequence is not an authenticated transaction identifier.
5. Separately require a new, post-send unit `Notification` for `0x4204` within the
   tolerance and deadline. `reported_match` compares that report to the last sent
   value; the `reported_match` phase additionally requires the correlated echo.
   Neither proves physical control or source selection. Heating/cooling must be
   assessed separately. A later divergent report faults.
6. Refresh no faster than the configured interval after actual send and only after
   both pieces of evidence. Sensor expiry, invalid/implausible input, bus failure,
   unit power OFF, mode change, invalid operating report or verification failure
   disables the sender and requires manual rearm after investigation.

`sent_temperature` is the last actual UART transmission, not a queued value.
`reported_temperature` is the last valid observed `0x4204`, which may become stale;
it is not substituted into climate state. `fresh` refers only to the raw input.
`reported_match` is cleared on a new transmission, fault or manual disable.
`status` includes the phase and reason; `failures` counts session faults and resets
on ESP restart. Diagnostics are published on change to avoid routine log floods.
The enable switch being OFF means this sender has stopped, **not** that the unit
uses its internal sensor. Other temperature/UI values can remain stale.

No guessed fallback temperature, setpoint copy, automatic power-off/on, service-mode
write, auto rearm, or automatic display suppression is implemented. Normal climate
commands remain available to the operator; their power/mode reports invalidate an
active injection session. Sensor-step validation retains its last accepted baseline
across sessions; investigate a large discontinuity instead of repeatedly rearming.

## Logs and supervised pilot procedure

For transaction logs add to the existing logger configuration:

```yaml
logger:
  level: DEBUG
  logs:
    samsung_ac.external_temp: DEBUG
```

INFO/WARN record starts, enable/disable, faults, controller detection and the
unverified fallback condition. With `debug: true`, DEBUG includes input validation,
freshness transitions, scheduling, source/destination, type, packet number, encoded
value, echo correlation, reported difference, mode and power. For an explicitly
requested raw capture use `debug_raw: true`, `logger.level: VERY_VERBOSE`, and the
tag level `VERY_VERBOSE`. RX raw lines are decoded/validated frame re-encodings;
the existing raw UART logger is the source for discarded/CRC-failing bytes.
There is no credential/entity-name logging in this feature.

Illustrative sequence, **not a hardware capture**:

```text
state=awaiting_new_sample reason=manual_enable_requires_new_sample
event=sent source=50.00.00 class=50 type=Write msg=4203 encoded=213 value=21.3 pn=7 retry=0
state=awaiting_report reason=echo_received_control_unverified
requested=21.3 reported=21.3 difference=0.0 echo_correlated=1 mode=1 power=1 control_loop=unverified
state=reported_match reason=echo_and_report_match_not_control_proof
event=fault reason=sensor_stale last_sent=21.3 fallback=unverified latched_possible=1 manual_rearm=required
```

Before any Office pilot, capture the idle F1/F2 bus, identify every controller and
verify addressing/electrical compatibility. Establish a supervised manufacturer-
appropriate power-isolation/recovery procedure first. Keep Living and Kitchen
outside the feature. Observe baseline power/mode, internal report, external source,
setpoint, room conditions and actuator/compressor behavior. Manually enable Office
only and retain raw traces with timestamps. Independently test Cool and Heat and
compare the actual regulation response with a separate physical thermometer.

Exercise disconnect, NaN/out-of-range source, stale source, power/mode changes,
missing echo/report, restart and manual disable while attended. Expect the sender
to stop; separately determine whether the **unit** still holds the injected value.
Do not infer safe fallback from an OFF switch, ACK, matching report, or lack of new
writes. No physical test, deployed firmware, flash or recovery procedure has been
performed or certified by the offline regression tests.

## Developer verification

Run `test/test_external_temperature.cmd` on Windows with g++ available, or
`sh test/test_external_temperature.sh` on Linux. The native suite covers disabled
startup/rearm, signed encoding/CRC, Cool/Heat evidence fixtures, exact correlation,
ACK/NACK, invalid/stale inputs, rollover, queue guards/timeouts, conflicts, operating
state changes and multi-unit isolation. Run the real ESPHome schema test
`python test/test_external_temperature_schema.py` alongside the existing NASA,
display, Non-NASA queue and climate custom-mode regressions. The existing Arduino
and ESP-IDF CI configs include an unavailable test source and all diagnostics to
compile the optional feature while the sender remains OFF. These are software
fixtures, not hardware validation.
