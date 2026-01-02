Welcome to the **ESPHome Samsung HVAC Integration** installation guide! This guide will help you set up and configure your system smoothly.

## Step 1: Prerequisites and Compatibility
Before you start, make sure that your Samsung HVAC unit is compatible with this integration. To verify compatibility, you can refer to the [Compatibility](Compatibility.md) page which lists the supported models and additional details.

## Step 2: Hardware Installation
To establish communication between your ESPHome device and Samsung HVAC unit, follow the hardware setup guidelines provided in the [Hardware Installation](Hardware-Installation.md) page. Here, you will find detailed instructions on using the recommended **M5STACK ATOM Lite + M5STACK RS-485 kit** and wiring diagrams for a successful setup.

## Step 3: Software Installation

### Create a New ESPHome Device
1. **Create a new ESPHome device:**  
   Use the Home Assistant ESPHome interface or command line tool to create a new device.  
   Make sure to use the [example YAML configuration](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/blob/main/example.yaml) as your starting point.  
   Copy essential sections like `api` and `ota` into your new configuration file. 

### Deploy and Boot
2. **Deploy the configured firmware to your ESP device:**  
   Once the firmware is deployed, power on your ESP device and ensure it’s ready to connect.

### Check the Log
3. **Monitor the ESPHome logs:**  
   Keep an eye on the device logs in the ESPHome dashboard or via the command line tool. You should see yellow log entries indicating successful reception of data packets.  
   If you only see a message like `[samsung_ac] ... update` every 30 seconds, there could be a connection issue that needs addressing.

### Identify Indoor Device Addresses
4. **Identify the indoor device addresses:**  
   Look for purple log messages showing discovered devices like `" indoor: 20.00.00, 20.00.01"` or `" indoor: 00, 01"`.  
   These addresses identify your indoor units. If you don’t see them, refer to the [Troubleshooting](Troubleshooting.md) section for assistance.

### Update Your YAML File
5. **Copy and update the discovered addresses:**  
   Once you’ve identified the indoor units, add their addresses to your ESPHome YAML configuration file.  
   Create sections for each indoor unit with meaningful names and customize the properties to fit your setup.

### Remove Unneeded Properties
6. **Clean up your configuration:**  
   Review your YAML configuration and remove unnecessary properties.  
   This will simplify your setup and ensure only relevant settings are included.

---

## Additional Resources
- [Compatibility](Compatibility.md): Check supported models and get more details on compatibility.
- [Blueprints](Blueprints-and-Integrations.md): Enhance your Home Assistant setup with additional Blueprints.
- [Troubleshooting](Troubleshooting.md): Get help with common issues or configuration challenges.
- [FAQ](Frequently-Asked-Questions-(FAQ).md): Check out common questions and answers.

By following these steps, you will be ready to use the **ESPHome Samsung HVAC Integration** effectively. 🚀
