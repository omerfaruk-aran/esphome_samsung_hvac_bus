# Internal sensor restoration after external temperature injection

Investigation date: 2026-11-04. Scope: PR #406, NASA F1/F2 sender.
**HARDWARE INJECTION AND NTC RESTORATION NOT TESTED by this implementation.**

## Decision

No documented, nonvolatile-safe, model-independent NASA method for restoring the
indoor NTC after `0x4203` injection was found in the sources below. This is a bounded
research finding, not proof that Samsung has no such mechanism. Missing evidence
includes a manufacturer-defined release message, its persistence semantics, and
physical recovery tests on the actual indoor model and firmware.

Keep the sender disabled by default and the PR in draft. There is no automatic
fallback packet, sensor-selection command, service-option change, or power cycle.
Unattended injection remains blocked. Software stopping safely does not guarantee
that the air conditioner reaches a safe physical state.

## Three different outcomes

| Outcome | Evidence required | Current implementation |
| --- | --- | --- |
| Stop transmission | No new writes; queued generations canceled | `enable(false)`, faults, and send-time guards stop this sender. This does not clear a value already held by the indoor unit. |
| Change selected sensor | Model-specific, manufacturer-defined selection semantics and readback | Not implemented. Echoing a value or displaying it does not identify the physical sensor selected in every control path. |
| Restore physical NTC control | Independently measured room conditions and regulation response in each mode, with known configuration and no injector | Unverified. An OFF switch, reboot, ACK, `Response 0x4203`, or matching `Notification 0x4204` cannot certify this. |

## Evidence matrix

Confidence describes the evidence within its stated scope, not universal applicability.
Manufacturer documentation is high confidence for that product's documented interface;
community observations are medium confidence for the reported setup; extrapolations
and unnamed-model claims are low confidence. No row verifies fallback on Office.

