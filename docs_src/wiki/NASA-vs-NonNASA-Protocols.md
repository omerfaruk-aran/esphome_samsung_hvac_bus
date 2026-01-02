Samsung utilizes two distinct protocols for communication between its HVAC systems: **NASA** and **NonNASA**. These protocols dictate how information is exchanged between indoor and outdoor units. Understanding these protocols can help with troubleshooting and configuring your setup correctly.

## NASA Protocol

The **NASA protocol** is the newer communication standard Samsung uses in its modern HVAC systems. It is designed to be more flexible and capable of transporting more data compared to the older NonNASA protocol. It uses keys and values for each piece of data, allowing a more structured and detailed way to exchange information between units.

For instance, to retrieve room temperature data, you would need to know the specific key and wait for it to be transmitted. This protocol allows more advanced communication between units and supports a wider range of data types such as enums, integers, long values, and byte arrays.

For more technical details, check out the documentation by [Foxhill67](https://wiki.myehs.eu/wiki/NASA_Protocol) which offers an in-depth breakdown of this protocol.

### Key Characteristics:
- **Advanced Key-Value Pairing:** Transports variables using a unique key and associated value.
- **Supports Multiple Data Types:** Allows for the use of Enums, Integers, Longs, and Bytes.
- **More Data Capacity:** Transfers larger and more detailed sets of data.

## NonNASA Protocol

The **NonNASA protocol** is the older standard that Samsung HVAC systems used. It is more basic and primarily designed to transport essential data between the air conditioner units. Although simpler, it still shares some structural characteristics with the NASA protocol, like the usage of start and end bytes for each message.

If you're looking for a more detailed explanation of the NonNASA protocol, take a look at the efforts made by [DannyDeGaspari](https://github.com/DannyDeGaspari/Samsung-HVAC-buscontrol), who has documented the protocol from the wall controller's perspective.

### Key Characteristics:
- **Simple Data Transport:** Designed for straightforward communication of basic air conditioner data.
- **Fewer Data Types Supported:** Primarily supports basic values like temperatures, modes, and error codes.
- **Limited Data Capacity:** Transfers fewer types of data compared to NASA.
