### Do I need to power cycle my Samsung devices to make it work?
No, a power cycle is not necessary. However, the devices should be turned on to properly establish communication.

### Does this integration work with Samsung heat pumps?
Yes, while this integration was primarily designed for air conditioners, it has been reported to work with Samsung heat pumps as well.

### Do I need an ESP device for each indoor unit?
No, if all your indoor units are connected to the same outdoor unit, you only need a single ESP device. However, if there are separate outdoor units, you will require one ESP for each outdoor unit.

### Do I need to turn off my climate devices when connecting the ESP?
It’s not mandatory to turn off the devices, but it is advised. The F1/F2 connectors are near 240V AC lines, which can be dangerous. For safety reasons, it’s best to disconnect the unit from power during installation.

### What if my device doesn't have F1/F2 connectors?
Without the F1/F2 connectors, this integration will not work. You may want to consult your device manual or manufacturer for further assistance.

### How can I confirm if my AC unit uses the NASA or Non-NASA protocol?
You can use the [Samsung AC Model Checker](http://git.samsung-hvac.aran.net.tr/model-checker/) to automatically check the classification of your Samsung AC model (NASA, Non-NASA, or Other).

### My ESP device is not receiving messages, what should I do?
- Double-check the wiring connections, as loose connections can cause communication issues.
- Ensure you are connected to the correct pins and cables on your outdoor unit, not those of a remote control unit.
- Try swapping the F1 and F2 cables.
- Adjust the baud rate from 9600 to 2400 if you’re using older hardware that may require a lower baud rate.
- For boards like NodeMCU, disable serial logging as it can interfere with the RS485 communication. Add `baud_rate: 0` to the logger section in your YAML file.

### What can I do if none of these solutions work?
Enable debugging in your YAML file to view all data received via RS485. This helps in identifying the root cause of the problem. 

```yaml
debug:
  direction: BOTH
  dummy_receiver: false
    after:
      delimiter: "\n"
      sequence:
      - lambda: UARTDebug::log_hex(direction, bytes, ' ');
```
---

By following the steps above, you can diagnose and resolve common issues effectively.
