# NASA indoor display light

`display_light` is an optional switch for NASA indoor units with an address in
`20.cc.aa` format. It uses Enum message `0x4016`: ON sends `1` (illuminated), and
OFF sends `0` (dark). It does not control the AC power or display brightness.
Non-NASA addresses are rejected during configuration validation.

```yaml
samsung_ac:
  devices:
    - address: "20.00.02"
      display_light:
        name: "Living Room Display Light"
        state_valid:
          name: "Living Room Display Light State Valid"
```

Add the block to your existing device configuration. Keep your UART, board,
network, and other device settings. Remove the entire `display_light` block to
disable the feature: no display switch, reads, or display commands are created
for devices without it. No restore command is sent at startup.

## State reporting

The component sends one Read at startup, one after each display command, and
one when the observed power state changes (including the first power report).
It does not periodically poll. Display commands use Request, not Write. Display
Requests are sent once, separately from climate fields. Concurrent climate fields
still merge into their usual Request and retain the existing ACK/retry behavior.
Combining display and climate actions does not activate the power-on workaround.

After a display Request is physically transmitted, the component waits at least
500 ms before queuing its verification Read. An ACK is receipt only and never
publishes the requested state. A lost ACK still permits the single Read. A matching
NACK stops that operation. The Read has a 2-second response window starting at
actual transmission, with no automatic retry. Bus congestion can delay transmission;
the response timer does not run while the packet is waiting in the UART queue.

Only a Response to this controller, from the intended indoor unit, with the matching
packet number and `0x4016` value `0` or `1`, confirms state. Unrelated, duplicate,
expired, or superseded responses do not confirm it. Values such as unsupported Enum
`0xFF`, a NACK, and a timeout leave the state unconfirmed. Response handling is
limited to this feature; unrelated NASA Response messages retain their existing
behavior. Rapid user changes are coalesced per device while a transaction is active.

The component does not publish an optimistic ON/OFF value. ESPHome's switch API
only carries a boolean: a frontend may show its default OFF value before the first
valid Response, and it may retain the **last confirmed** value after a command or
failed read. Neither is confirmation of the current display state. The API cannot
publish an unknown switch value. The optional `state_valid` diagnostic binary sensor
is OFF while verification is pending or unsuccessful, and ON after a valid Response.
Include it when building automations that need confirmed state. The switch exposes
explicit ON/OFF controls because a toggle based on stale state may be misleading.

Indoor units reportedly do not normally notify `0x4016`. Changes through IR or
SmartThings may remain invisible until the next triggered Read. A valid state is
a confirmed snapshot, not a guarantee that no external controller has changed it.
An explicit ON or OFF action triggers another bounded verification attempt.
NASA packet numbers are 8-bit; after reuse, an extremely late response with identical
addresses and packet number cannot be distinguished on the wire.

## Power-on behavior and compatibility

Powering on reportedly resets the light to ON. This feature reads the resulting
state; it does not automatically restore a preferred light setting. You can turn
the display OFF afterwards, which may produce an additional beep. Normal display
changes may also beep. Display control does not disable the buzzer.

The reported workaround requires setting `0x4016=1` while the unit is OFF, then
sending a single ordered Request containing `4016=0, 4001=mode, 4000=1`. It only
worked when the power-on Request actually changed the display value. Automatic
power-on handling is **deferred**: the current queue merges independent controls,
does not establish a verified OFF/precondition transaction, and cannot prevent
another controller from powering the unit on between steps. Safely implementing
this needs a separately opted-in, hardware-validated transaction model with
precondition verification and cancellation. There is no power-on workaround option
in this version, and no intermediate display write is sent automatically.

The protocol discovery was reported in
[issue #256](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/256#issuecomment-6579546576).
The contributor visually checked two indoor units; compatibility across other
Samsung models is unknown. **This implementation has NOT BEEN TESTED on hardware.**

## Manual hardware validation

1. Use the feature branch and add the YAML above with your actual indoor address.
   Keep a known-working firmware available for rollback.
2. Start with one indoor unit. Enable `debug_log_messages` and
   `debug_log_messages_raw` if needed, and record the indoor/outdoor model numbers.
3. After startup, confirm a Read to that unit and a matching Response. Check that
   `state_valid` turns ON only for `0` or `1`. Compare the switch with the panel.
4. Send OFF, then ON. Check `4016=0` and `4016=1` Requests, destination addresses,
   ACKs, and the subsequent Read/Response. Visually confirm both results and note
   beeps. An ACK without a valid Response must not count as confirmed state.
5. Turn the AC OFF and ON using ESPHome, then using IR. Check the normal light reset
   and readback; verify that the component sends no automatic display-setting
   workaround. Turn the display OFF explicitly afterwards if desired.
6. Change setpoint, mode, medium/turbo fan, swing, presets, and automatic cleaning.
   Verify existing behavior, including concurrent display and climate controls.
7. If multiple indoor units are available, operate each separately and verify no
   other unit's switch is updated by its Response.
8. On a device that does not respond or returns `0xFF`, verify `state_valid` stays
   OFF and no continuous Read/retry traffic appears. Do not inject test frames into
   a live installation.
9. Change lighting using IR/SmartThings, note the documented stale snapshot, then
   send an explicit display action to verify readback again.
10. Remove `display_light`, rebuild, and confirm all existing controls still work
    without any display-related traffic. Report observations before merging.
