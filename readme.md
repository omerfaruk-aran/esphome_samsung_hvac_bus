# This repo is outdated and doesn't work. I moved to https://github.com/Beormund/esphome-samsung-nasa 


# ESPHome Samsung AC

This project offers a [ESPHome](https://esphome.io/index.html) component for integrating Samsung air conditioners into your home automation system. It provides the ability to monitor and control your AC units effortlessly. 

This component is designed to connect to the F1 and F2 communication bus between the indoor and the outdoor devices.

Samsung has employed different software protocols for their AC devices over the years. The older devices utilize the Non NASA protocol, while the newer ones utilize the NASA protocol. This ESPHome component is designed to support both protocols, ensuring compatibility with a wide range of Samsung AC units.

## Current Features

The current implementation offers the following features:

- **Mutlisplit Support:** One ESP can control all indoor units connected to the outdoor unit.

- **Temperature Monitoring:** You can monitor the room temperature.

- **Humidity Monitoring (NASA Protocol):** For systems using the NASA protocol, the controller allows you to monitor humidity levels, providing additional environmental insights.

- **Target Temperature Adjustment:** You have the ability to change the target temperature, allowing you to set your desired climate.

- **AC Mode Control:** The controller enables you to change the AC mode, giving you control over cooling, heating, or other operational modes.


## Hardware installation

An ESPHome compatible device and an RS-485 to (TTL) serial adapter is required to run this project. While its possible to run it on an ESP8266 its better to chose an ESP32 since it handles the incoming message stream better (cause it has more CPU power and RAM).

### We recommand to use the M5STACK ATOM Lite + M5STACK RS-485 kit
Its cheap, comes with a tiny case (which can fit inside an indoor unit) and allow directly to use the 12V comming from the V1/V2 lines which some AC units provide.

1. Purchase the following components and stack them:
   - **M5STACK ATOM Lite** - [Aliexpress](https://a.aliexpress.com/_mO88aeK), [M5STACK store](https://shop.m5stack.com/products/atom-lite-esp32-development-kit), [documentation](https://docs.m5stack.com/en/core/ATOM%20Lite)
   - **M5STACK ATOM RS-485 Base** - [Aliexpress](https://a.aliexpress.com/_mLhOZQA), [M5STACK store](https://shop.m5stack.com/products/atomic-rs485-base), [documentation](https://docs.m5stack.com/en/atom/atomic485)

1. Connect the components as follows:
   - Connect F1 on the AC unit to A on the M5STACK controller.
   - Connect F2 on the AC unit to B on the M5STACK controller.
   - Connect V1 on the AC unit to DC on the M5STACK controller.
   - Connect V2 on the AC unit to G on the M5STACK controller.

<img alt='M5STACK Wiring Diagram' src='https://github.com/lanwin/esphome_samsung_ac/assets/32042186/42a6757d-bfcf-4a29-be87-cf1b204e248a' width='400'>

## Software Installation

Follow these steps to install and configure the software for your AC unit controller:

1. **Create a New ESPHome Device:**
   - Begin by creating a new ESPHome device in your Home Assistant instance or ESPHome command line tool.
   - Use the configuration from [example.yaml](https://github.com/lanwin/esphome_samsung_ac/blob/main/example.yaml) file as a template and copy over the `api` and `ota` sections from the newly created YAML. 

1. **Deploy and Boot:**
   - Deploy the configured firmware to your ESP device.
   - After deploying, power on the ESP.

1. **Check the Log:**
   - Monitor the log output of your ESPHome device. You should see yellow log messages indicating the reception of data packets.
  
1. **Identify Indoor Device Addresses:**
   - Wait for a minute and watch for purple log messages and the "Discovered devices" seciton in the format: "  indoor: 20.00.00, 20.00.01" or "  indoor: 00, 01".
   - These are the addresses of your indoor devices.
   - If you see only see - look into the Troubleshooting section below.
  
1. **Update Your YAML File:**
   - Copy the address block containing the indoor device addresses from the log.
   - In your ESPHome configuration YAML file, create a section for each indoor unit using the copied addresses.
   - Assign meaningful names to each unit based on the rooms they control.
   - Customize the configuration properties as needed for each unit.

1. **Remove Unneeded Properties:**
   - Review and clean up the configuration by removing any properties that are not relevant or needed for your setup.

## Troubleshooting

* Check your wiring (I had a lot problems cause the wire connection was loose)
* Check that you really connected to the same pins/cables as our outdoor device (usally F1/F2). Not to the pins/calbes of a remote control unit.
* Test if swapping F1/F2 helps
* Change **baud_rate** from 9600 to 2400 (some older hardware uses a lower baud rate)
* For some boards (like NodeMCU) you need to disable serial logging, since it blocks the pins required for the RS484 serial communication. Just add `baud_rate: 0` to the logger section.
* Add the following to your yaml which dumps all data which is received via RS484 to logs. This helps to check if you get any data. This also helps when reporting problems.
```yaml
  debug:
    direction: BOTH
    dummy_receiver: false
      after:
        delimiter: "\n"
        sequence:
        - lambda: UARTDebug::log_hex(direction, bytes, ' ');
```

## FAQ

* **Did I need to power cycle my Samsung devices to make it work?** No, but they should be turned on.
* **Did this works also with Samsung heat pumps?** Yes, while it was not desinged in the first place for them, we have reports that it also works.
* **Did I need a ESP for each indoor device?** When all your indoor devices are connected to the same outdoor device, then you need just one. Otherwise you need one for each outdoor device.
* **Did I need to turn off my climate devices when I connect the ESP?** No, but its adviced to do so, cause there is no garantee that it will not harm you Samsung hardware.

## Development

The following YAML configuration is not included in the example since they are for development purposes. 

```yaml
# All this values are optional. Only use the ones you need.
samsung_ac:
  # Sends all NASA package values to MQTT so the can be analysed or monitored.
  debug_mqtt_host: 10.10.10.10
  debug_mqtt_port: 1883
  debug_mqtt_username: user
  debug_mqtt_password: password

  # Prints the parsed message data to the log
  debug_log_messages: true
  # Prints the binary message data (HEX encoded) to the log
  debug_log_messages_raw: true
  
  # Prints parsed read value only if it match the source address (optional) AND if it's exactly the number set on HA user interface
  debug_number:
    - name: "debugFrom10"
      source: "10.00.00"
    - name: "debugFrom20"
      source: "20.00.00"

```

## NASA vs Non NASA

It took me a while to figure out what the difference is. NASA is the new wire protocol which Samsung uses for their AC systems.
The old units are using the so-called Non NASA protocol. The protocols share some aspects like the start and end byte. But the
newer NASA protocol is more complex and allows more data to be transferred and more units to communicate.

### NASA

The NASA protocol is pretty generic. Its basicaly desinged to transport variables which are a key (number) and a value (with 
a datatype like Enum, Int, Long, Bytes). All meaning is defined to the keys. If you want to know the room temperature you need 
to know the number and wait for it.

[Foxhill67](https://github.com/Foxhill67) started to document the NASA protocol [here](https://wiki.myehs.eu/wiki/NASA_Protocol).

### Non NASA

The Non NASA protocol is specifically desined to transport AC data.

[DannyDeGaspari](https://github.com/DannyDeGaspari) started toe document the Non NASA protocol (but from wall controller side) [here](https://github.com/DannyDeGaspari/Samsung-HVAC-buscontrol).

## Credits

Thanks goes to DannyDeGaspari https://github.com/DannyDeGaspari/Samsung-HVAC-buscontrol. He made the initial attempt to describe the
older Non NASA protocol.

Thanks goes to matthias882 https://github.com/matthias882/some_esphome_components. He made an an basic ESPHome component
for the older Non NASA protocol which was a perfect source for start playing and learning to communicate with the AC
and use ESPHome.
