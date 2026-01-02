This section provides information on the development features, configuration settings, and debugging capabilities for contributors and developers who want to extend or enhance the Samsung HVAC ESPHome component.

## Debug Configuration

For development purposes, the component provides specific YAML configurations to help debug and monitor data communication. These configurations are not included in the standard example as they are intended only for development purposes. Use the configurations based on your need to get detailed information from the AC unit.

```yaml
# All these values are optional. Only use the ones you need.
samsung_ac:
  # Sends all NASA package values to MQTT so they can be analyzed or monitored.
  debug_mqtt_host: 10.10.10.10
  debug_mqtt_port: 1883
  debug_mqtt_username: user
  debug_mqtt_password: password

  # Prints the parsed message data to the log
  debug_log_messages: true
  # Prints the binary message data (HEX encoded) to the log
  debug_log_messages_raw: true
```

### Debug Options

- **`debug_mqtt_host`, `debug_mqtt_port`, `debug_mqtt_username`, `debug_mqtt_password`:**  
  These options allow you to send all NASA protocol packages to an MQTT server for analysis or monitoring. This is especially useful for identifying issues, tracking message sequences, and understanding packet structures.

- **`debug_log_messages`:**  
  Prints the parsed message data in a human-readable format to the logs. This option is crucial for verifying if the data being received or sent matches expectations.

- **`debug_log_messages_raw`:**  
  Prints the raw binary data in hexadecimal format. This is helpful for debugging communication issues or verifying low-level protocol details.

## Protocol Variants: NASA and NonNASA

### NASA Protocol

The NASA protocol is the current standard Samsung uses for their air conditioning systems. It uses a flexible key-value structure to exchange data, allowing a greater range of information to be transferred between the indoor and outdoor units.

For an in-depth understanding of the NASA protocol, including its key mappings and message structures, refer to [Foxhill67's documentation](https://wiki.myehs.eu/wiki/NASA_Protocol).

### NonNASA Protocol

Older Samsung AC units use the NonNASA protocol, which follows a different message structure. This protocol is less flexible compared to NASA but still supports essential functionalities like temperature monitoring and fan control.

For more information about the NonNASA protocol, refer to [DannyDeGaspari's documentation](https://github.com/DannyDeGaspari/Samsung-HVAC-buscontrol).

## Code Contributions and Best Practices

### Code Guidelines

- Follow the established project structure and conventions to maintain consistency.
- When introducing new features or fixing bugs, provide detailed comments and documentation in the code.
- Submit changes through GitHub pull requests and reference related issues to improve traceability.

### Testing Changes

- Use debug options and MQTT to track the behavior of new or modified code.
- Conduct thorough testing in different environments, especially when adding support for new hardware models or protocols.
- Verify that changes do not introduce compatibility issues with existing protocols or functionalities.

