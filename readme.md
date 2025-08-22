# ESPHome Samsung HVAC Integration

Welcome to the **ESPHome Samsung HVAC Integration** project! This project provides an [ESPHome](https://esphome.io/) component to integrate Samsung HVAC units (air conditioners or heat pumps) seamlessly into Home Assistant. Our goal is to empower users with comprehensive control and monitoring of their HVAC systems using modern, open-source tools.

## 🌟 Overview
This project focuses on Samsung HVAC units that communicate via a dedicated hardware bus (commonly F1/F2, sometimes called R1/R2). Samsung uses two main protocols for communication between indoor and outdoor units:
- **NASA Protocol**: Newer models utilize this more advanced protocol. [NASA protocol documentation](docs/samsung_nasa_protocol.md).
- **NonNASA Protocol**: Older units often rely on this simpler protocol.

The integration is designed to support both protocols, ensuring compatibility across a wide range of Samsung AC units.

## 📚 Key Features
This integration offers a variety of powerful features:
- **Multisplit Support** for controlling multiple indoor units.
- **Temperature and Humidity Monitoring** with detailed data.
- **Energy Monitoring Sensors** for tracking power consumption.
- **AC Mode and Target Temperature Control**.
- **Real-Time Error Code Monitoring and Notifications**.

For a detailed breakdown, see the [Features Overview](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Features-Overview) page.

## 🚀 Quick Start
1. **Check Compatibility**: Make sure your Samsung unit is compatible by reviewing the [Compatibility](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Compatibility) page.
2. **Install and Configure**: Follow the detailed instructions in our [Installation Guide](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Installation-Guide) to get started.

## 🔧 Troubleshooting & FAQ
Need help? Visit the [Troubleshooting](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Troubleshooting) page for common fixes or check out our [FAQ](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Frequently-Asked-Questions-(FAQ)) for frequently asked questions.

## 📖 Learn More
- **NASA vs NonNASA Protocols**: Discover the technical differences between the two protocols [here](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/NASA-vs-NonNASA-Protocols).
- **Development Notes**: Interested in extending the project? Visit the [Development](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Development) page for advanced configurations and debugging tips.

## 📢 Announcements & Community
For the latest updates and discussions, visit our [Discussions](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/discussions) section. We welcome contributions and feedback from the community! Special recognition goes to **Steve Wagner** ([@lanwin](https://github.com/lanwin)) for founding and shaping this project. 🚀

We are grateful to all contributors who have played a role in developing and enhancing this project. Their dedication and efforts have shaped what this project is today.

For a complete list of contributors and to learn more about their roles, visit our [Credits and Contributors](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/wiki/Credits-and-Contributors) page.

Your contributions are always appreciated! If you are interested in getting involved, please check out our [CONTRIBUTING.md](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/blob/main/CONTRIBUTING.md) for guidelines on how to get started.

Thank you for being part of the ESPHome Samsung HVAC community!