| Source | Scope and observation | Confidence / limitation |
| --- | --- | --- |
| [dettofatto, #338](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-4883277024) | AJ080TXJ4KG with R09TXEAAWKNEU: F1/F2 `Write 0x4203`, source class `0x50`; `0x4204` reportedly follows after about 30 seconds. | Medium, model-specific injection evidence. The author later clarifies this was cooling testing; no restoration procedure demonstrated. |
| [Latching and decoder correction](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-5624904752) | Reporter says the last value persists when transmissions stop. Earlier YAML scanned arbitrary byte pairs and could fabricate plausible temperatures. | Medium hazard evidence; indefinite persistence is a reporter claim, not a manufacturer guarantee across models. Only CRC-validated, message-aligned traces are usable. |
| [Cool/Heat setup](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-5632219119), [behavioral follow-up](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-5633289467) | AJ040TXJ2KG/EU with AJ026TN1DKG/EU: heating echoes injected values but reportedly regulates using internal temperature; cooling follows external values. | Medium; separate mode behavior and fan-response observations, not controlled manufacturer validation. Echo is insufficient. |
| [WiFi kit conflict / scope correction](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-5632518200) | MIM-H04EN `62.00.64` identified as a possible competing controller; original author had not tested heating. | Medium for observed topology; register contention in that installation remains a hypothesis. |
| [Service-setting report](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-6401589952), [follow-up](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-6473225265) | Posters report heating adoption after an IR service/EEPROM option change; follow-up warns about latching until power cycle. | Low for transfer to other units: complete model/firmware/option applicability is not established. No procedure or option values are prescribed here. Persistent option programming violates this investigation's scope. |
| [Continued heating report](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-6473329744) | Unit reportedly stuck at 21 degrees with continued full heating until reset. | Medium hazard report, low confidence about exact reset semantics/model because these are unspecified. It does not establish an emergency recovery method for another system. |
| [F3/F4 Modbus experiment](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/338#issuecomment-3961943767) | Gateway-based external sensing after controller configuration. | Medium for that gateway installation. It is not direct F1/F2 `0x4203` release evidence. |
| [70p4z EHS zone experiment, #130](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/130#issuecomment-3047925736) | F3/F4 zone Notifications: `0x423A` with `0x4076`, and `0x42DA` with `0x4118`; `0x4203` described as effectively read-only on MIM E03CN. | Medium for the reported zone path, low for extrapolation to split F1/F2. No release/persistence proof. Sensor disconnection experiments in that thread are not recommended. |
| [Intesis INMBSSAM001R100 manual, r1.4, section 4.2.3, p.12](https://www.hms-networks.com/docs/default-source/products/intesis/manuals-and-guides---manuals/user-manual-inmbssam001r100.pdf) | Modbus register 22 (protocol) / 23 (PLC) starts at `-32768` (`0x8000`), meaning no external reference through that gateway. The selected alternative can be return-air or a configured remote sensor. | High for gateway semantics. The manual supplies no corresponding NASA reset frame. **Do not put `0x8000` into NASA `0x4203` as a reset.** |
| [Samsung MWR-WE10 installation manual, DB98-32811A-08, printed p.20](https://s3.amazonaws.com/samsung-files/Tech_Files/Controls/Local%20Control/Installation%20Manuals/MWR-WE10/MWR-WE10_IM_32811A-08_ENGLISH_140613.pdf) | Documents indoor versus wired-controller sensor selection and averaging options in that controller's service settings. | High for MWR-WE10 options, not applicability to an unspecified indoor unit. It does not document a generic volatile NASA release after injection. |

## Research candidates: not transmission instructions

| ID | Reported context | Missing proof |
| --- | --- | --- |
| `0x4076` | EHS zone-1 sensor enable/selection candidate | No evidence that clearing it releases F1/F2 `0x4203`, is volatile, or is valid on the pilot model. |
| `0x423A` | EHS zone-1 temperature candidate | A temperature field is not a release operation; no reset sentinel established. |
| `0x4118` | EHS zone-2 sensor enable/selection candidate | No cross-model mapping, persistence contract, or NTC recovery evidence. |
| `0x42DA` | EHS zone-2 temperature candidate | No reset semantics established. |

F1/F2 and F3/F4 are distinct interfaces; EHS hydraulic zones and split-unit room
control must not be treated as interchangeable because they share message IDs.
Packet type, addresses, controller firmware, bus electrical interface and service
configuration are part of the evidence. Never derive a reset by inverting a reported
enable bit. `0x4201` is the setpoint, not an internal NTC value to copy as fallback.
Zero, `0xFFFF`, `0x8000`, and the last measured room value are not proven release
tokens. No new candidate packet is built or transmitted by this work.

Service-setting prerequisites must be verified against the exact indoor/outdoor
and remote models and manual revisions by Samsung service personnel. Record the
existing configuration and heating compensation/averaging behavior. Do not change
EEPROM, installer options or sensor wiring to make this experiment pass. If the
existing configuration cannot support the test without those changes, stop here.

## Fail-safe threat model

| Failure | Existing software response | Residual physical risk / prerequisite |
| --- | --- | --- |
| HA state stops arriving | Input expires after `max_age`; sender faults and requires rearm | Last injected value can remain latched. HA availability alone is not a safety interlock. |
| HA republishes stale cache or reconnect snapshot | Raw-event timestamp can become fresh; a new event after enable is required but does not prove physical measurement age | Unresolved. Validate acquisition timestamps/sequence and upstream sensor health externally; do not use periodic cached helpers as freshness evidence. |
| Lost Zigbee reports / dead battery | No raw events eventually causes expiry; unchanged genuine reports remain valid | Cached Zigbee/HA republishing can mask loss. Monitor actual acquisition and battery/link independently; software cannot infer this from a float. |
| ESP reboot | New instance starts OFF; prior session is not restored | Indoor state is not reset by ESP reboot. Do not rearm to conceal the hazard. |
| ESP crash, power loss or failed transceiver | Software watchdog/fault handling may not execute; no reliable final packet | Unit may keep operating with a frozen temperature. Recovery cannot depend on the failed ESP, HA, or the same bus. |
| AC or whole-system power loss / partial restoration | Sender remains OFF after an ESP restart; a running ESP detects only the status it receives | Do not assume all indoor/outdoor/controllers lost power together, that the latch cleared, or that auto-restart is safe. Isolate and verify using the exact manufacturer procedure. |
| Bus busy, lost packets, conflict or duplicate address | Guarded send expiry; no retries; echo/report timeout; observed `0x50`/`0x62` conflicts block until restart/review | Silent controllers/identical frames may evade detection. Another controller can continue writing. Passive absence is not ownership proof. |
| Value latches; heating or cooling diverges | Fault disables this sender; OFF and invalidated evidence are diagnostic only | Could sustain heating/cooling. Independent thermometer, continuously attending operator and manufacturer-approved physical stop are required. |

Stopping an active injector removes future updates, including potentially useful
ones; it does not repair the unit's control reference. An HA automation or an ESP
power-OFF command shares failure paths and is not a substitute for an independent
physical recovery plan. No automatic OFF or power-cycle behavior is added.

## Prerequisites for a future supervised test

This document does not authorize deployment or injection. Begin with passive-only
capture without an `external_room_temperature` block or custom UART transmit
automations. Normal bridge controls are not a receive-only electrical interface;
avoid control actions and use a qualified receive-only capture arrangement if
absolute bus silence is required.

Before any separately authorized active test:

1. Record complete indoor/outdoor/controller model and firmware identities, bus
   topology, source addresses and existing options. Obtain the matching Samsung
   installation/service instructions and have a qualified person establish a
   recovery plan that needs no EEPROM/installer changes. Missing procedure is a blocker.
2. Identify **all** relevant power supplies and the actual isolation/stop facilities,
   including shared outdoor equipment and other rooms. Determine effects on occupants,
   auto-restart, compressor restart delays and any required discharge/wait periods
   from those model-specific instructions. No universal isolation duration is asserted.
3. Keep the injector OFF throughout planning. Establish normal NTC regulation using
   an independent calibrated thermometer and observed unit behavior. Baseline `0x4204`,
   setpoint, mode and power are supporting data, not sole proof.
4. Limit any future pilot to Office `20.00.01`; protect Living `20.00.02` and Kitchen
   `20.00.00` from shared-system effects. Set physical temperature/time abort limits
   appropriate to the installation before testing; continuously attend the equipment.
5. Capture CRC-validated addressed frames, actual measurements, controller state and
   actuator behavior with timestamps. Evaluate heating and cooling separately.

## Physical recovery decision procedure (not executed)

On unsafe regulation, stop the experiment and use the pre-established independent,
manufacturer-approved stop/isolation method. An ESP switch or IR OFF is not assumed
to clear the latch. A qualified person must apply the exact equipment isolation
procedure to all required supplies; do not hot-unplug controllers/sensors, open
energized equipment, or improvise repeated breaker cycling. Follow the applicable
wait/discharge and restart requirements rather than a generic timer.

Prevent all injector automations/bridges from resending on restoration. Re-energize
only as the manufacturer permits and under supervision. Establish normal control
against the independent thermometer with injection absent, separately in the modes
to be used; check options, alarms and any auto-restart. A cleared display or matching
`0x4204` is not sufficient. If physical regulation cannot be established, leave the
equipment safely stopped/isolated according to that procedure and escalate to
Samsung service. Community power-cycle reports justify investigating this path,
but do not certify it or prove that persistent sensor-selection options were cleared.

## Offline regression boundary

### Existing implementation reviewed

`components/samsung_ac/protocol_nasa.h` maps `0x4201` to setpoint, `0x4203`
to the experimental external input and `0x4204` to the reported room value.
`NasaProtocol::process_external_packet()` delivers validated decoded packets to
registered observers. `Samsung_AC_External_Temperature::setup()` attaches only a
raw sensor event callback; it does not restore an enabled session or seed freshness
from cached sensor state.

In `external_room_temperature.cpp`, `begin()` leaves the sender Disabled.
`enable(true)` checks `unsafe_reason()` and enters AwaitSample. A new raw event
permits Ready, then Queued. `can_send()` rechecks safety, expiry and generation at
delivery; `sent()` starts AwaitEcho only after transport delivery. Correlated
Response leads to AwaitReport; post-send matching Notification permits Following
(reported match, not physical-control verification). ACK cannot satisfy the echo
deadline. `fault()` disables and invalidates queued work. `enable(false)` clears
reported-match evidence and explicitly reports unverified fallback. There is no
RestoreNTC state, selected-physical-sensor diagnostic, or release operation.

### Test scope

The C++ `ExternalTarget` is an in-memory transport, not a serial port. Additional
tests check no release/fallback frames after disable, expiry and reboot, persistent
fault/manual rearm, and the inability to distinguish republished stale HA floats.
They prove sender behavior only; they do not simulate or certify Samsung's physical
NTC recovery. Existing NASA/display/Non-NASA suites and schema/codegen tests must
also be run. Report pre-existing failures explicitly rather than claiming all tests
pass or silently excluding them. Product runtime code and default OFF state are
unchanged by this documentation/test investigation.

### Results on 2026-11-04

Native tests used the existing LLVM-MinGW g++ runner with an in-memory target;
Python tests used ESPHome 2026.10.1. No serial/network HVAC transport was used.

| Test | Result |
| --- | --- |
| External temperature | PASS, 25 groups (21 existing plus four recovery-boundary groups) |
| NASA | PASS, exit 0 |
| NASA display | PASS, exit 0 |
| Maintained Non-NASA queue | PASS, exit 0 |
| Python schema, documentation YAML generation and climate modes | PASS, 26/26 |
| Legacy `main_test_non_nasa.cpp` | FAIL to compile, existing `Temperature` versus integer comparisons and obsolete API use, beginning at line 100. Neither that test nor the production Non-NASA code was changed. |

### Legacy baseline verification

The legacy test was rerun on both the feature checkout and an isolated source
snapshot extracted with `git archive main`; neither run used a hardware transport.
Local `main` and remote `main` were both
`01d5b23a8c72aaf3225bd06eca8c75223c224213` when checked. The feature HEAD before
this documentation/test commit was `7708675dc42f589a17e92ba3ed25e63a3eb6229c`.

`test/main_test_non_nasa.cpp` has identical contents in both branches (Git blob
`412c8f13a9f876a42f724aedbe39eb40fed6c8f0`). Both native compilation attempts
returned exit 1, beginning with the same invalid `Temperature`/`int` comparison
at line 100, followed by equivalent comparisons at lines 101-103. The legacy test
was neither skipped nor modified. Its API migration is separate from PR #406.

All maintained tests listed above pass. The broader condition **all existing tests
pass is not satisfied** because the legacy suite already fails on `main`.
This baseline blocker must be resolved separately without weakening its assertions.
Native runs also retain the six existing format-security
warnings in the test logging path. No firmware build is necessary for this patch:
only Markdown and native regression assertions changed; no production behavior changed.
