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

