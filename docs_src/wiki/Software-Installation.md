This page provides a step-by-step guide for installing and configuring the software required to integrate your Samsung HVAC units with ESPHome and Home Assistant.

## Step 1: Create a New ESPHome Device

- Begin by creating a new ESPHome device using either your Home Assistant instance or the ESPHome command line tool.
- Utilize the configuration from our example YAML file as a starting template. Be sure to copy the essential `api` and `ota` sections into your configuration.

You can find a detailed example YAML file [here](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/blob/main/example.yaml).

## Step 2: Deploy and Boot

- Deploy the configured firmware to your ESP device. This can be done using ESPHome's web interface, command line, or Home Assistant.
- Once deployed, power on the ESP device and wait for it to initialize.

## Step 3: Monitor the Logs

- Open the log viewer for your ESPHome device and monitor the output.
- Look for yellow log messages indicating the reception of data packets. If you only see messages like `[samsung_ac] ... update` every 30 seconds or so, this means that no packets are being received, and there may be an issue with your wiring or configuration.

## Step 4: Identify Indoor Device Addresses

- Allow the system to run for a minute, and check for purple log messages indicating discovered indoor device addresses. These addresses will appear in the format `indoor: 20.00.00, 20.00.01` or `indoor: 00, 01`.
- Note these addresses, as they are required for the next configuration steps.

If you do not see these messages, please refer to the [Troubleshooting](Troubleshooting.md) page.

## Step 5: Update Your YAML Configuration

- Copy the address block containing the indoor device addresses from the log viewer.
- Create a section for each indoor unit in your ESPHome configuration YAML using these addresses.
- Assign meaningful names to each unit based on the rooms they control and customize the configuration properties as needed.

## Step 6: Remove Unneeded Properties

- Review your configuration to remove any unnecessary properties that are not relevant for your setup. This will help keep your configuration clean and maintainable.

## Final Check

After completing the configuration, re-deploy the updated YAML to your ESP device and perform a final log check to ensure everything is working as expected.

If you encounter any issues, visit the [Troubleshooting](Troubleshooting.md) page or refer to the [FAQ](Frequently-Asked-Questions-(FAQ).md) for further guidance.

---

By following these steps, you should be able to successfully install and configure the ESPHome Samsung HVAC component for your setup.

