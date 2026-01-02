This page details the compatibility of the ESPHome Samsung HVAC integration with various Samsung air conditioning units and heat pumps. The goal is to provide users with a clear understanding of which models and protocols are supported, and how to check if their units are compatible.

## Supported Devices

The ESPHome Samsung HVAC component supports Samsung units that communicate using dedicated communication wires, typically labeled F1/F2. The component is compatible with units using the **NASA protocol** and **NonNASA protocol**.

### Automatic Model Checker

To identify the protocol used by your Samsung AC unit (NASA, NonNASA, or Other), please use our [Samsung AC Model Checker](http://git.samsung-hvac.aran.net.tr/model-checker/).

[![Model Checker](https://github.com/user-attachments/assets/8b237bb5-ebbd-492a-921a-f46f6315fe5e)](http://git.samsung-hvac.aran.net.tr/model-checker/)

## Confirmed Working Models

### NASA Protocol

The following Samsung units have been confirmed to work with the NASA protocol:

- AJ080TXJ4KG
- AJ026TN1DKG
- AR24TXHZAWKNEU
- AR24TXFCAWKN
- AR09TXFCAWKNEU
- AC030KNZDCH/AA (CNH30ZDK)
- AE090RNYDEG
- AE090RXEDEG
- AE160JXYDEH/EU
- AR09TXFCAWKNEU
- AR07TXFCAWKNEU
- AR12TSFABWKNCV
- AR12TSFACWKX

### NonNASA Protocol

The following Samsung units have been confirmed to work with the NonNASA protocol:

- RJ100F5HXBA
- AJ050NCJ2EG

### Non-Compatible Models

The following model is known not to work with this integration due to a lack of communication wires:

- AR12BSEAMWKX

## Community Contributions

Users are encouraged to share their findings and success stories with models not listed above. For confirmed NASA and NonNASA models, refer to the [NASA Protocol Discussion](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/discussions/82) and [NonNASA Protocol Discussion](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/discussions/78). If you successfully set up a model that is not yet confirmed, please contribute to the discussions!
