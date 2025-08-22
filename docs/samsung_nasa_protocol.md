# Samsung NASA Protocol
* The original content *was* on [myehs.eu/wiki](https://wiki.myehs.eu/wiki/NASA_Protocol)
* That page was downloaded by the Internet Archive's Wayback machine and is  
  available, [link](https://web.archive.org/web/20240330070335/https://wiki.myehs.eu/wiki/NASA_Protocol), with a [CC BY-SA 4.0 license](https://web.archive.org/web/20240330071629/https://creativecommons.org/licenses/by-sa/4.0/).
* This page is a slightly cleaned up version of the Wayback Machine's version.
* The original file contains a note, "From Samsung EHS Wiki". That 
[wiki](https://wiki.myehs.eu/wiki/Main_Page) also uses the CC BY-SA 4.0
license. 
* See [Issue 28](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/issues/28) for more information.

From Samsung EHS Wiki

## General

The indoor and outdoor unit of the EHS heat pump communicate via a protocol called "NASA".

## Physical layer

The data is transmitted via a [RS-485](https://en.wikipedia.org/wiki/RS-485) link with 9600 [baud](https://en.wikipedia.org/wiki/Baud), even [parity](https://en.wikipedia.org/wiki/Parity%20bit) and one stop bit.  
Connection to the physical layer via the [F1/F2 connector](/web/20240330070335/https://wiki.myehs.eu/wiki/F1/F2_connector "F1/F2 connector").

## Protocol details

The indoor and outdoor unit both regularly transmit their current status and sensor readings via so called "packets". Each packet contains metadata, like the sender and receiver address as well as the actual payload data, which is structured into "messages". Every packet can contain one or more messages.

## Packet structure

| Byte # | Description | Value | Note |
| --- | --- | --- | --- |
| 0   | Packet Start | 0x32 |     |
| 1, 2 | Packet Size | 16bit | (int)data\[2\]; `size + 2 == data.size();` |
| 3   | Source Adress Class | Address Class Enum | Outdoor = 0x10<br />HTU = 0x11<br />Indoor = 0x20<br />ERV = 0x30<br />Diffuser = 0x35<br />MCU = 0x38<br />RMC = 0x40<br />WiredRemote = 0x50<br />PIM = 0x58<br />SIM = 0x59<br />Peak = 0x5A<br />PowerDivider = 0x5B<br />OnOffController = 0x60<br />WiFiKit = 0x62<br />CentralController = 0x65<br />DMS = 0x6A<br />JIGTester = 0x80<br />BroadcastSelfLayer = 0xB0<br />BroadcastControlLayer = 0xB1<br />BroadcastSetLayer = 0xB2<br />BoradcastCS = 0xB3<br />BroadcastControlAndSetLayer = 0xB3<br />BroadcastModuleLayer = 0xB4<br />BoradcastCSM = 0xB7<br />BroadcastLocalLayer = 0xB8<br />BroadcastCSML = 0xBF<br />Undefiend = 0xFF |
| 4   | Source Channel | 8bit |     |
| 5   | Source Address | 8bit |     |
| 6   | Destination Address Class | Address Class Enum | See source Address class |
| 7   | Destination Channel | 8bit |     |
| 8   | Destination Address | 8bit |     |
| 9   | Packet Information | `packetInformation = ((int)data[index] & 128) >> 7 == 1` |     |
| 9   | Protocol Version | `protocolVersion = (uint8_t)(((int)data[index] & 96) >> 5);` |     |
| 9   | Retry Count | `retryCount = (uint8_t)(((int)data[index] & 24) >> 3);` |     |
| 10  | Packet Type | `packetType = (PacketType)(((int)data[index + 1] & 240) >> 4);` | StandBy = 0 Normal = 1, Gathering = 2, Install = 3, Download = 4 |
| 10  | Data Type | `dataType = (DataType)((int)data[index + 1] & 15);` | Undefined = 0, Read = 1, Write = 2, Request = 3, Notification = 4, Response = 5, Ack = 6, Nack = 7 |
| 11  | Packet Number | 8bit | Increasing packet number |
| 12  | Capacity (Number of Messages) | 8bit |     |
| 13, 14 | Message Number | `messageNumber = (uint32_t)data[index] * 256U + (uint32_t)data[index + 1]; type = (MessageSetType)(((uint32_t)messageNumber & 1536) >> 9);` | `messageNumber` as seen in the S-NET NASA.ptc file; type: Enum = 0 (1 byte payload), Variable = 1 (2 bytes payload), LongVariable = 2 (4 bytes payload), Structure = 3 (all following bytes until the end of the packet, minus 3 end bytes; when a packet contains a structure, it does not contain any other messages. Therefore the capacity will be 1.) |
| 14 + 1, 14 + 2, … | Message Payload | (size as derived from the Message Number) |     |
| …   | Iterate over Capacity to retrieve all messages |     |     |
| \-3, -2 | CRC 16 | 16bit | (int)data\[data.size() - 2\]; |
| \-1 | Packet End | 0x34 |     |

Thank you to lanwin, who implemented [esphome\_samsung\_ac](https://github.com/lanwin/esphome_samsung_ac/tree/main) and was the first to understand the structure of the NASA Messages.

## Message Numbers

A complete overview of all NASA message numbers does not seem to be available. Some information can be found in file NASA.prc, which is part of the SNET Pro service software ([https://s3.amazonaws.com/samsung-files/Tech\_Files/SNET+Pro+and+SNET+Pro+2+Service+Software/Snet+Pro+v1.5.3.zip](https://s3.amazonaws.com/samsung-files/Tech_Files/SNET+Pro+and+SNET+Pro+2+Service+Software/Snet+Pro+v1.5.3.zip)). Additional information is available in file NasaConst.java, which is part of the WiFiKit\_Source.zip ([https://opensource.samsung.com/uploadList?menuItem=home\_appliances&classification1=airconditioners&classification2=control\_solutions](https://opensource.samsung.com/uploadList?menuItem=home_appliances&classification1=airconditioners&classification2=control_solutions)).

Below tables show the available information from these files in a concise layout:


### NASA Message Numbers
| MsgNr | Label (NASA.prc) | Label (NasaConst.java) | Description | Remarks |
| --- | --- | --- | --- | --- |
| 0x0000 |     | NASA\_IM\_MASTER\_NOTIFY |     |     |
| 0x0004 |     | NASA\_INSPECTION\_MODE |     |     |
| 0x0007 |     | NASA\_GATHER\_INFORMATION |     |     |
| 0x0008 |     | NASA\_GATHER\_INFORMATION\_COUNT |     |     |
| 0x000A |     | NASA\_ENABLEDOWNLOAD |     |     |
| 0x000D |     | NASA\_DETECTION\_TYPE |     |     |
| 0x000E |     | NASA\_PEAK\_LEVEL |     |     |
| 0x000F |     | NASA\_PEAK\_MODE |     |     |
| 0x0010 |     | NASA\_PEAK\_CONTROL\_PERIOD |     |     |
| 0x0011 |     | NASA\_POWER\_MANUFACTURE |     |     |
| 0x0012 |     | NASA\_POWER\_CHANNEL1\_TYPE |     |     |
| 0x0013 |     | NASA\_POWER\_CHANNEL2\_TYPE |     |     |
| 0x0014 |     | NASA\_POWER\_CHANNEL3\_TYPE |     |     |
| 0x0015 |     | NASA\_POWER\_CHANNEL4\_TYPE |     |     |
| 0x0016 |     | NASA\_POWER\_CHANNEL5\_TYPE |     |     |
| 0x0017 |     | NASA\_POWER\_CHANNEL6\_TYPE |     |     |
| 0x0018 |     | NASA\_POWER\_CHANNEL7\_TYPE |     |     |
| 0x0019 |     | NASA\_POWER\_CHANNEL8\_TYPE |     |     |
| 0x001A |     | NASA\_POWER\_CHANNEL1\_USED |     |     |
| 0x001B |     | NASA\_POWER\_CHANNEL2\_USED |     |     |
| 0x001C |     | NASA\_POWER\_CHANNEL3\_USED |     |     |
| 0x001D |     | NASA\_POWER\_CHANNEL4\_USED |     |     |
| 0x001E |     | NASA\_POWER\_CHANNEL5\_USED |     |     |
| 0x001F |     | NASA\_POWER\_CHANNEL6\_USED |     |     |
| 0x0020 |     | NASA\_POWER\_CHANNEL7\_USED |     |     |
| 0x0021 |     | NASA\_POWER\_CHANNEL8\_USED |     |     |
| 0x0023 |     | NASA\_STANDBY\_MODE |     |     |
| 0x0025 | ENUM\_AD\_MULTI\_TENANT\_NO |     | WiFi Kit Multi Tenant No. |     |
| 0x0202 | VAR\_AD\_ERROR\_CODE1 | NASA\_ERROR\_CODE1 | Error code |     |
| 0x0203 |     | NASA\_ERROR\_CODE2 |     |     |
| 0x0204 |     | NASA\_ERROR\_CODE3 |     |     |
| 0x0205 |     | NASA\_ERROR\_CODE4 |     |     |
| 0x0206 |     | NASA\_ERROR\_CODE5 |     |     |
| 0x0207 | VAR\_AD\_INSTALL\_NUMBER\_INDOOR | NASA\_OUTDOOR\_INDOORCOUNT | Number of indoor units connected |     |
| 0x0208 |     | NASA\_OUTDOOR\_ERVCOUNT |     |     |
| 0x0209 |     | NASA\_OUTDOOR\_EHSCOUNT |     |     |
| 0x0210 |     | NASA\_NET\_ADDRESS |     |     |
| 0x0211 | VAR\_AD\_INSTALL\_NUMBER\_MCU | NASA\_OUTDOOR\_MCUCOUNT | Number of connected MCUs |     |
| 0x0213 |     | NASA\_DEMAND\_SYNC\_TIME |     |     |
| 0x0214 |     | NASA\_PEAK\_TARGET\_DEMAND |     |     |
| 0x0217 |     | NASA\_PNP\_NET\_ADDRESS | PNP only |     |
| 0x0401 | LVAR\_AD\_ADDRESS\_MAIN | NASA\_CONFIRM\_ADDRESS |     |     |
| 0x0402 | LVAR\_AD\_ADDRESS\_RMC | NASA\_RMCADDRESS |     | LogicalAnd 0xFF |
| 0x0403 |     | NASA\_RANDOM\_ADDRESS |     |     |
| 0x0406 |     | NASA\_ALL\_POWER\_CONSUMPTION\_SET | Total instantaneous power consumption |     |
| 0x0407 |     | NASA\_ALL\_POWER\_CONSUMPTION\_CUMULATIVE | Total cumulative power consumption |     |
| 0x0408 | LVAR\_AD\_ADDRESS\_SETUP | NASA\_SETUP\_ADDRESS |     |     |
| 0x0409 | LVAR\_AD\_INSTALL\_LEVEL\_ALL | NASA\_ALL\_REMOTE\_LEVEL |     |     |
| 0x040A | LVAR\_AD\_INSTALL\_LEVEL\_OPERATION\_POWER | NASA\_LEVEL\_POWER |     |     |
| 0x040B | LVAR\_AD\_INSTALL\_LEVEL\_OPERATION\_MODE | NASA\_LEVEL\_OPMODE |     |     |
| 0x040C | LVAR\_AD\_INSTALL\_LEVEL\_FAN\_MODE | NASA\_LEVEL\_FANSPEED |     |     |
| 0x040D | LVAR\_AD\_INSTALL\_LEVEL\_FAN\_DIRECTION | NASA\_LEVEL\_AIRSWING |     |     |
| 0x040E | LVAR\_AD\_INSTALL\_LEVEL\_TEMP\_TARGET | NASA\_LEVEL\_SETTEMP |     |     |
| 0x040F | LVAR\_AD\_INSTALL\_LEVEL\_KEEP\_INDIVIDUAL\_CONTROL | NASA\_LEVEL\_KEEP\_ALTERNATIVE\_MODE |     |     |
| 0x0410 | LVAR\_AD\_INSTALL\_LEVEL\_OPERATION\_MODE\_ONLY | NASA\_LEVEL\_OPMODE\_LIMIT |     |     |
| 0x0411 | LVAR\_AD\_INSTALL\_LEVEL\_COOL\_MODE\_UPPER | NASA\_LEVEL\_COOL\_HIGH\_TEMP\_LIMIT |     |     |
| 0x0412 | LVAR\_AD\_INSTALL\_LEVEL\_COOL\_MODE\_LOWER | NASA\_LEVEL\_COOL\_LOW\_TEMP\_LIMIT |     |     |
| 0x0413 | LVAR\_AD\_INSTALL\_LEVEL\_HEAT\_MODE\_UPPER | NASA\_LEVEL\_HEAT\_HIGH\_TEMP\_LIMIT |     |     |
| 0x0414 | LVAR\_AD\_INSTALL\_LEVEL\_HEAT\_MODE\_LOWER | NASA\_LEVEL\_HEAT\_LOW\_TEMP\_LIMIT |     |     |
| 0x0415 | LVAR\_AD\_INSTALL\_LEVEL\_CONTACT\_CONTROL | NASA\_LEVEL\_OUT\_POINT\_INPUT |     |     |
| 0x0416 | LVAR\_AD\_INSTALL\_LEVEL\_KEY\_OPERATION\_INPUT | NASA\_LEVEL\_KEY\_INPUT |     |     |
| 0x0417 | LVAR\_AD\_?? | NASA\_PNP\_CONFIRM\_ADDRESS | PNP only |     |
| 0x0418 | LVAR\_AD\_?? | NASA\_PNP\_RANDOM\_ADDRESS | PNP only |     |
| 0x0419 | LVAR\_AD\_?? | NASA\_PNP\_SETUP\_ADDRESS | PNP only |     |
| 0x041B | LVAR\_AD\_?? |     |     |     |
| 0x041C |     | NASA\_POWER\_CHANNEL1\_ELECTRIC\_VALUE |     |     |
| 0x041D |     | NASA\_POWER\_CHANNEL2\_ELECTRIC\_VALUE |     |     |
| 0x041E |     | NASA\_POWER\_CHANNEL3\_ELECTRIC\_VALUE |     |     |
| 0x041F |     | NASA\_POWER\_CHANNEL4\_ELECTRIC\_VALUE |     |     |
| 0x0420 |     | NASA\_POWER\_CHANNEL5\_ELECTRIC\_VALUE |     |     |
| 0x0421 |     | NASA\_POWER\_CHANNEL6\_ELECTRIC\_VALUE |     |     |
| 0x0422 |     | NASA\_POWER\_CHANNEL7\_ELECTRIC\_VALUE |     |     |
| 0x0423 |     | NASA\_POWER\_CHANNEL8\_ELECTRIC\_VALUE |     |     |
| 0x0434 |     | NASA\_PEAK\_RATIO\_CURRENT |     |     |
| 0x0435 |     | NASA\_PEAK\_RATIO\_POTENTIAL |     |     |
| 0x0436 |     | NASA\_PEAK\_TOTAL\_POWER |     |     |
| 0x0437 |     | NASA\_PEAK\_CURRENT\_TARGET\_DEMAND |     |     |
| 0x0438 |     | NASA\_PEAK\_FORCAST\_DEMAND |     |     |
| 0x0439 |     | NASA\_PEAK\_TOP\_DEMAND |     |     |
| 0x043A |     | NASA\_PEAK\_TARGET\_POWER |     |     |
| 0x043B |     | NASA\_POWER\_CHANNEL1\_PULSEVALUE |     |     |
| 0x043C |     | NASA\_POWER\_CHANNEL2\_PULSEVALUE |     |     |
| 0x043D |     | NASA\_POWER\_CHANNEL3\_PULSEVALUE |     |     |
| 0x043E |     | NASA\_POWER\_CHANNEL4\_PULSEVALUE |     |     |
| 0x043F |     | NASA\_POWER\_CHANNEL5\_PULSEVALUE |     |     |
| 0x0440 |     | NASA\_POWER\_CHANNEL6\_PULSEVALUE |     |     |
| 0x0441 |     | NASA\_POWER\_CHANNEL7\_PULSEVALUE |     |     |
| 0x0442 |     | NASA\_POWER\_CHANNEL8\_PULSEVALUE |     |     |
| 0x0443 |     | NASA\_PEAK\_SYNC\_TIME |     |     |
| 0x0444 |     | NASA\_PEAK\_CURRENT\_DEMAND |     |     |
| 0x0445 |     | NASA\_PEAK\_REAL\_VALUE |     |     |
| 0x0448 | LVAR\_AD\_MCU\_PORT\_SETUP |     |     |     |
| 0x0600 | STR\_AD\_OPTION\_BASIC | NASA\_PRODUCT\_OPTION |     |     |
| 0x0601 | STR\_AD\_OPTION\_INSTALL | NASA\_INSTALL\_OPTION |     |     |
| 0x0602 | STR\_AD\_OPTION\_INSTALL\_2 | NASA\_INSTALLOPTION2 |     |     |
| 0x0603 | STR\_AD\_OPTION\_CYCLE | NASA\_CYCLEOPTION |     |     |
| 0x0604 |     | NASA\_PBAOPTION |     |     |
| 0x0605 | STR\_AD\_INFO\_EQUIP\_POSITION | NASA\_NAME |     |     |
| 0x0607 | STR\_AD\_ID\_SERIAL\_NUMBER | NASA\_SERIAL\_NO | OutdoorTableSerialNumber |     |
| 0x0608 | STR\_AD\_DBCODE\_MICOM\_MAIN | NASA\_MICOM\_CODE | OutdoorUnitMainDBCodeVersion | VariableAssign Identifier="dbCode" |
| 0x060C | STR\_AD\_DBCODE\_EEPROM | NASA\_EEPROM\_CODE | OutdoorTableEEPROMDBCodeVersion |     |
| 0x0613 |     | NASA\_SIMPIM\_SYNC\_DATETIME |     |     |
| 0x0619 |     | NASA\_SIMPIM\_PASSWORD |     |     |
| 0x061A | STR\_AD\_PRODUCT\_MODEL\_NAME | NASA\_PRODUCT\_MODEL\_NAME |     |     |
| 0x061C | STR\_AD\_PRODUCT\_MAC\_ADDRESS |     | WiFi Kit MAC Address |     |
| 0x061F | STR\_AD\_ID\_MODEL\_NAME |     | Model Name |     |
| 0x2000 |     | NASA\_IM\_MASTER |     |     |
| 0x2001 |     | NASA\_CHANGE\_POLAR |     |     |
| 0x2002 |     | NASA\_ADDRESSING\_ASSIGN\_CONFIRM\_ADDRESS |     |     |
| 0x2003 |     | NASA\_ADDRESSING |     |     |
| 0x2004 | ENUM\_NM\_? | NASA\_PNP |     |     |
| 0x2006 |     | NASA\_CHANGE\_CONTROL\_NETWORK\_STATUS |     |     |
| 0x2007 |     | NASA\_CHANGE\_SET\_NETWORK\_STATUS |     |     |
| 0x2008 |     | NASA\_CHANGE\_LOCAL\_NETWORK\_STATUS |     |     |
| 0x2009 |     | NASA\_CHANGE\_MODULE\_NETWORK\_STATUS |     |     |
| 0x200A |     | NASA\_CHANGE\_ALL\_NETWORK\_STATUS |     |     |
| 0x200F | ENUM\_NM\_NETWORK\_POSITINON\_LAYER | NASA\_LAYER | Enumeration Type |     |
| 0x2010 | ENUM\_NM\_NETWORK\_TRACKING\_STATE | NASA\_TRACKING\_RESULT |     |     |
| 0x2012 | ENUM\_NM\_? |     |     |     |
| 0x2017 |     | NASA\_COMMU\_MICOM\_LED |     |     |
| 0x2018 |     | NASA\_COMMU\_MICOM\_BUTTON |     |     |
| 0x22F7 | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22F9 | VAR\_NM\_?? |     |     |     |
| 0x22FA | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22FB | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22FC | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22FD | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22FE | VAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x22FF | VAR\_NM\_?? |     |     |     |
| 0x2400 | LVAR\_NM\_?? | NASA\_ALL\_LAYER\_DEVICE\_COUNT |     | seen in NASA data from EHS Mono HT Quiet |
| 0x2401 | LVAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x24FB | LVAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x24FC | LVAR\_NM\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4000 | ENUM\_IN\_OPERATION\_POWER | NASA\_POWER | Indoor unit power on/off | 0 Off, 1 On, 2 On |
| 0x4001 | ENUM\_IN\_OPERATION\_MODE | NASA\_INDOOR\_OPMODE | Indoor unit control mode | 0 Auto, 1 Cool, 2 Dry, 3 Fan, 4 Heat, 21 Cool Storage, 24 Hot water |
| 0x4002 | ENUM\_IN\_OPERATION\_MODE\_REAL | NASA\_INDOOR\_REAL\_OPMODE | Indoor unit current operation mode | 0 Auto, 1 Cool, 2 Dry, 3 Fan, 4 Heat, 11 Auto Cool, 12 Auto Dry, 13 Auto Fan, 14 Auto Heat, 21 Cool Storage, 24 Hot water, 255 NULL mode |
| 0x4003 | ENUM\_IN\_OPERATION\_VENT\_POWER | NASA\_ERV\_POWER | Ventilation operation mode |     |
| 0x4004 | ENUM\_IN\_OPERATION\_VENT\_MODE | NASA\_ERV\_OPMODE |     |     |
| 0x4006 | ENUM\_IN\_?? | NASA\_FANSPEED |     |     |
| 0x4007 | ENUM\_IN\_FAN\_MODE\_REAL |     | Indoor unit current air volume |     |
| 0x4008 | ENUM\_IN\_FAN\_VENT\_MODE | NASA\_ERV\_FANSPEED | Indoor unit current air volume |     |
| 0x400F | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4010 | ENUM\_IN\_?? |     |     |     |
| 0x4011 | ENUM\_IN\_LOUVER\_HL\_SWING | NASA\_AIRFLOW\_UPDOWN | Up and down wind direction setting/status |     |
| 0x4012 | ENUM\_IN\_LOUVER\_HL\_PART\_SWING |     | Up and down wind direction setting/status |     |
| 0x4015 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4018 | ENUM\_IN\_?? | NASA\_USE\_WIREDREMOTE |     |     |
| 0x4019 | ENUM\_IN\_?? | NASA\_USE\_DISCHARGE\_TEMP | This value is a value that cannot be controlled by the upper controller. |     |
| 0x401B | ENUM\_IN\_?? | NASA\_USE\_CENTUAL\_CONTROL | Income from InstallOption information. |     |
| 0x4023 | ENUM\_IN\_?? | NASA\_USE\_SPI |     |     |
| 0x4024 | ENUM\_IN\_?? | NASA\_USE\_FILTER\_WARNING\_TIME |     |     |
| 0x4025 |     | NASA\_FILTER\_CLEAN |     |     |
| 0x4027 | ENUM\_IN\_?? | NASA\_FILTER\_WARNING |     |     |
| 0x4028 | ENUM\_IN\_STATE\_THERMO |     | Thermo On/Off | 0 Off, 1 On |
| 0x4029 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x402A | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x402B | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x402D | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x402E | ENUM\_IN\_STATE\_DEFROST\_MODE | NASA\_INDOOR\_DEFROST\_STATUS | Defrost mode | 0 Off, 1 On |
| 0x402F | ENUM\_IN\_MTFC |     |     |     |
| 0x4031 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4035 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4038 | ENUM\_IN\_STATE\_HUMIDITY\_PERCENT | NASA\_HUMIDITY\_PERCENT |     |     |
| 0x403D |     | NASA\_CONTROL\_OAINTAKE |     |     |
| 0x403E |     | NASA\_USE\_MDS |     |     |
| 0x403F |     | NASA\_CONTROL\_MDS |     |     |
| 0x4040 |     | NASA\_USE\_HUMIDIFICATION |     |     |
| 0x4041 |     | NASA\_CONTROL\_HUMIDIFICATION |     |     |
| 0x4042 |     | NASA\_CONTROL\_AUTO\_CLEAN |     |     |
| 0x4043 | ENUM\_IN\_?? | NASA\_CONTROL\_SPI |     |     |
| 0x4045 |     | NASA\_USE\_SILENCE |     |     |
| 0x4046 | ENUM\_IN\_SILENCE | NASA\_CONTROL\_SILENCE | Silence mode | 0 Off, 1 On |
| 0x4047 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4048 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x404F | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4050 |     | NASA\_CONTROL\_SILENCT |     |     |
| 0x4051 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4059 | ENUM\_IN\_?? |     |     |     |
| 0x405B |     | NASA\_USE\_OUTER\_COOL |     |     |
| 0x405C |     | NASA\_CONTROL\_OUTER\_COOL |     |     |
| 0x405D |     | NASA\_USE\_DESIRED\_HUMIDITY |     |     |
| 0x405E |     | NASA\_CONTROL\_DESIRED\_HUMIDITY |     |     |
| 0x405F | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4060 | ENUM\_IN\_ALTERNATIVE\_MODE | NASA\_ALTERNATIVE\_MODE |     | 0 Off, 9 On |
| 0x4063 |     | NASA\_EHS\_INDOOR\_POWER |     |     |
| 0x4064 |     | NASA\_EHS\_INDOOR\_OPMODE |     |     |
| 0x4065 | ENUM\_IN\_WATER\_HEATER\_POWER | NASA\_DHW\_POWER | Water heater power | 0 Off, 1 On |
| 0x4066 | ENUM\_IN\_WATER\_HEATER\_MODE | NASA\_DHW\_OPMODE | Water heater mode | 0 Eco, 1 Standard, 2 Power, 3 Force |
| 0x4067 | ENUM\_IN\_3WAY\_VALVE | NASA\_DHW\_VALVE | Hydro\_3Way | 0 Room, 1 Tank |
| 0x4068 | ENUM\_IN\_SOLAR\_PUMP | NASA\_SOLAR\_PUMP | Hydro\_SolarPump |     |
| 0x4069 | ENUM\_IN\_THERMOSTAT1 |     | Hydro\_ExternalThermostat | 0 Off, 1 Cool, 2 Heat |
| 0x406A | ENUM\_IN\_THERMOSTAT2 |     | Hydro\_ExternalThermostat2 | 0 Off, 1 Cool, 2 Heat |
| 0x406B | ENUM\_IN\_?? | NASA\_SMART\_GRID |     |     |
| 0x406C | ENUM\_IN\_BACKUP\_HEATER |     | Backup heater mode | 0 Off, 1 Step 1, 2 Step 2 |
| 0x406D | ENUM\_IN\_OUTING\_MODE | NASA\_INDOOR\_OUT\_GOING | Outing mode | 0 Off, 1 On |
| 0x406E | ENUM\_IN\_QUIET\_MODE |     |     |     |
| 0x406F | ENUM\_IN\_REFERENCE\_EHS\_TEMP | NASA\_DHW\_REFERENCE\_TEMP | Hydro\_ControlChoice\_RoomTemp | 0 Room, 1 Water out. Variable isEhsSetTempWaterOut |
| 0x4070 | ENUM\_IN\_DISCHAGE\_TEMP\_CONTROL | NASA\_DISCHARGE\_TEMP\_ENABLE |     | 0 Off, 1 On (rem: "DISCHAGE" is typo in NASA.ptc) |
| 0x4073 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4074 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4076 | ENUM\_IN\_ROOM\_TEMP\_SENSOR |     |     |     |
| 0x4077 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x407B | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x407D | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x407E | ENUM\_IN\_LOUVER\_LR\_SWING | NASA\_AIRFLOW\_LEFTRIGHT | Left and right wind direction settings/status | 0 Off, 1 On |
| 0x4085 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4086 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4087 | ENUM\_IN\_BOOSTER\_HEATER |     | Booster heater | 0 Off, 1 On |
| 0x4089 | ENUM\_IN\_STATE\_WATER\_PUMP |     | Water pump | 0 Off, 1 On |
| 0x408A | ENUM\_IN\_2WAY\_VALVE |     |     | 0 Off, 2 CV, 3 Boiler |
| 0x4093 | ENUM\_IN\_FSV\_2041 |     | FSV Water Law Type Heating | 1 Floor, 2 FCU |
| 0x4094 | ENUM\_IN\_FSV\_2081 |     | FSV Water Law Type Cooling | 1 Floor, 2 FCU |
| 0x4095 | ENUM\_IN\_FSV\_2091 | NASA\_USE\_THERMOSTAT1 |     | values 0="No" up to 4="4" |
| 0x4096 | ENUM\_IN\_FSV\_2092 | NASA\_USE\_THERMOSTAT2 |     | values 0="No" up to 4="4" |
| 0x4097 | ENUM\_IN\_FSV\_3011 | NASA\_ENABLE\_DHW |     | values 0="No" up to 2="2" |
| 0x4098 | ENUM\_IN\_FSV\_3031 | NASA\_USE\_BOOSTER\_HEATER |     | 0 Off, 1 On |
| 0x4099 | ENUM\_IN\_FSV\_3041 |     |     | 0 No, 1 Yes |
| 0x409A | ENUM\_IN\_FSV\_3042 |     |     | Sunday=0, Monday=1 .. up to 7=Everyday |
| 0x409B | ENUM\_IN\_FSV\_3051 |     |     | 0 No, 1 Yes |
| 0x409C | ENUM\_IN\_FSV\_3061 | NASA\_USE\_DHW\_THERMOSTAT |     |     |
| 0x409D | ENUM\_IN\_FSV\_3071 |     |     |     |
| 0x409E | ENUM\_IN\_FSV\_4011 |     |     |     |
| 0x409F | ENUM\_IN\_FSV\_4021 |     |     |     |
| 0x40A0 | ENUM\_IN\_FSV\_4022 |     |     |     |
| 0x40A1 | ENUM\_IN\_FSV\_4023 |     |     |     |
| 0x40A2 | ENUM\_IN\_FSV\_4031 |     |     |     |
| 0x40A3 | ENUM\_IN\_FSV\_4032 |     |     |     |
| 0x40A4 | ENUM\_IN\_FSV\_5041 |     |     |     |
| 0x40A5 | ENUM\_IN\_FSV\_5042 |     |     |     |
| 0x40A6 | ENUM\_IN\_FSV\_5043 |     |     |     |
| 0x40A7 | ENUM\_IN\_FSV\_5051 |     |     |     |
| 0x40B1 |     | NASA\_DHW\_OPMODE\_SUPPORT |     |     |
| 0x40B4 | ENUM\_IN\_FSV\_5061 |     |     |     |
| 0x40B5 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x40BB | ENUM\_IN\_STATE\_AUTO\_STATIC\_PRESSURE\_RUNNING |     |     |     |
| 0x40BC | ENUM\_IN\_STATE\_KEY\_TAG | NASA\_VACANCY\_STATUS | Vacancy control |     |
| 0x40BD | ENUM\_IN\_EMPTY\_ROOM\_CONTROL\_USED | NASA\_USE\_VACANCY\_STATUS |     |     |
| 0x40C0 | ENUM\_IN\_FSV\_4041 |     |     |     |
| 0x40C1 | ENUM\_IN\_FSV\_4044 |     |     |     |
| 0x40C2 | ENUM\_IN\_FSV\_4051 |     |     |     |
| 0x40C3 | ENUM\_IN\_FSV\_4053 |     |     |     |
| 0x40C4 | ENUM\_IN\_WATERPUMP\_PWM\_VALUE |     | Water pump speed | unit % |
| 0x40C5 | ENUM\_IN\_THERMOSTAT\_WATER\_HEATER |     | Hydro\_WaterHeaterThermostat |     |
| 0x40C6 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x40C7 |     | NASA\_AHUPANEL\_ENTHALPY\_CONTROL |     |     |
| 0x40C8 |     | NASA\_AHUPANEL\_DUTY\_CONTROL |     |     |
| 0x40C9 |     | NASA\_AHUPANEL\_SUMMERNIGHT\_CONTROL |     |     |
| 0x40CA |     | NASA\_AHUPANEL\_CO2\_CONTROL |     |     |
| 0x40CB |     | NASA\_AHUPANEL\_ENERGYMANAGE\_CONTROL |     |     |
| 0x40CC |     | NASA\_AHUPANEL\_RA\_SMOKE\_DECTION\_STATUS |     |     |
| 0x40CD |     | NASA\_AHUPANEL\_SA\_FAN\_STATUS |     |     |
| 0x40CE |     | NASA\_AHUPANEL\_RA\_FAN\_ONOFF\_STATUS |     |     |
| 0x40CF |     | NASA\_AHUPANEL\_ERROR\_STATUS |     |     |
| 0x40D0 |     | NASA\_AHUPANEL\_HEATER\_ONOFF\_STATUS |     |     |
| 0x40D1 |     | NASA\_AHUPANEL\_SA\_FAN\_ONOFF\_STATUS |     |     |
| 0x40D2 |     | NASA\_AHUPANEL\_SMOKE\_DECTION\_CONTROL |     |     |
| 0x40D5 | ENUM\_IN\_ENTER\_ROOM\_CONTROL\_USED |     |     |     |
| 0x40D6 | ENUM\_IN\_ERROR\_HISTORY\_CLEAR\_FOR\_HASS |     |     |     |
| 0x40E3 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x40E7 | ENUM\_IN\_CHILLER\_WATERLAW\_SENSOR |     | DMV Chiller Option |     |
| 0x40F7 | ENUM\_IN\_CHILLER\_WATERLAW\_ON\_OFF |     |     |     |
| 0x40FB | ENUM\_IN\_CHILLLER\_SETTING\_SILENT\_LEVEL |     |     |     |
| 0x40FC | ENUM\_IN\_CHILLER\_SETTING\_DEMAND\_LEVEL |     |     |     |
| 0x4101 | ENUM\_IN\_CHILLER\_EXT\_WATER\_OUT\_INPUT |     |     |     |
| 0x4102 | ENUM\_IN\_STATE\_FLOW\_CHECK |     |     |     |
| 0x4103 | ENUM\_IN\_WATER\_VALVE\_1\_ON\_OFF |     | FCU Kit |     |
| 0x4104 | ENUM\_IN\_WATER\_VALVE\_2\_ON\_OFF |     |     |     |
| 0x4105 | ENUM\_IN\_ENTHALPY\_CONTROL\_STATE |     |     |     |
| 0x4107 | ENUM\_IN\_FSV\_5033 |     |     |     |
| 0x4108 | ENUM\_IN\_TDM\_INDOOR\_TYPE |     |     |     |
| 0x410D | ENUM\_IN\_FREE\_COOLING\_STATE |     |     |     |
| 0x4113 | ENUM\_IN\_3WAY\_VALVE\_2 |     |     |     |
| 0x4117 | ENUM\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4119 | ENUM\_IN\_OPERATION\_POWER\_ZONE1 |     |     |     |
| 0x411A | ENUM\_IN\_FSV\_4061 |     |     |     |
| 0x411B | ENUM\_IN\_FSV\_5081 |     |     |     |
| 0x411C | ENUM\_IN\_FSV\_5091 |     |     |     |
| 0x411D | ENUM\_IN\_FSV\_5094 |     |     |     |
| 0x411E | ENUM\_IN\_OPERATION\_POWER\_ZONE2 |     | Zone2 Normal Power | Min = 0 Max = 1 |
| 0x4123 | ENUM\_IN\_PV\_CONTACT\_STATE |     | PV Control |     |
| 0x4124 | ENUM\_IN\_SG\_READY\_MODE\_STATE |     | Smart Grid |     |
| 0x4125 | ENUM\_IN\_FSV\_LOAD\_SAVE |     |     | Min = 0 Max = 1, similar name as 0x412D in NASA.ptc |
| 0x4127 | ENUM\_IN\_FSV\_2093 |     |     | Min = 1 Max = 4 |
| 0x4128 | ENUM\_IN\_FSV\_5022 |     |     | Min = 0 Max = 1 |
| 0x412A | ENUM\_IN\_FSV\_2094 |     |     | values 0="No" up to 4="4" |
| 0x412D | ENUM\_IN\_FSV\_LOAD\_SAVE |     |     | Min = 0 Max = 1, similar name as 0x4125 in NASA.ptc |
| 0x4147 | ENUM\_IN\_GAS\_LEVEL |     |     |     |
| 0x4149 | ENUM\_IN\_DIFFUSER\_OPERATION\_POWER |     |     |     |
| 0x4201 | VAR\_IN\_TEMP\_TARGET\_F | NASA\_SET\_TEMP | Indoor unit set temperature | if isEhsSetTempWaterOut (406F) ==1 , use value of variable waterOutSetTemp = 4247 |
| 0x4202 | VAR\_IN\_?? |     |     |     |
| 0x4203 | VAR\_IN\_TEMP\_ROOM\_F | NASA\_CURRENT\_TEMP | Room Temperature |     |
| 0x4204 | VAR\_IN\_?? | NASA\_MODIFIED\_CURRENT\_TEMP | Temperature |     |
| 0x4205 | VAR\_IN\_TEMP\_EVA\_IN\_F | NASA\_EVA\_IN\_TEMP | Indoor Eva In Temperature |     |
| 0x4206 | VAR\_IN\_TEMP\_EVA\_OUT\_F | NASA\_EVA\_OUT\_TEMP | Indoor Eva Out Temperature |     |
| 0x4207 | VAR\_IN\_TEMP\_ELECTRIC\_HEATER\_F |     | Electric heater temperature value |     |
| 0x4208 |     | NASA\_EVA\_INHOLE\_TEMP |     |     |
| 0x4209 | VAR\_IN\_?? | NASA\_SET\_DISCHARGE |     |     |
| 0x420B | VAR\_IN\_TEMP\_DISCHARGE | NASA\_CURRENT\_DISCHARGE | Indoor Discharge Temp(Duct, AHU) |     |
| 0x420C | VAR\_IN\_?? | NASA\_INDOOR\_OUTER\_TEMP |     | same value as 0x8204 (sensor\_airout) ? |
| 0x4211 | VAR\_IN\_CAPACITY\_REQUEST | NASA\_INDOOR\_CAPACITY | Capacity |     |
| 0x4212 | VAR\_IN\_CAPACITY\_ABSOLUTE | NASA\_INDOOR\_ABSOLUTE\_CAPACITY |     |     |
| 0x4213 | VAR\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4217 | VAR\_IN\_EEV\_VALUE\_REAL\_1 | NASA\_INODDR\_CURRENT\_EEV1 | Current EEV development level |     |
| 0x4218 | VAR\_IN\_EEV\_VALUE\_REAL\_2 | NASA\_INDOOR\_CURRENT\_EEV2 | Current EEV2 development level |     |
| 0x4219 | VAR\_IN\_?? | NASA\_INDOOR\_CURRENT\_EEV3 |     |     |
| 0x421A |     | NASA\_INDOOR\_CURRENT\_EEV4 |     |     |
| 0x421B | VAR\_IN\_SENSOR\_CO2\_PPM |     | CO2 sensor detection ppm |     |
| 0x4220 |     | NASA\_INDOOR\_AIRCLEANFAN\_CURRENT\_RPM |     |     |
| 0x4229 | VAR\_IN\_MODEL\_INFORMATION | NASA\_INDOOR\_MODEL\_INFORMATION | Indoor unit model information |     |
| 0x422A | VAR\_IN\_TEMP\_DISCHARGE\_COOL\_TARGET\_F | NASA\_COOL\_SET\_DISCHARGE | User limitation - Water Cooling Temperature Max. |     |
| 0x422B | VAR\_IN\_TEMP\_DISCHARGE\_HEAT\_TARGET\_F | NASA\_HEAT\_SET\_DISCHARGE |     |     |
| 0x4235 | VAR\_IN\_TEMP\_WATER\_HEATER\_TARGET\_F | NASA\_INDOOR\_DHW\_SET\_TEMP | DHW target temperature |     |
| 0x4236 | VAR\_IN\_TEMP\_WATER\_IN\_F | NASA\_INDOOR\_WATER\_IN\_TEMP | Hydro\_WaterIn |     |
| 0x4237 | VAR\_IN\_TEMP\_WATER\_TANK\_F | NASA\_INDOOR\_DHW\_CURRENT\_TEMP | DHW tank current temperature |     |
| 0x4238 | VAR\_IN\_TEMP\_WATER\_OUT\_F | NASA\_INDOOR\_WATER\_OUT\_TEMP | Hydro\_WaterOut |     |
| 0x4239 | VAR\_IN\_TEMP\_WATER\_OUT2\_F |     | Hydro\_HeaterOut |     |
| 0x423E | VAR\_IN\_?? |     |     |     |
| 0x4247 | VAR\_IN\_TEMP\_WATER\_OUTLET\_TARGET\_F | NASA\_INDOOR\_SETTEMP\_WATEROUT | Hydro\_WaterOutletTargetF | variable waterOutSetTemp |
| 0x4248 | VAR\_IN\_TEMP\_WATER\_LAW\_TARGET\_F |     |     |     |
| 0x424A | VAR\_IN\_FSV\_1011 | NASA\_INDOOR\_COOL\_MAX\_SETTEMP\_WATEROUT | User limitation - Water Cooling Temperature Max. |     |
| 0x424B | VAR\_IN\_FSV\_1012 | NASA\_INDOOR\_COOL\_MIN\_SETTEMP\_WATEROUT |     |     |
| 0x424C | VAR\_IN\_FSV\_1021 | NASA\_INDOOR\_COOL\_MAX\_SETTEMP\_ROOM | User limitation - Room Cooling Temperature Max. |     |
| 0x424D | VAR\_IN\_FSV\_1022 | NASA\_INDOOR\_COOL\_MIN\_SETTEMP\_ROOM |     |     |
| 0x424E | VAR\_IN\_FSV\_1031 | NASA\_INDOOR\_HEAT\_MAX\_SETTEMP\_WATEROUT | User limitation - Water Heating Temperature Max. |     |
| 0x424F | VAR\_IN\_FSV\_1032 | NASA\_INDOOR\_HEAT\_MIN\_SETTEMP\_WATEROUT |     |     |
| 0x4250 | VAR\_IN\_FSV\_1041 | NASA\_INDOOR\_HEAT\_MAX\_SETTEMP\_ROOM | User limitation - Room heating Temperature Max. |     |
| 0x4251 | VAR\_IN\_FSV\_1042 | NASA\_INDOOR\_HEAT\_MIN\_SETTEMP\_ROOM |     |     |
| 0x4252 | VAR\_IN\_FSV\_1051 | NASA\_DHW\_MAX\_SETTEMPLIMIT | User limitation - Hot Water Temperature Max. |     |
| 0x4253 | VAR\_IN\_FSV\_1052 | NASA\_DHW\_MIN\_SETTEMPLIMIT |     |     |
| 0x4254 | VAR\_IN\_FSV\_2011 |     | Water Law Auto heating ambient temperature - Max. |     |
| 0x4255 | VAR\_IN\_FSV\_2012 |     |     |     |
| 0x4256 | VAR\_IN\_FSV\_2021 |     | Water Law (WL1-Floor) Temperature auto heating - Max. |     |
| 0x4257 | VAR\_IN\_FSV\_2022 |     |     |     |
| 0x4258 | VAR\_IN\_FSV\_2031 |     | Water Law (WL2-FCU) Temperature auto heating - Max. |     |
| 0x4259 | VAR\_IN\_FSV\_2032 |     |     |     |
| 0x425A | VAR\_IN\_FSV\_2051 |     |     |     |
| 0x425B | VAR\_IN\_FSV\_2052 |     |     |     |
| 0x425C | VAR\_IN\_FSV\_2061 |     |     |     |
| 0x425D | VAR\_IN\_FSV\_2062 |     |     |     |
| 0x425E | VAR\_IN\_FSV\_2071 |     |     |     |
| 0x425F | VAR\_IN\_FSV\_2072 |     |     |     |
| 0x4260 | VAR\_IN\_FSV\_3021 |     | DHW Heating mode - Max. |     |
| 0x4261 | VAR\_IN\_FSV\_3022 |     |     |     |
| 0x4262 | VAR\_IN\_FSV\_3023 |     | DHW Heating mode - Start |     |
| 0x4263 | VAR\_IN\_FSV\_3024 |     |     |     |
| 0x4264 | VAR\_IN\_FSV\_3025 |     | DHW Heating mode - DHW operation time |     |
| 0x4265 | VAR\_IN\_FSV\_3026 |     |     |     |
| 0x4266 | VAR\_IN\_FSV\_3032 |     | DHW Booster heater - Delayed time |     |
| 0x4267 | VAR\_IN\_FSV\_3033 |     |     |     |
| 0x4268 | VAR\_IN\_FSV\_3034 |     |     | not for EHS Mono HT Quiet |
| 0x4269 | VAR\_IN\_FSV\_3043 |     |     |     |
| 0x426A | VAR\_IN\_FSV\_3044 |     | Desinfection - Target temp. |     |
| 0x426B | VAR\_IN\_FSV\_3045 |     |     |     |
| 0x426C | VAR\_IN\_FSV\_3052 |     |     |     |
| 0x426D | VAR\_IN\_FSV\_4012 |     |     |     |
| 0x426E | VAR\_IN\_FSV\_4013 |     | Heating mode - Heating Off |     |
| 0x426F | VAR\_IN\_FSV\_4014 |     |     |     |
| 0x4270 | VAR\_IN\_FSV\_4024 |     |     |     |
| 0x4271 | VAR\_IN\_FSV\_4025 |     |     |     |
| 0x4272 | VAR\_IN\_FSV\_4033 |     |     |     |
| 0x4273 | VAR\_IN\_FSV\_5011 |     |     |     |
| 0x4274 | VAR\_IN\_FSV\_5012 |     | Outing mode - Room Temperature of cooling Mode |     |
| 0x4275 | VAR\_IN\_FSV\_5013 |     |     |     |
| 0x4276 | VAR\_IN\_FSV\_5014 |     | Outing mode- Indoor heating temperature |     |
| 0x4277 | VAR\_IN\_FSV\_5015 |     |     |     |
| 0x4278 | VAR\_IN\_FSV\_5016 |     |     |     |
| 0x4279 | VAR\_IN\_FSV\_5017 |     |     |     |
| 0x427A | VAR\_IN\_FSV\_5018 |     | Outing mode - Temperature of auto heating WL2 water |     |
| 0x427B | VAR\_IN\_FSV\_5019 |     |     |     |
| 0x427C | VAR\_IN\_FSV\_5021 |     | Economic DHW mode - Temperature of hot water Tank |     |
| 0x427D | VAR\_IN\_FSV\_5031 |     |     |     |
| 0x427E | VAR\_IN\_FSV\_5032 |     |     |     |
| 0x427F | VAR\_IN\_TEMP\_WATER\_LAW\_F |     | Hydro\_WaterLawTargetF |     |
| 0x4284 | VAR\_IN\_?? | NASA\_INDOOR\_POWER\_CONSUMPTION | Indoor unit power consumption |     |
| 0x4286 | VAR\_IN\_FSV\_4042 |     |     |     |
| 0x4287 | VAR\_IN\_FSV\_4043 |     |     |     |
| 0x4288 | VAR\_IN\_FSV\_4045 |     |     |     |
| 0x4289 | VAR\_IN\_FSV\_4046 |     |     |     |
| 0x428A | VAR\_IN\_FSV\_4052 |     |     |     |
| 0x428C | VAR\_IN\_TEMP\_MIXING\_VALVE\_F |     | Hydro\_MixingValve |     |
| 0x428D | VAR\_IN\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x4290 | VAR\_IN\_?? | NASA\_AHUPANEL\_TARGET\_HUMIDITY |     |     |
| 0x4291 |     | NASA\_AHUPANEL\_OA\_DAMPER\_TARGET\_RATE |     |     |
| 0x4292 | VAR\_IN\_?? | NASA\_AHUPANEL\_RA\_TEMP |     |     |
| 0x4293 |     | NASA\_AHUPANEL\_RA\_HUMIDITY |     |     |
| 0x4294 | VAR\_IN\_?? | NASA\_AHUPANEL\_EA\_RATE |     |     |
| 0x4295 |     | NASA\_AHUPANEL\_OA\_TEMP |     |     |
| 0x4296 | VAR\_IN\_?? | NASA\_AHUPANEL\_OA\_HUMIDITY |     |     |
| 0x4297 | VAR\_AHU\_PANEL\_SA\_TEMP | NASA\_AHUPANEL\_SA\_TEMP |     |     |
| 0x4298 | VAR\_AHU\_PANEL\_SA\_HUMIDITY | NASA\_AHUPANEL\_SA\_HUMIDITY |     |     |
| 0x4299 |     | NASA\_AHUPANEL\_STATIC\_PRESSURE |     |     |
| 0x429A | VAR\_IN\_?? | NASA\_AHUPANEL\_MIXING\_TEMP |     |     |
| 0x429B |     | NASA\_AHUPANEL\_MIXING\_RATE |     |     |
| 0x429C | VAR\_IN\_?? | NASA\_AHUPANEL\_POINT\_STATUS |     |     |
| 0x429F | VAR\_IN\_FAN\_CURRENT\_RPM\_SUCTION1 |     |     |     |
| 0x42A1 | VAR\_IN\_FAN\_CURRENT\_RPM\_SUCTION2 |     |     |     |
| 0x42A3 | VAR\_IN\_FAN\_CURRENT\_RPM\_SUCTION3 |     |     |     |
| 0x42A5 | VAR\_IN\_TEMP\_PANEL\_AIR\_COOL1\_F |     |     |     |
| 0x42A6 | VAR\_IN\_TEMP\_PANEL\_AIR\_COOL2\_F |     |     |     |
| 0x42A7 | VAR\_IN\_TEMP\_PANEL\_ROOM\_COOL1\_F |     |     |     |
| 0x42A8 | VAR\_IN\_TEMP\_PANEL\_ROOM\_COOL2\_F |     |     |     |
| 0x42A9 | VAR\_IN\_TEMP\_PANEL\_TARGET\_COOL1\_F |     |     |     |
| 0x42AA | VAR\_IN\_TEMP\_PANEL\_TARGET\_COOL2\_F |     |     |     |
| 0x42AB | VAR\_IN\_TEMP\_PANEL\_AIR\_HEAT1\_F |     |     |     |
| 0x42AC | VAR\_IN\_TEMP\_PANEL\_AIR\_HEAT2\_F |     |     |     |
| 0x42AD | VAR\_IN\_TEMP\_PANEL\_ROOM\_HEAT1\_F |     |     |     |
| 0x42AE | VAR\_IN\_TEMP\_PANEL\_ROOM\_HEAT2\_F |     |     |     |
| 0x42AF | VAR\_IN\_TEMP\_PANEL\_TARGET\_HEAT1\_F |     |     |     |
| 0x42B0 | VAR\_IN\_TEMP\_PANEL\_TARGET\_HEAT2\_F |     |     |     |
| 0x42B1 | VAR\_IN\_MCC\_GROUP\_MODULE\_ADDRESS |     |     |     |
| 0x42B2 | VAR\_IN\_MCC\_GROUP\_MAIN |     |     |     |
| 0x42B3 | VAR\_IN\_MCC\_MODULE\_MAIN |     |     |     |
| 0x42C2 | VAR\_IN\_TEMP\_EVA2\_IN\_F |     | Indoor Eva2 In temperature |     |
| 0x42C3 | VAR\_IN\_TEMP\_EVA2\_OUT\_F |     | Indoor Eva2 Out Temperature |     |
| 0x42C4 | VAR\_IN\_CHILLER\_PHE\_IN\_P |     | Inlet pressure |     |
| 0x42C5 | VAR\_IN\_CHILLER\_PHE\_OUT\_P |     | Outlet pressure |     |
| 0x42C9 | VAR\_IN\_CHILLER\_EXTERNAL\_TEMPERATURE |     | External sensor-Room temperature |     |
| 0x42CA | VAR\_IN\_MODULATING\_VALVE\_1 |     |     |     |
| 0x42CB | VAR\_IN\_MODULATING\_VALVE\_2 |     |     |     |
| 0x42CC | VAR\_IN\_MODULATING\_FAN |     |     |     |
| 0x42CD | VAR\_IN\_TEMP\_WATER\_IN2\_F |     |     |     |
| 0x42CE | VAR\_IN\_FSV\_3046 |     | DHW Desinfection - Max. operation time | NASA Value is \[minutes\], not \[hours\] |
| 0x42CF | VAR\_IN\_ENTHALPY\_SENSOR\_OUTPUT |     |     |     |
| 0x42D0 | VAR\_IN\_EXT\_VARIABLE\_DAMPER\_OUTPUT |     |     |     |
| 0x42D1 | VAR\_IN\_DUST\_SENSOR\_PM10\_0\_VALUE |     |     |     |
| 0x42D2 | VAR\_IN\_DUST\_SENSOR\_PM2\_5\_VALUE |     |     |     |
| 0x42D3 | VAR\_IN\_DUST\_SENSOR\_PM1\_0\_VALUE |     |     |     |
| 0x42D4 | VAR\_IN\_TEMP\_ZONE2\_F |     | Idiom\_RoomTemp\_Zone2 |     |
| 0x42D6 | VAR\_IN\_TEMP\_TARGET\_ZONE2\_F |     | Zone2 Room Set Temp. |     |
| 0x42D7 | VAR\_IN\_TEMP\_WATER\_OUTLET\_TARGET\_ZONE2\_F |     | Water Outlet2 Set Temp. |     |
| 0x42D8 | VAR\_IN\_TEMP\_WATER\_OUTLET\_ZONE1\_F |     | Zone1 WaterOut Temp |     |
| 0x42D9 | VAR\_IN\_TEMP\_WATER\_OUTLET\_ZONE2\_F |     | Zone2 WaterOut Temp |     |
| 0x42DB | VAR\_IN\_FSV\_5082 |     |     |     |
| 0x42DC | VAR\_IN\_FSV\_5083 |     |     |     |
| 0x42DD | VAR\_IN\_FSV\_5092 |     |     |     |
| 0x42DE | VAR\_IN\_FSV\_5093 |     |     |     |
| 0x42E8 | VAR\_IN\_FLOW\_SENSOR\_VOLTAGE |     |     |     |
| 0x42E9 | VAR\_IN\_FLOW\_SENSOR\_CALC |     | Flow Sensor | value appears about every 90 seconds |
| 0x42ED | VAR\_IN\_FSV\_3081 |     |     |     |
| 0x42EE | VAR\_IN\_FSV\_3082 |     |     |     |
| 0x42EF | VAR\_IN\_FSV\_3083 |     |     |     |
| 0x42F0 | VAR\_IN\_FSV\_5023 |     |     |     |
| 0x42F1 | VAR\_OUT\_COMP\_FREQ\_RATE\_CONTROL |     |     | undocumented, taken from Pyton code |
| 0x4301 | VAR\_IN\_?? |     |     |     |
| 0x4302 | VAR\_IN\_CAPACITY\_VENTILATION\_REQUEST |     |     |     |
| 0x4401 | LVAR\_IN\_?? |     |     |     |
| 0x4405 |     | NASA\_GROUPCONTROL\_BIT1 |     |     |
| 0x4406 |     | NASA\_GROUPCONTROL\_BIT2 |     |     |
| 0x4407 |     | NASA\_GROUPCONTROL\_BIT3 |     |     |
| 0x440A | LVAR\_IN\_DEVICE\_STAUS\_HEATPUMP\_BOILER |     | Switch\_HyrdoFlow |     |
| 0x440E | LVAR\_IN\_?? |     |     |     |
| 0x440F | LVAR\_IN\_?? | NASA\_ERROR\_INOUT |     |     |
| 0x4415 | LVAR\_IN\_AUTO\_STATIC\_PRESSURE |     |     |     |
| 0x4418 | LVAR\_IN\_EMPTY\_ROOM\_CONTROL\_DATA | NASA\_VACANCY\_SETTING |     |     |
| 0x441B | LVAR\_IN\_ENTER\_ROOM\_CONTROL\_DATA |     |     |     |
| 0x441F | LVAR\_IN\_ETO\_COOL\_CONTROL\_DATA |     |     |     |
| 0x4420 | LVAR\_IN\_ETO\_HEAT\_CONTROL\_DATA |     |     |     |
| 0x4423 | LVAR\_IN\_?? |     | Minutes since installation | seen in NASA data from EHS Mono HT Quiet |
| 0x4424 | LVAR\_IN\_?? |     | Minutes active | seen in NASA data from EHS Mono HT Quiet |
| 0x4426 | LVAR\_IN\_?? |     | Generated power last minute |     |
| 0x4427 | LVAR\_IN\_?? |     | Total generated power |     |
| 0x4604 | STR\_IN\_INSTALL\_INDOOR\_SETUP\_INFO | NASA\_INDOOR\_ABLE\_FUNCTION |     |     |
| 0x4608 |     | NASA\_INDOOR\_SETTING\_MIN\_MAX\_TEMP |     |     |
| 0x4619 |     | NASA\_EHS\_SETTING\_MIN\_MAX\_TEMP |     |     |
| 0x461A |     | NASA\_EHS\_FSV\_SETTING\_MIN\_MAX\_TEMP |     |     |
| 0x461C |     | NASA\_AHUPANEL\_AHUKIT\_ADDRESS |     |     |
| 0x461D |     | NASA\_AHUPANEL\_PANEL\_OPTION |     |     |
| 0x461E | STR\_IN\_ERROR\_HISTORY\_FOR\_HASS |     | Structure Type |     |
| 0x8000 | ENUM\_OUT\_OPERATION\_SERVICE\_OP |     | Indoor unit defrost operation steps | 2 Heating test run, 3 Pump out, 13 Cooling test run, 14 Pump down |
| 0x8001 | ENUM\_OUT\_OPERATION\_ODU\_MODE | NASA\_OUTDOOR\_OPERATION\_STATUS | Outdoor Driving Mode | 0 OP\_STOP, 1 OP\_SAFETY, 2 OP\_NORMAL, 3 OP\_BALANCE, 4 OP\_RECOVERY, 5 OP\_DEICE, 6 OP\_COMPDOWN, 7 OP\_PROHIBIT, 8 OP\_LINEJIG, 9 OP\_PCBJIG, 10 OP\_TEST, 11 OP\_CHARGE, 12 OP\_PUMPDOWN, 13 OP\_PUMPOUT, 14 OP\_VACCUM, 15 OP\_CALORYJIG, 16 OP\_PUMPDOWNSTOP, 17 OP\_SUBSTOP, 18 OP\_CHECKPIPE, 19 OP\_CHECKREF, 20 OP\_FPTJIG, 21 OP\_NONSTOP\_HEAT\_COOL\_CHANGE, 22 OP\_AUTO\_INSPECT, 23 OP\_ELECTRIC\_DISCHARGE, 24 OP\_SPLIT\_DEICE, 25 OP\_INVETER\_CHECK, 26 OP\_NONSTOP\_DEICE, 27 OP\_REM\_TEST, 28 OP\_RATING, 29 OP\_PC\_TEST, 30 OP\_PUMPDOWN\_THERMOOFF, 31 OP\_3PHASE\_TEST, 32 OP\_SMARTINSTALL\_TEST, 33 OP\_DEICE\_PERFORMANCE\_TEST, 34 OP\_INVERTER\_FAN\_PBA\_CHECK, 35 OP\_AUTO\_PIPE\_PAIRING, 36 OP\_AUTO\_CHARGE |
| 0x8002 | ENUM\_OUT\_?? |     |     |     |
| 0x8003 | ENUM\_OUT\_OPERATION\_HEATCOOL | NASA\_OUTDOOR\_OPERATION\_MODE | Outdoor unit cooling/heating mode | 1 Cool, 2 Heat, 3 CoolMain, 4 HeatMain |
| 0x8005 | ENUM\_OUT\_?? |     |     |     |
| 0x800D | ENUM\_OUT\_?? |     |     |     |
| 0x8010 | ENUM\_OUT\_LOAD\_COMP1 | NASA\_OUTDOOR\_COMP1\_STATUS | Comp#1 On/Off |     |
| 0x8011 | ENUM\_OUT\_LOAD\_COMP2 | NASA\_OUTDOOR\_COMP2\_STATUS | Comp#2 On/Off |     |
| 0x8012 | ENUM\_OUT\_LOAD\_COMP3 | NASA\_OUTDOOR\_COMP3\_STATUS | Comp#3 On/Off |     |
| 0x8013 | ENUM\_OUT\_LOAD\_CCH1 | NASA\_OUTDOOR\_CCH1\_STATUS | CCH1 On/Off |     |
| 0x8014 | ENUM\_OUT\_LOAD\_CCH2 | NASA\_OUTDOOR\_CCH2\_STATUS | CCH2 On/Off |     |
| 0x8015 |     | NASA\_OUTDOOR\_CCH3\_STATUS |     |     |
| 0x8016 |     | NASA\_OUTDOOR\_ACCUMULATOR\_CCH |     |     |
| 0x8017 | ENUM\_OUT\_LOAD\_HOTGAS | NASA\_OUTDOOR\_HOTGAS1 | HotGas1 On/Off |     |
| 0x8018 | ENUM\_OUT\_LOAD\_HOTGAS2 | NASA\_OUTDOOR\_HOTGAS2 | HotGas2 On/Off |     |
| 0x8019 | ENUM\_OUT\_LOAD\_LIQUID | NASA\_OUTDOOR\_LIQUID\_BYPASS\_VALVE | Liquid On/Off |     |
| 0x801A | ENUM\_OUT\_LOAD\_4WAY | NASA\_OUTDOOR\_4WAY\_VALVE | 4Way On/Off |     |
| 0x801F | ENUM\_OUT\_LOAD\_MAINCOOL | NASA\_OUTDOOR\_MAIN\_COOL\_VALVE |     |     |
| 0x8020 | ENUM\_OUT\_LOAD\_OUTEEV | NASA\_OUTDOOR\_OD\_EEV\_VALVE |     |     |
| 0x8021 | ENUM\_OUT\_LOAD\_EVI\_BYPASS | NASA\_OUTDOOR\_EVI\_BYPASS\_VALVE | EVI ByPass On/Off |     |
| 0x8022 | ENUM\_OUT\_LOAD\_EVI\_SOL1 | NASA\_OUTDOOR\_EVI\_SOL1\_VALVE | EVI Sol1 On/Off |     |
| 0x8023 | ENUM\_OUT\_LOAD\_EVI\_SOL2 | NASA\_OUTDOOR\_EVI\_SOL2\_VALVE | EVI Sol2 On/Off |     |
| 0x8024 |     | NASA\_OUTDOOR\_EVI\_SOL3\_VALVE |     |     |
| 0x8025 | ENUM\_OUT\_LOAD\_GASCHARGE | NASA\_OUTDOOR\_GAS\_CHARGE | Hot Gas Charging |     |
| 0x8026 | ENUM\_OUT\_LOAD\_WATER | NASA\_OUTDOOR\_WATER\_VALVE | 2Way Valve |     |
| 0x8027 | ENUM\_OUT\_LOAD\_PUMPOUT | NASA\_OUTDOOR\_PUMPOUT\_VALVE | Pump Out |     |
| 0x802A | ENUM\_OUT\_LOAD\_4WAY2 | NASA\_OUTDOOR\_4WAY2\_VALVE |     |     |
| 0x8031 | ENUM\_OUT\_?? |     |     |     |
| 0x8032 | ENUM\_OUT\_?? |     |     |     |
| 0x8033 | ENUM\_OUT\_?? |     |     |     |
| 0x8034 | ENUM\_OUT\_LOAD\_LIQUIDTUBE | NASA\_OUTDOOR\_LIQUID\_TUBE\_VALVE | Liquid tube |     |
| 0x8037 | ENUM\_OUT\_LOAD\_ACCRETURN | NASA\_OUTDOOR\_ACCUM\_RETURN\_VALVE | ARV On/Off |     |
| 0x803B | ENUM\_OUT\_LOAD\_FLOW\_SWITCH | NASA\_OUTDOOR\_FLOW\_SWITCH | Flow Switch |     |
| 0x803C | ENUM\_OUT\_OPERATION\_AUTO\_INSPECT\_STEP |     | Automatic check step |     |
| 0x803F | ENUM\_OUT\_?? |     |     |     |
| 0x8043 | ENUM\_OUT\_?? |     |     |     |
| 0x8045 | ENUM\_OUT\_?? |     |     |     |
| 0x8046 | ENUM\_OUT\_OP\_TEST\_OP\_COMPLETE | NASA\_OUTDOOR\_TEST\_OP\_COMPLETE |     |     |
| 0x8047 | ENUM\_OUT\_?? | NASA\_OUTDOOR\_SERVICEOPERATION |     |     |
| 0x8048 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8049 | ENUM\_OUT\_MCU\_LOAD\_COOL\_A |     | MCU |     |
| 0x804A | ENUM\_OUT\_MCU\_LOAD\_HEAT\_A |     |     |     |
| 0x804B | ENUM\_OUT\_MCU\_LOAD\_COOL\_B |     |     |     |
| 0x804C | ENUM\_OUT\_MCU\_LOAD\_HEAT\_B |     |     |     |
| 0x804D | ENUM\_OUT\_MCU\_LOAD\_COOL\_C |     |     |     |
| 0x804E | ENUM\_OUT\_MCU\_LOAD\_HEAT\_C |     |     |     |
| 0x804F | ENUM\_OUT\_MCU\_LOAD\_COOL\_D |     |     |     |
| 0x8050 | ENUM\_OUT\_MCU\_LOAD\_HEAT\_D |     |     |     |
| 0x8051 | ENUM\_OUT\_MCU\_LOAD\_COOL\_E |     |     |     |
| 0x8052 | ENUM\_OUT\_MCU\_LOAD\_HEAT\_E |     |     |     |
| 0x8053 | ENUM\_OUT\_MCU\_LOAD\_COOL\_F |     |     |     |
| 0x8054 | ENUM\_OUT\_MCU\_LOAD\_HEAT\_F |     |     |     |
| 0x8055 | ENUM\_OUT\_MCU\_LOAD\_LIQUID |     |     |     |
| 0x8058 | ENUM\_OUT\_MCU\_PORT0\_INDOOR\_ADDR |     |     |     |
| 0x8059 | ENUM\_OUT\_MCU\_PORT1\_INDOOR\_ADDR |     |     |     |
| 0x805A | ENUM\_OUT\_MCU\_PORT2\_INDOOR\_ADDR |     |     |     |
| 0x805B | ENUM\_OUT\_MCU\_PORT3\_INDOOR\_ADDR |     |     |     |
| 0x805C | ENUM\_OUT\_MCU\_PORT4\_INDOOR\_ADDR |     |     |     |
| 0x805D | ENUM\_OUT\_MCU\_PORT5\_INDOOR\_ADDR |     |     |     |
| 0x805E | ENUM\_OUT\_?? |     |     |     |
| 0x8061 | ENUM\_OUT\_DEICE\_STEP\_INDOOR | NASA\_OUTDOOR\_INDOOR\_DEFROST\_STEP | Indoor unit defrost operation steps | 1 Defrost stage 1, 2 Defrost stage 2, 3 Defrost stage 3, 7 Defrost operation end stage, 255 No defrost operation |
| 0x8062 |     | NASA\_OUTDOOR\_LOGICAL\_DEFROST\_STEP |     |     |
| 0x8063 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8065 |     | NASA\_OUTDOOR\_SYSTEM\_RESET |     |     |
| 0x8066 | ENUM\_OUT\_?? | NASA\_OUTDOOR\_OPMODELIMIT |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8077 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8078 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8079 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807A | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807B | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807C | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807D | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807E | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x807F | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8081 | ENUM\_OUT\_?? | NASA\_OUTDOOR\_EXT\_CMD\_OPERATION |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8083 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x808C | ENUM\_OUT\_?? |     |     |     |
| 0x808D | ENUM\_OUT\_?? |     |     |     |
| 0x808E | ENUM\_OUT\_OP\_CHECK\_REF\_STEP |     | Refrigerant amount level | This is Enum in definition. But we need operation, so just consider this as variable. Min = 0, Max = 8 |
| 0x808F | ENUM\_OUT\_?? |     |     |     |
| 0x8092 | ENUM\_OUT\_INSTALL\_ODU\_COUNT |     |     |     |
| 0x8099 | ENUM\_OUT\_CONTROL\_FAN\_NUM |     | Number of outdoor fans |     |
| 0x809C | ENUM\_OUT\_CHECK\_REF\_RESULT |     | Refrigerant amount determination result |     |
| 0x809D | ENUM\_OUT\_?? | NASA\_OUTDOOR\_COOLONLY\_MODEL |     | seen in NASA data from EHS Mono HT Quiet |
| 0x809E | ENUM\_OUT\_LOAD\_CBOX\_COOLING\_FAN | NASA\_OUTDOOR\_CBOX\_COOLING\_FAN | DC Fan |     |
| 0x80A5 | ENUM\_OUT\_STATE\_BACKUP\_OPER | NASA\_OUTDOOR\_BACKUP\_OPERATION | Backup operation operation status On/Off |     |
| 0x80A6 | ENUM\_OUT\_STATE\_COMP\_PROTECT\_OPER | NASA\_OUTDOOR\_COM\_PROTECT\_OPERATIOIN | Compressor protection control operation status On/Off | 0 Off, 1 On |
| 0x80A7 | ENUM\_OUT\_?? | NASA\_OUTDOOR\_DRED\_LEVEL |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80A8 | ENUM\_OUT\_?? |     |     |     |
| 0x80A9 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80AA | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80AB | ENUM\_OUT\_?? |     |     |     |
| 0x80AC |     | NASA\_OUTDOOR\_ACCUM\_RETURN2\_VALVE |     |     |
| 0x80AE | ENUM\_OUT\_?? |     |     |     |
| 0x80AF | ENUM\_OUT\_LOAD\_BASEHEATER | NASA\_OUTDOOR\_BASE\_HEATER | Base heater On/Off state for EHS | 0 Off, 1 On |
| 0x80B1 | ENUM\_OUT\_?? |     |     |     |
| 0x80B2 | ENUM\_OUT\_?? | NASA\_OUTDOOR\_CH\_SWITCH\_VALUE |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80B4 | ENUM\_OUT\_STATE\_ACCUM\_VALVE\_ONOFF |     |     |     |
| 0x80B6 | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80B8 | ENUM\_OUT\_LOAD\_OIL\_BYPASS1 |     |     |     |
| 0x80B9 | ENUM\_OUT\_LOAD\_OIL\_BYPASS2 |     |     |     |
| 0x80BC | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80BE | ENUM\_OUT\_OP\_A2\_CURRENTMODE |     |     |     |
| 0x80C1 | ENUM\_OUT\_LOAD\_A2A\_VALVE |     |     |     |
| 0x80CE | ENUM\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x80D7 | ENUM\_OUT\_LOAD\_PHEHEATER |     |     |     |
| 0x80D8 | ENUM\_OUT\_EHS\_WATEROUT\_TYPE |     |     | 0 Default, 1 70°C |
| 0x8200 | VAR\_OUT\_?? | NASA\_OUTDOOR\_OPMODE\_OPTION |     |     |
| 0x8201 | VAR\_OUT\_?? |     |     |     |
| 0x8202 | VAR\_OUT\_INSTALL\_COMP\_NUM |     | Number of outdoor unit compressors |     |
| 0x8204 | VAR\_OUT\_SENSOR\_AIROUT | NASA\_OUTDOOR\_OUT\_TEMP | Outdoor temperature |     |
| 0x8206 | VAR\_OUT\_SENSOR\_HIGHPRESS | NASA\_OUTDOOR\_HIGH\_PRESS | High pressure |     |
| 0x8208 | VAR\_OUT\_SENSOR\_LOWPRESS | NASA\_OUTDOOR\_LOW\_PRESS | low pressure |     |
| 0x820A | VAR\_OUT\_SENSOR\_DISCHARGE1 | NASA\_OUTDOOR\_DISCHARGE\_TEMP1 | Discharge1 | The discharge temperature in a heat pump refers to the temperature of the refrigerant as it exits the compressor and enters the condenser. |
| 0x820C | VAR\_OUT\_SENSOR\_DISCHARGE2 | NASA\_OUTDOOR\_DISCHARGE\_TEMP2 | Discharge2 |     |
| 0x820E | VAR\_OUT\_SENSOR\_DISCHARGE3 | NASA\_OUTDOOR\_DISCHARGE\_TEMP3 | Discharge3 |     |
| 0x8210 |     | NASA\_OUTDOOR\_SUMPTEMP |     |     |
| 0x8217 | VAR\_OUT\_SENSOR\_CT1 | NASA\_OUTDOOR\_CT1 | Compressor 1 current |     |
| 0x8218 | VAR\_OUT\_SENSOR\_CONDOUT | NASA\_OUTDOOR\_COND\_OUT1 | Main heat exchanger outlet temperature |     |
| 0x821A | VAR\_OUT\_SENSOR\_SUCTION | NASA\_OUTDOOR\_SUCTION1\_TEMP | Suction temperature |     |
| 0x821C | VAR\_OUT\_SENSOR\_DOUBLETUBE | NASA\_OUTDOOR\_DOUBLE\_TUBE | Liquid pipe temperature |     |
| 0x821E | VAR\_OUTCD\_\_SENSOR\_EVIIN | NASA\_OUTDOOR\_EVI\_IN | EVI IN |     |
| 0x8220 | VAR\_OUT\_SENSOR\_EVIOUT | NASA\_OUTDOOR\_EVI\_OUT | EVI OUT |     |
| 0x8222 | VAR\_OUT\_?? | NASA\_OUTDOOR\_OLP\_TEMP |     |     |
| 0x8223 | VAR\_OUT\_CONTROL\_TARGET\_DISCHARGE | NASA\_OUTDOOR\_TARGET\_DISCHARGE | Target discharge temperature |     |
| 0x8224 | VAR\_OUT\_?? |     | Temperature | seen in NASA data from EHS Mono HT Quiet |
| 0x8225 | VAR\_OUT\_?? |     | Temperature | seen in NASA data from EHS Mono HT Quiet |
| 0x8226 | VAR\_OUT\_LOAD\_FANSTEP1 | NASA\_OUTDOOR\_FAN\_STEP1 | Outdoor Fan Step | Min 0, Max 10000 |
| 0x8227 | VAR\_OUT\_?? | NASA\_OUTDOOR\_FAN\_STEP2 |     |     |
| 0x8228 |     | NASA\_OUTDOOR\_LOADINGTIME |     |     |
| 0x8229 | VAR\_OUT\_LOAD\_OUTEEV1 | NASA\_OUTDOOR\_MAINEEV1 | Main EEV1 | An Electronic Expansion Valve, or EEV for short, is installed before the evaporator in an air handler/coil and after the condenser in a heat pump. It regulates the refrigerant flow rate to control superheat at the evaporator outlet by opening and closing. |
| 0x822A | VAR\_OUT\_LOAD\_OUTEEV2 | NASA\_OUTDOOR\_MAINEEV2 | Main EEV2 |     |
| 0x822B | VAR\_OUT\_LOAD\_OUTEEV3 | NASA\_OUTDOOR\_MAINEEV3 | Main EEV3 |     |
| 0x822C | VAR\_OUT\_LOAD\_OUTEEV4 | NASA\_OUTDOOR\_MAINEEV4 | Main EEV4 |     |
| 0x822D | VAR\_OUT\_LOAD\_OUTEEV5 | NASA\_OUTDOOR\_MAINEEV5 | Main EEV5 |     |
| 0x822E | VAR\_OUT\_LOAD\_EVIEEV | NASA\_OUTDOOR\_EVIEEV | EVI EEV |     |
| 0x822F |     | NASA\_OUTDOOR\_HREEV |     |     |
| 0x8230 | VAR\_OUT\_?? | NASA\_OUTDOOR\_RUNNING\_SUM\_CAPA |     |     |
| 0x8231 | VAR\_OUT\_?? | NASA\_OUTDOOR\_HEATING\_PERCENT |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8233 | VAR\_OUT\_?? | NASA\_OUTDOOR\_OPERATION\_CAPA\_SUM |     | division by 8.6 = kW |
| 0x8234 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8235 | VAR\_OUT\_ERROR\_CODE |     | HTU error code |     |
| 0x8236 | VAR\_OUT\_CONTROL\_ORDER\_CFREQ\_COMP1 | NASA\_OUTDOOR\_COMP1\_ORDER\_HZ | Instruction frequency 1 |     |
| 0x8237 | VAR\_OUT\_CONTROL\_TARGET\_CFREQ\_COMP1 | NASA\_OUTDOOR\_COMP1\_TARGET\_HZ | Target frequency 1 |     |
| 0x8238 | VAR\_OUT\_CONTROL\_CFREQ\_COMP1 | NASA\_OUTDOOR\_COMP1\_RUN\_HZ | Current frequency 1 |     |
| 0x8239 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x823B | VAR\_OUT\_SENSOR\_DCLINK\_VOLTAGE | NASA\_OUTDOOR\_DCLINK1\_VOLT | DC Link1 (Inverter DC voltage input) | Min 0, Max 1000 |
| 0x823C |     |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x823D | VAR\_OUT\_LOAD\_FANRPM1 | NASA\_OUTDOOR\_FAN\_RPM1 | Outdoor Fan1 RPM |     |
| 0x823E | VAR\_OUT\_LOAD\_FANRPM2 | NASA\_OUTDOOR\_FAN\_RPM2 | Outdoor Fan2 RPM |     |
| 0x823F | VAR\_OUT\_?? | NASA\_OUTDOOR\_CONTROL\_PRIME\_UNIT |     |     |
| 0x8240 |     | NASA\_OUTDOOR\_ODU\_CAPA1 | current electric capacity of outdoor unit | value in percent |
| 0x8241 |     | NASA\_OUTDOOR\_ODU\_CAPA2 |     |     |
| 0x8243 | VAR\_OUT\_?? |     |     |     |
| 0x8244 |     | NASA\_OUTDOOR\_OIL\_RECOVERY\_STEP |     |     |
| 0x8245 |     | NASA\_OUTDOOR\_OIL\_BALANCE\_STEP |     |     |
| 0x8247 | VAR\_OUT\_?? | NASA\_OUTDOOR\_DEFROST\_STEP |     |     |
| 0x8248 | VAR\_OUT\_?? | NASA\_OUTDOOR\_SAFETY\_START |     |     |
| 0x8249 | VAR\_OUT\_?? |     |     |     |
| 0x824B | VAR\_OUT\_?? |     |     |     |
| 0x824C | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x824F | VAR\_OUT\_CONTROL\_REFRIGERANTS\_VOLUME |     | Refrigerant amount |     |
| 0x8254 | VAR\_OUT\_SENSOR\_IPM1 | NASA\_OUTDOOR\_IPM\_TEMP1 | IPM1 Temperature | Min -41, Max 150. The IPM is a component within the inverter system. It is responsible for converting the incoming direct current (DC) power from the power supply into alternating current (AC) power that drives the compressor motor. The term "intelligent" is often used because the IPM includes sophisticated electronics and control algorithms that optimize the motor's performance. |
| 0x8255 | VAR\_OUT\_SENSOR\_IPM2 | NASA\_OUTDOOR\_IPM\_TEMP2 | IPM2 Temperature |     |
| 0x825A | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x825B | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x825C | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x825D | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x825E | VAR\_OUT\_SENSOR\_TEMP\_WATER | NASA\_OUTDOOR\_WATER\_TEMP | Water Temperature |     |
| 0x825F | VAR\_OUT\_SENSOR\_PIPEIN1 |     |     |     |
| 0x8260 | VAR\_OUT\_SENSOR\_PIPEIN2 |     |     |     |
| 0x8261 | VAR\_OUT\_SENSOR\_PIPEIN3 |     |     |     |
| 0x8262 | VAR\_OUT\_SENSOR\_PIPEIN4 |     |     |     |
| 0x8263 | VAR\_OUT\_SENSOR\_PIPEIN5 |     |     |     |
| 0x8264 | VAR\_OUT\_SENSOR\_PIPEOUT1 |     |     |     |
| 0x8265 | VAR\_OUT\_SENSOR\_PIPEOUT2 |     |     |     |
| 0x8266 | VAR\_OUT\_SENSOR\_PIPEOUT3 |     |     |     |
| 0x8267 | VAR\_OUT\_SENSOR\_PIPEOUT4 |     |     |     |
| 0x8268 | VAR\_OUT\_SENSOR\_PIPEOUT5 |     |     |     |
| 0x826B | VAR\_OUT\_MCU\_SENSOR\_SUBCOOLER\_IN |     |     |     |
| 0x826C | VAR\_OUT\_MCU\_SENSOR\_SUBCOOLER\_OUT |     |     |     |
| 0x826D | VAR\_OUT\_MCU\_SUBCOOLER\_EEV |     |     |     |
| 0x826E | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV1 |     |     |     |
| 0x826F | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV2 |     |     |     |
| 0x8270 | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV3 |     |     |     |
| 0x8271 | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV4 |     |     |     |
| 0x8272 | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV5 |     |     |     |
| 0x8273 | VAR\_OUT\_MCU\_CHANGE\_OVER\_EEV6 |     |     |     |
| 0x8274 | VAR\_OUT\_CONTROL\_ORDER\_CFREQ\_COMP2 | NASA\_OUTDOOR\_COMP2\_ORDER\_HZ | Instruction frequency 2 |     |
| 0x8275 | VAR\_OUT\_CONTROL\_TARGET\_CFREQ\_COMP2 | NASA\_OUTDOOR\_COMP2\_TARGET\_HZ | Target frequency 2 |     |
| 0x8276 | VAR\_OUT\_CONTROL\_CFREQ\_COMP2 | NASA\_OUTDOOR\_COMP2\_RUN\_HZ | Current frequency 2 |     |
| 0x8277 | VAR\_OUT\_SENSOR\_CT2 | NASA\_OUTDOOR\_CT2 | Compressor 2 current |     |
| 0x8278 | VAR\_OUT\_SENSOR\_OCT1 | NASA\_OUTDOOR\_OCT1 | Compressor OCT1 |     |
| 0x8279 |     | NASA\_OUTDOOR\_OCT2 |     |     |
| 0x827A | VAR\_OUT\_CONTROL\_DSH1 |     | Just for EHS HTU |     |
| 0x827E |     | NASA\_OUTDOOR\_ODU\_CAPA3 |     |     |
| 0x827F |     | NASA\_OUTDOOR\_ODU\_CAPA4 |     |     |
| 0x8280 | VAR\_OUT\_SENSOR\_TOP1 | NASA\_OUTDOOR\_TOP\_SENSOR\_TEMP1 | Top1 |     |
| 0x8281 | VAR\_OUT\_SENSOR\_TOP2 | NASA\_OUTDOOR\_TOP\_SENSOR\_TEMP2 | Top2 |     |
| 0x8282 |     | NASA\_OUTDOOR\_TOP\_SENSOR\_TEMP3 |     |     |
| 0x8287 | VAR\_OUT\_INSTALL\_CAPA | NASA\_OUTDOOR\_HP | Outdoor unit horsepower | unknown UNIT "HP" |
| 0x8298 |     | NASA\_OUTDOOR\_COOL\_SUM\_CAPA |     |     |
| 0x829A | VAR\_OUT\_SENSOR\_SUCTION2\_1SEC | NASA\_OUTDOOR\_SUCTION2\_TEMP |     |     |
| 0x829B |     | NASA\_OUTDOOR\_CT\_RESTRICT\_OPTION |     |     |
| 0x829C |     | NASA\_OUTDOOR\_COMPENSATE\_COOL\_CAPA |     |     |
| 0x829D |     | NASA\_OUTDOOR\_COMPENSATE\_HEAT\_CAPA |     |     |
| 0x829F | VAR\_OUT\_SENSOR\_SAT\_TEMP\_HIGH\_PRESSURE | NASA\_OUTDOOR\_HIGH\_PRESS\_TEMP | High pressure saturation temperature |     |
| 0x82A0 | VAR\_OUT\_SENSOR\_SAT\_TEMP\_LOW\_PRESSURE | NASA\_OUTDOOR\_LOW\_PRESS\_TEMP | Low pressure saturation temperature |     |
| 0x82A2 | VAR\_OUT\_?? |     |     |     |
| 0x82A3 |     | NASA\_OUTDOOR\_CT3 |     |     |
| 0x82A4 |     | NASA\_OUTDOOR\_OCT3 |     |     |
| 0x82A6 |     | NASA\_OUTDOOR\_FAN\_IPM1\_TEMP |     |     |
| 0x82A7 |     | NASA\_OUTDOOR\_FAN\_IPM2\_TEMP |     |     |
| 0x82A8 | VAR\_OUT\_CONTROL\_IDU\_TOTAL\_ABSCAPA |     |     |     |
| 0x82A9 | undefined |     |     | same value as 0x82A8 |
| 0x82AF | VAR\_OUT\_INSTALL\_COND\_SIZE |     |     |     |
| 0x82B2 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x82B3 |     | NASA\_OUTDOOR\_DCLINK2\_VOLT |     |     |
| 0x82B5 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x82B6 | VAR\_OUT\_?? |     |     |     |
| 0x82B8 | VAR\_OUT\_SENSOR\_MIDPRESS | NASA\_OUTDOOR\_MID\_PRESS | medium pressure |     |
| 0x82B9 |     | NASA\_OUTDOOR\_FAN\_CT1 |     |     |
| 0x82BA |     | NASA\_OUTDOOR\_FAN\_CT2 |     |     |
| 0x82BC | VAR\_OUT\_PROJECT\_CODE | NASA\_OUTDOOR\_PROJECT\_CODE | Project code |     |
| 0x82BD | VAR\_OUT\_LOAD\_FLUX\_VARIABLE\_VALVE | NASA\_OUTDOOR\_FLUX\_VARIABLE\_VALVE | Flow Control |     |
| 0x82BE | VAR\_OUT\_SENSOR\_CONTROL\_BOX | NASA\_OUTDOOR\_CBOX\_TEMP | Contor Box Temp |     |
| 0x82BF | VAR\_OUT\_SENSOR\_CONDOUT2 | NASA\_OUTDOOR\_COND\_OUT2 | Sub heat exchanger outlet temperature |     |
| 0x82C0 |     | NASA\_OUTDOOR\_COMP3\_ORDER\_HZ |     |     |
| 0x82C1 |     | NASA\_OUTDOOR\_COMP3\_TARGET\_HZ |     |     |
| 0x82C2 |     | NASA\_OUTDOOR\_COMP3\_RUN\_HZ |     |     |
| 0x82C3 |     | NASA\_OUTDOOR\_DCLINK3\_VOLT |     |     |
| 0x82C4 |     | NASA\_OUTDOOR\_IPM\_TEMP3 |     |     |
| 0x82C8 | VAR\_OUT\_SENSOR\_ACCUM\_TEMP | NASA\_OUTDOOR\_ACCUM\_TEMP | Accumulator outlet temperature |     |
| 0x82C9 | VAR\_OUT\_SENSOR\_ENGINE\_WATER\_TEMP | NASA\_OUTDOOR\_ENGINE\_WATER\_TEMP | Engine water temperature |     |
| 0x82CA | VAR\_OUT\_OIL\_BYPASS\_VALVE | NASA\_OUTDOOR\_OIL\_BYPASS\_VALVE | Oil Bypass Valve |     |
| 0x82CB | VAR\_OUT\_SUCTION\_OVER\_HEAT | NASA\_OUTDOOR\_SUCTION\_OVER\_HEAT | Suction superheat |     |
| 0x82CC | VAR\_OUT\_SUB\_COND\_OVER\_HEAT | NASA\_OUTDOOR\_SUB\_COND\_OVER\_HEAT | Sub heat exchanger outlet superheat |     |
| 0x82CD | VAR\_OUT\_OVER\_COOL | NASA\_OUTDOOR\_OVER\_COOL | Outdoor unit supercooling |     |
| 0x82CE | VAR\_OUT\_COND\_OVER\_COOL | NASA\_OUTDOOR\_COND\_OVER\_COOL | Outdoor heat exchanger subcooling degree |     |
| 0x82CF | VAR\_OUT\_ENGINE\_RPM | NASA\_OUTDOOR\_ENGINE\_RPM | Engine RPM |     |
| 0x82D0 | VAR\_OUT\_APPEARANCE\_RPM | NASA\_OUTDOOR\_APPEARANCE\_RPM | Appearance RPM |     |
| 0x82D1 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x82D2 | VAR\_OUT\_SUB\_COND\_EEV\_STEP | NASA\_OUTDOOR\_SUB\_COND\_EEV\_STEP | Sub EEV |     |
| 0x82D3 |     | NASA\_OUTDOOR\_SNOW\_LEVEL |     |     |
| 0x82D4 | VAR\_OUT\_?? |     |     |     |
| 0x82D5 |     | NASA\_OUTDOOR\_UPL\_TP\_COOL |     |     |
| 0x82D6 |     | NASA\_OUTDOOR\_UPL\_TP\_HEAT |     |     |
| 0x82D9 | VAR\_OUT\_?? |     |     |     |
| 0x82DA | VAR\_OUT\_?? |     |     |     |
| 0x82DB | VAR\_OUT\_PHASE\_CURRENT | NASA\_OUTDOOR\_PHASE\_CURRENT | Phase current value |     |
| 0x82DC | VAR\_OUT\_?? |     |     |     |
| 0x82DD | VAR\_OUT\_?? |     |     |     |
| 0x82DE | VAR\_OUT\_SENSOR\_EVAIN | NASA\_OUTDOOR\_EVA\_IN | Eva In for EHS |     |
| 0x82DF | VAR\_OUT\_SENSOR\_TW1 | NASA\_OUTDOOR\_TW1\_TEMP | Water In 1 for EHS |     |
| 0x82E0 | VAR\_OUT\_SENSOR\_TW2 | NASA\_OUTDOOR\_TW2\_TEMP | Water In 2 for EHS |     |
| 0x82E1 | VAR\_OUT\_?? |     |     |     |
| 0x82E3 | VAR\_OUT\_PRODUCT\_OPTION\_CAPA |     | Outdoor unit product option capacity (based on 0.1Kw) for EHS |     |
| 0x82E7 | VAR\_OUT\_SENSOR\_TOTAL\_SUCTION |     | Total Suction Sensor | Min -41, Max 150 |
| 0x82E8 | VAR\_OUT\_LOAD\_MCU\_HR\_BYPASS\_EEV |     | MCU HR Bypass EEV opening diagram |     |
| 0x82E9 | VAR\_OUT\_SENSOR\_PFCM1 |     | PFCM#1 element temperature | Min -54, Max 3000 |
| 0x82ED | VAR\_OUT\_?? |     |     |     |
| 0x82F5 | VAR\_OUT\_HIGH\_OVERLOAD\_DETECT |     | PFCM#1 element temperature |     |
| 0x82F6 | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x82F9 | VAR\_OUT\_SENSOR\_SUCTION3\_1SEC |     | Suction3 temperature |     |
| 0x82FC | VAR\_OUT\_LOAD\_EVI\_SOL\_EEV |     | EVI SOL EEV |     |
| 0x82FD | VAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8401 | LVAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8404 | LVAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8405 | LVAR\_OUT\_LOAD\_COMP1\_RUNNING\_TIME | NASA\_OUTDOOR\_COMP1\_RUNNING\_TIME | OutdoorTableCompressorRunningTime 1 | hours |
| 0x8406 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_COMP2\_RUNNING\_TIME | OutdoorTableCompressorRunningTime 2 |     |
| 0x8408 | LVAR\_OUT\_?? |     |     |     |
| 0x8409 | LVAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x840B | LVAR\_OUT\_AUTO\_INSPECT\_RESULT0 |     |     |     |
| 0x840C | LVAR\_OUT\_AUTO\_INSPECT\_RESULT1 |     |     |     |
| 0x840E |     | NASA\_OUTDOOR\_COMP3\_RUNNING\_TIME |     |     |
| 0x840F | LVAR\_OUT\_?? |     |     |     |
| 0x8410 | LVAR\_OUT\_?? |     |     |     |
| 0x8411 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_1UNIT | Instantaneous power consumption of outdoor unit. One outdoor unit. Not used by the controller. | appears about every 135 seconds, so less often than 0x8413 |
| 0x8412 |     | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_1UNIT\_ACCUM | Cumulative power consumption of outdoor unit. One outdoor unit. Not used by the controller. |     |
| 0x8413 | LVAR\_OUT\_CONTROL\_WATTMETER\_1W\_1MIN\_SUM | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_ALL\_UNIT | Outdoor unit instantaneous power consumption. Sum of modules | appears about every 30 seconds, not once in a minute |
| 0x8414 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_ALL\_UNIT\_ACCUM | Outdoor unit cumulative power consumption. Sum of modules | value is Wh, so do div 1000 |
| 0x8415 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_TOTAL\_SUM | Total (indoor + outdoor) instantaneous power consumption | never seen in NASA data from EHS Mono HT Quiet |
| 0x8416 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_CONTROL\_WATTMETER\_TOTAL\_SUM\_ACCUM | Total (indoor + outdoor) cumulative power consumption | never seen in NASA data from EHS Mono HT Quiet |
| 0x8417 | LVAR\_OUT\_?? | NASA\_OUTDOOR\_VARIABLE\_SETUP\_INFO |     |     |
| 0x841F | LVAR\_OUT\_?? |     |     | seen in NASA data from EHS Mono HT Quiet |
| 0x8601 | STR\_OUT\_INSTALL\_INVERTER\_AND\_BOOTLOADER\_INFO | NASA\_OUTDOOR\_SUBMICOM | Structure Type |     |
| 0x8608 | STR\_OUT\_?? |     | Structure Type | seen in NASA data from EHS Mono HT Quiet |
| 0x860A | STR\_OUT\_BASE\_OPTION |     | Structure Type |     |
| 0x860C | STR\_OUT\_?? |     | Structure Type |     |
| 0x860D | STR\_OUT\_INSTALL\_MODEL\_INFO | NASA\_OUTDOOR\_MODELINFORMATION | Structure Type |     |
| 0x860F | STR\_OUT\_INSTALL\_OUTDOOR\_SETUP\_INFO | NASA\_OUTDOOR\_SETUP\_INFO | Structure Type |     |
| 0x8613 | STR\_OUT\_REF\_CHECK\_INFO |     | Structure Type |     |

  

### NASA Message Numbers, detailed info
| MsgNr | Type | Signed | Unit | Arithmetic |
| --- | --- | --- | --- | --- |
| 0x0000 |     |     |     |     |
| 0x0004 |     |     |     |     |
| 0x0007 |     |     |     |     |
| 0x0008 |     |     |     |     |
| 0x000A |     |     |     |     |
| 0x000D |     |     |     |     |
| 0x000E |     |     |     |     |
| 0x000F |     |     |     |     |
| 0x0010 |     |     |     |     |
| 0x0011 |     |     |     |     |
| 0x0012 |     |     |     |     |
| 0x0013 |     |     |     |     |
| 0x0014 |     |     |     |     |
| 0x0015 |     |     |     |     |
| 0x0016 |     |     |     |     |
| 0x0017 |     |     |     |     |
| 0x0018 |     |     |     |     |
| 0x0019 |     |     |     |     |
| 0x001A |     |     |     |     |
| 0x001B |     |     |     |     |
| 0x001C |     |     |     |     |
| 0x001D |     |     |     |     |
| 0x001E |     |     |     |     |
| 0x001F |     |     |     |     |
| 0x0020 |     |     |     |     |
| 0x0021 |     |     |     |     |
| 0x0023 |     |     |     |     |
| 0x0025 | ENUM |     |     |     |
| 0x0202 | VAR | false |     |     |
| 0x0203 |     |     |     |     |
| 0x0204 |     |     |     |     |
| 0x0205 |     |     |     |     |
| 0x0206 |     |     |     |     |
| 0x0207 | VAR | false |     |     |
| 0x0208 |     |     |     |     |
| 0x0209 |     |     |     |     |
| 0x0210 |     |     |     |     |
| 0x0211 | VAR | false |     |     |
| 0x0213 |     |     |     |     |
| 0x0214 |     |     |     |     |
| 0x0217 |     |     |     |     |
| 0x0401 | LVAR | false |     |     |
| 0x0402 | LVAR | false |     |     |
| 0x0403 |     |     |     |     |
| 0x0406 |     |     |     |     |
| 0x0407 |     |     |     |     |
| 0x0408 | LVAR | false |     |     |
| 0x0409 | LVAR | false |     |     |
| 0x040A | LVAR | false |     |     |
| 0x040B | LVAR | false |     | LogicalAnd 0xFF |
| 0x040C | LVAR | false |     |     |
| 0x040D | LVAR | false |     |     |
| 0x040E | LVAR | false |     |     |
| 0x040F | LVAR | false |     |     |
| 0x0410 | LVAR | false |     |     |
| 0x0411 | LVAR | false | Celsius | (value & 0xFFFF0000u) >> 16) / 10.0; |
| 0x0412 | LVAR | false | Celsius | (value & 0xFFFF0000u) >> 16) / 10.0; |
| 0x0413 | LVAR | false | Celsius | (value & 0xFFFF0000u) >> 16) / 10.0; |
| 0x0414 | LVAR | false | Celsius | (value & 0xFFFF0000u) >> 16) / 10.0; |
| 0x0415 | LVAR | false |     |     |
| 0x0416 | LVAR | false |     |     |
| 0x0417 | LVAR |     |     |     |
| 0x0418 | LVAR |     |     |     |
| 0x0419 | LVAR |     |     |     |
| 0x041B | LVAR |     |     |     |
| 0x041C |     |     |     |     |
| 0x041D |     |     |     |     |
| 0x041E |     |     |     |     |
| 0x041F |     |     |     |     |
| 0x0420 |     |     |     |     |
| 0x0421 |     |     |     |     |
| 0x0422 |     |     |     |     |
| 0x0423 |     |     |     |     |
| 0x0434 |     |     |     |     |
| 0x0435 |     |     |     |     |
| 0x0436 |     |     |     |     |
| 0x0437 |     |     |     |     |
| 0x0438 |     |     |     |     |
| 0x0439 |     |     |     |     |
| 0x043A |     |     |     |     |
| 0x043B |     |     |     |     |
| 0x043C |     |     |     |     |
| 0x043D |     |     |     |     |
| 0x043E |     |     |     |     |
| 0x043F |     |     |     |     |
| 0x0440 |     |     |     |     |
| 0x0441 |     |     |     |     |
| 0x0442 |     |     |     |     |
| 0x0443 |     |     |     |     |
| 0x0444 |     |     |     |     |
| 0x0445 |     |     |     |     |
| 0x0448 | LVAR | false |     |     |
| 0x0600 | STR |     |     |     |
| 0x0601 | STR |     |     |     |
| 0x0602 | STR |     |     |     |
| 0x0603 | STR |     |     |     |
| 0x0604 |     |     |     |     |
| 0x0605 | STR |     |     |     |
| 0x0607 | STR |     |     |     |
| 0x0608 | STR |     |     |     |
| 0x060C | STR |     |     |     |
| 0x0613 |     |     |     |     |
| 0x0619 |     |     |     |     |
| 0x061A | STR |     |     |     |
| 0x061C | STR |     |     |     |
| 0x061F | STR |     |     |     |
| 0x2000 |     |     |     |     |
| 0x2001 |     |     |     |     |
| 0x2002 |     |     |     |     |
| 0x2003 |     |     |     |     |
| 0x2004 | ENUM |     |     |     |
| 0x2006 |     |     |     |     |
| 0x2007 |     |     |     |     |
| 0x2008 |     |     |     |     |
| 0x2009 |     |     |     |     |
| 0x200A |     |     |     |     |
| 0x200F | ENUM |     |     |     |
| 0x2010 | ENUM |     |     |     |
| 0x2012 | ENUM |     |     |     |
| 0x2017 |     |     |     |     |
| 0x2018 |     |     |     |     |
| 0x22F7 | VAR |     |     |     |
| 0x22F9 | VAR |     |     |     |
| 0x22FA | VAR |     |     |     |
| 0x22FB | VAR |     |     |     |
| 0x22FC | VAR |     |     |     |
| 0x22FD | VAR |     |     |     |
| 0x22FE | VAR |     |     |     |
| 0x22FF | VAR |     |     |     |
| 0x2400 | LVAR |     |     |     |
| 0x2401 | LVAR |     |     |     |
| 0x24FB | LVAR |     |     |     |
| 0x24FC | LVAR |     |     |     |
| 0x4000 | ENUM |     |     |     |
| 0x4001 | ENUM |     |     |     |
| 0x4002 | ENUM |     |     |     |
| 0x4003 | ENUM |     |     |     |
| 0x4004 | ENUM |     |     |     |
| 0x4006 | ENUM |     |     |     |
| 0x4007 | ENUM |     |     |     |
| 0x4008 | ENUM |     |     |     |
| 0x400F | ENUM |     |     |     |
| 0x4010 | ENUM |     |     |     |
| 0x4011 | ENUM |     |     |     |
| 0x4012 | ENUM |     |     |     |
| 0x4015 | ENUM |     |     |     |
| 0x4018 | ENUM |     |     |     |
| 0x4019 | ENUM |     |     |     |
| 0x401B | ENUM |     |     |     |
| 0x4023 | ENUM |     |     |     |
| 0x4024 | ENUM |     |     |     |
| 0x4025 |     |     |     |     |
| 0x4027 | ENUM |     |     |     |
| 0x4028 | ENUM |     |     |     |
| 0x4029 | ENUM |     |     |     |
| 0x402A | ENUM |     |     |     |
| 0x402B | ENUM |     |     |     |
| 0x402D | ENUM |     |     |     |
| 0x402E | ENUM |     |     |     |
| 0x402F | ENUM |     |     |     |
| 0x4031 | ENUM |     |     |     |
| 0x4035 | ENUM |     |     |     |
| 0x4038 | ENUM |     |     |     |
| 0x403D |     |     |     |     |
| 0x403E |     |     |     |     |
| 0x403F |     |     |     |     |
| 0x4040 |     |     |     |     |
| 0x4041 |     |     |     |     |
| 0x4042 |     |     |     |     |
| 0x4043 | ENUM |     |     |     |
| 0x4045 |     |     |     |     |
| 0x4046 | ENUM |     |     |     |
| 0x4047 | ENUM |     |     |     |
| 0x4048 | ENUM |     |     |     |
| 0x404F | ENUM |     |     |     |
| 0x4050 |     |     |     |     |
| 0x4051 | ENUM |     |     |     |
| 0x4059 | ENUM |     |     |     |
| 0x405B |     |     |     |     |
| 0x405C |     |     |     |     |
| 0x405D |     |     |     |     |
| 0x405E |     |     |     |     |
| 0x405F | ENUM |     |     |     |
| 0x4060 | ENUM |     |     |     |
| 0x4063 |     |     |     |     |
| 0x4064 |     |     |     |     |
| 0x4065 | ENUM |     |     |     |
| 0x4066 | ENUM |     |     |     |
| 0x4067 | ENUM |     |     |     |
| 0x4068 | ENUM |     |     |     |
| 0x4069 | ENUM |     |     |     |
| 0x406A | ENUM |     |     |     |
| 0x406B | ENUM |     |     |     |
| 0x406C | ENUM |     |     |     |
| 0x406D | ENUM |     |     |     |
| 0x406E | ENUM |     |     |     |
| 0x406F | ENUM |     |     |     |
| 0x4070 | ENUM |     |     |     |
| 0x4073 | ENUM |     |     |     |
| 0x4074 | ENUM |     |     |     |
| 0x4076 | ENUM |     |     |     |
| 0x4077 | ENUM |     |     |     |
| 0x407B | ENUM |     |     |     |
| 0x407D | ENUM |     |     |     |
| 0x407E | ENUM |     |     |     |
| 0x4085 | ENUM |     |     |     |
| 0x4086 | ENUM |     |     |     |
| 0x4087 | ENUM |     |     |     |
| 0x4089 | ENUM |     |     |     |
| 0x408A | ENUM |     |     |     |
| 0x4093 | ENUM |     |     |     |
| 0x4094 | ENUM |     |     |     |
| 0x4095 | ENUM |     |     |     |
| 0x4096 | ENUM |     |     |     |
| 0x4097 | ENUM |     |     |     |
| 0x4098 | ENUM |     |     |     |
| 0x4099 | ENUM |     |     |     |
| 0x409A | ENUM |     |     |     |
| 0x409B | ENUM |     |     |     |
| 0x409C | ENUM |     |     |     |
| 0x409D | ENUM |     |     |     |
| 0x409E | ENUM |     |     |     |
| 0x409F | ENUM |     |     |     |
| 0x40A0 | ENUM |     |     |     |
| 0x40A1 | ENUM |     |     |     |
| 0x40A2 | ENUM |     |     |     |
| 0x40A3 | ENUM |     |     |     |
| 0x40A4 | ENUM |     |     |     |
| 0x40A5 | ENUM |     |     |     |
| 0x40A6 | ENUM |     |     |     |
| 0x40A7 | ENUM |     |     |     |
| 0x40B1 |     |     |     |     |
| 0x40B4 | ENUM |     |     |     |
| 0x40B5 | ENUM |     |     |     |
| 0x40BB | ENUM |     |     |     |
| 0x40BC | ENUM |     |     |     |
| 0x40BD | ENUM |     |     |     |
| 0x40C0 | ENUM |     |     |     |
| 0x40C1 | ENUM |     |     |     |
| 0x40C2 | ENUM |     |     |     |
| 0x40C3 | ENUM |     |     |     |
| 0x40C4 | ENUM |     | \[%\] |     |
| 0x40C5 | ENUM |     |     |     |
| 0x40C6 | ENUM |     |     |     |
| 0x40C7 |     |     |     |     |
| 0x40C8 |     |     |     |     |
| 0x40C9 |     |     |     |     |
| 0x40CA |     |     |     |     |
| 0x40CB |     |     |     |     |
| 0x40CC |     |     |     |     |
| 0x40CD |     |     |     |     |
| 0x40CE |     |     |     |     |
| 0x40CF |     |     |     |     |
| 0x40D0 |     |     |     |     |
| 0x40D1 |     |     |     |     |
| 0x40D2 |     |     |     |     |
| 0x40D5 | ENUM |     |     |     |
| 0x40D6 | ENUM |     |     |     |
| 0x40E3 | ENUM |     |     |     |
| 0x40E7 | ENUM |     |     |     |
| 0x40F7 | ENUM |     |     |     |
| 0x40FB | ENUM |     |     |     |
| 0x40FC | ENUM |     |     |     |
| 0x4101 | ENUM |     |     |     |
| 0x4102 | ENUM |     |     |     |
| 0x4103 | ENUM |     |     |     |
| 0x4104 | ENUM |     |     |     |
| 0x4105 | ENUM |     |     |     |
| 0x4107 | ENUM |     |     |     |
| 0x4108 | ENUM |     |     |     |
| 0x410D | ENUM |     |     |     |
| 0x4113 | ENUM |     |     |     |
| 0x4117 | ENUM |     |     |     |
| 0x4119 | ENUM |     |     |     |
| 0x411A | ENUM |     |     |     |
| 0x411B | ENUM |     |     |     |
| 0x411C | ENUM |     |     |     |
| 0x411D | ENUM |     |     |     |
| 0x411E | ENUM |     |     |     |
| 0x4123 | ENUM |     |     |     |
| 0x4124 | ENUM |     |     |     |
| 0x4125 | ENUM |     |     |     |
| 0x4127 | ENUM |     |     |     |
| 0x4128 | ENUM |     |     |     |
| 0x412A | ENUM |     |     |     |
| 0x412D | ENUM |     |     |     |
| 0x4147 | ENUM |     |     |     |
| 0x4149 | ENUM |     |     |     |
| 0x4201 | VAR | true | Celsius | division by 10 |
| 0x4202 | VAR | true | Celsius | division by 10 |
| 0x4203 | VAR | true | Celsius | division by 10 |
| 0x4204 | VAR | true | Celsius | division by 10 |
| 0x4205 | VAR | true | Celsius | division by 10 |
| 0x4206 | VAR | true | Celsius | division by 10 |
| 0x4207 | VAR | true | Celsius | subtract 55 |
| 0x4208 |     |     |     |     |
| 0x4209 |     |     |     |     |
| 0x420B | VAR | true | Celsius | division by 10 |
| 0x420C | VAR | true | Celsius | division by 10 |
| 0x4211 | VAR | false | kW  | division by 8.6 |
| 0x4212 | VAR | false | kW  | division by 8.6 |
| 0x4213 | VAR |     |     |     |
| 0x4217 | VAR | false |     |     |
| 0x4218 | VAR | false |     |     |
| 0x4219 |     |     |     |     |
| 0x421A |     |     |     |     |
| 0x421B | VAR | false |     |     |
| 0x4220 |     |     |     |     |
| 0x4229 | VAR |     |     |     |
| 0x422A | VAR | true | Celsius | division by 10 |
| 0x422B | VAR | true | Celsius | division by 10 |
| 0x4235 | VAR | true | Celsius | division by 10 |
| 0x4236 | VAR | true | Celsius | division by 10 |
| 0x4237 | VAR | true | Celsius | division by 10 |
| 0x4238 | VAR | true | Celsius | division by 10 |
| 0x4239 | VAR | true | Celsius | division by 10 |
| 0x423E | VAR |     |     |     |
| 0x4247 | VAR | true | Celsius | division by 10 |
| 0x4248 | VAR | true | Celsius | division by 10 |
| 0x424A | VAR | true | Celsius | division by 10 |
| 0x424B | VAR | true | Celsius | division by 10 |
| 0x424C | VAR | true | Celsius | division by 10 |
| 0x424D | VAR | true | Celsius | division by 10 |
| 0x424E | VAR | true | Celsius | division by 10 |
| 0x424F | VAR | true | Celsius | division by 10 |
| 0x4250 | VAR | true | Celsius | division by 10 |
| 0x4251 | VAR | true | Celsius | division by 10 |
| 0x4252 | VAR | true | Celsius | division by 10 |
| 0x4253 | VAR | true | Celsius | division by 10 |
| 0x4254 | VAR | true | Celsius | division by 10 |
| 0x4255 | VAR | true | Celsius | division by 10 |
| 0x4256 | VAR | true | Celsius | division by 10 |
| 0x4257 | VAR | true | Celsius | division by 10 |
| 0x4258 | VAR | true | Celsius | division by 10 |
| 0x4259 | VAR | true | Celsius | division by 10 |
| 0x425A | VAR | true | Celsius | division by 10 |
| 0x425B | VAR | true | Celsius | division by 10 |
| 0x425C | VAR | true | Celsius | division by 10 |
| 0x425D | VAR | true | Celsius | division by 10 |
| 0x425E | VAR | true | Celsius | division by 10 |
| 0x425F | VAR | true | Celsius | division by 10 |
| 0x4260 | VAR | true | Celsius | division by 10 |
| 0x4261 | VAR | true | Celsius | division by 10 |
| 0x4262 | VAR | true | Celsius | division by 10 |
| 0x4263 | VAR | false |     |     |
| 0x4264 | VAR | false |     |     |
| 0x4265 | VAR | false |     |     |
| 0x4266 | VAR | false |     |     |
| 0x4267 | VAR | true | Celsius | division by 10 |
| 0x4268 | VAR | true | Celsius | division by 10 |
| 0x4269 | VAR | false |     |     |
| 0x426A | VAR | true | Celsius | division by 10 |
| 0x426B | VAR | true |     |     |
| 0x426C | VAR | true |     | division by 0.1 |
| 0x426D | VAR | true | Celsius | division by 10 |
| 0x426E | VAR | true | Celsius | division by 10 |
| 0x426F | VAR | true | Celsius | division by 10 |
| 0x4270 | VAR | true | Celsius | division by 10 |
| 0x4271 | VAR | true | Celsius | division by 10 |
| 0x4272 | VAR | true | Celsius | division by 10 |
| 0x4273 | VAR | true | Celsius | division by 10 |
| 0x4274 | VAR | true | Celsius | division by 10 |
| 0x4275 | VAR | true | Celsius | division by 10 |
| 0x4276 | VAR | true | Celsius | division by 10 |
| 0x4277 | VAR | true | Celsius | division by 10 |
| 0x4278 | VAR | true | Celsius | division by 10 |
| 0x4279 | VAR | true | Celsius | division by 10 |
| 0x427A | VAR | true | Celsius | division by 10 |
| 0x427B | VAR | true | Celsius | division by 10 |
| 0x427C | VAR | true | Celsius | division by 10 |
| 0x427D | VAR | false |     |     |
| 0x427E | VAR | false |     |     |
| 0x427F | VAR | true | Celsius | division by 10 |
| 0x4284 |     |     |     |     |
| 0x4286 | VAR | false | Celsius | division by 10 |
| 0x4287 | VAR | false | Celsius | division by 10 |
| 0x4288 | VAR | false |     |     |
| 0x4289 | VAR | false |     | division by 0.1 |
| 0x428A | VAR | false | Celsius | division by 10 |
| 0x428C | VAR | true | Celsius | division by 10 |
| 0x428D | VAR |     |     |     |
| 0x4290 |     |     |     |     |
| 0x4291 |     |     |     |     |
| 0x4292 |     |     |     |     |
| 0x4293 |     |     |     |     |
| 0x4294 |     |     |     |     |
| 0x4295 |     |     |     |     |
| 0x4296 |     |     |     |     |
| 0x4297 | VAR | true |     |     |
| 0x4298 | VAR | true |     |     |
| 0x4299 |     |     |     |     |
| 0x429A |     |     |     |     |
| 0x429B |     |     |     |     |
| 0x429C |     |     |     |     |
| 0x429F | VAR | false |     |     |
| 0x42A1 | VAR | false |     |     |
| 0x42A3 | VAR | false |     |     |
| 0x42A5 | VAR | true | Celsius | division by 10 |
| 0x42A6 | VAR | true | Celsius | division by 10 |
| 0x42A7 | VAR | true | Celsius | division by 10 |
| 0x42A8 | VAR | true | Celsius | division by 10 |
| 0x42A9 | VAR | true | Celsius | division by 10 |
| 0x42AA | VAR | true | Celsius | division by 10 |
| 0x42AB | VAR | true | Celsius | division by 10 |
| 0x42AC | VAR | true | Celsius | division by 10 |
| 0x42AD | VAR | true | Celsius | division by 10 |
| 0x42AE | VAR | true | Celsius | division by 10 |
| 0x42AF | VAR | true | Celsius | division by 10 |
| 0x42B0 | VAR | true | Celsius | division by 10 |
| 0x42B1 | VAR | false |     |     |
| 0x42B2 | VAR | false |     |     |
| 0x42B3 | VAR | false |     |     |
| 0x42C2 | VAR | true | Celsius | division by 10 |
| 0x42C3 | VAR | true | Celsius | division by 10 |
| 0x42C4 | VAR | true | kgfcm2 | division by 100 |
| 0x42C5 | VAR | true | kgfcm2 | division by 100 |
| 0x42C9 | VAR | true | Celsius | division by 10 |
| 0x42CA | VAR | false |     |     |
| 0x42CB | VAR | false |     |     |
| 0x42CC | VAR | false |     |     |
| 0x42CD | VAR | true | Celsius | division by 10 |
| 0x42CE | VAR | false |     | division by 60 (not documented in NASA.ptc) |
| 0x42CF | VAR | false | Enthalpy | division by 10 |
| 0x42D0 | VAR | false |     |     |
| 0x42D1 | VAR | false |     |     |
| 0x42D2 | VAR | false |     |     |
| 0x42D3 | VAR | false |     |     |
| 0x42D4 | VAR | true | Celsius | division by 10 |
| 0x42D6 | VAR | true | Celsius | division by 10 |
| 0x42D7 | VAR | true | Celsius | division by 10 |
| 0x42D8 | VAR | true | Celsius | division by 10 |
| 0x42D9 | VAR | true | Celsius | division by 10 |
| 0x42DB | VAR | false | Celsius | division by 10 |
| 0x42DC | VAR | false | Celsius | division by 10 |
| 0x42DD | VAR | false | Celsius | division by 10 |
| 0x42DE | VAR | false | Celsius | division by 10 |
| 0x42E8 | VAR | false |     | division by 10 |
| 0x42E9 | VAR | true |     | division by 10 |
| 0x42ED | VAR | true |     |     |
| 0x42EE | VAR | true |     |     |
| 0x42EF | VAR | true |     |     |
| 0x42F0 | VAR | true | Celsius | division by 10 |
| 0x42F1 | VAR |     |     |     |
| 0x4301 | VAR |     |     |     |
| 0x4302 | VAR | false | kW  | division by 8.6 |
| 0x4401 | LVAR |     |     |     |
| 0x4405 |     |     |     |     |
| 0x4406 |     |     |     |     |
| 0x4407 |     |     |     |     |
| 0x440A | LVAR | false |     | LogicalAnd 0x00000002 + division by 2 |
| 0x440E | LVAR |     |     |     |
| 0x440F | LVAR |     |     |     |
| 0x4415 | LVAR | false |     |     |
| 0x4418 | LVAR | false |     |     |
| 0x441B | LVAR | false |     |     |
| 0x441F | LVAR | false |     |     |
| 0x4420 | LVAR | false |     |     |
| 0x4423 | LVAR |     |     |     |
| 0x4424 | LVAR |     |     |     |
| 0x4426 | LVAR | false | kW  | division by 1000 |
| 0x4427 | LVAR | false | kW  | division by 1000 |
| 0x4604 | STR |     |     |     |
| 0x4608 |     |     |     |     |
| 0x4619 |     |     |     |     |
| 0x461A |     |     |     |     |
| 0x461C |     |     |     |     |
| 0x461D |     |     |     |     |
| 0x461E | STR |     |     |     |
| 0x8000 | ENUM |     |     |     |
| 0x8001 | ENUM |     |     |     |
| 0x8002 | ENUM |     |     |     |
| 0x8003 | ENUM |     |     |     |
| 0x8005 | ENUM |     |     |     |
| 0x800D | ENUM |     |     |     |
| 0x8010 | ENUM |     |     |     |
| 0x8011 | ENUM |     |     |     |
| 0x8012 | ENUM |     |     |     |
| 0x8013 | ENUM |     |     |     |
| 0x8014 | ENUM |     |     |     |
| 0x8015 |     |     |     |     |
| 0x8016 |     |     |     |     |
| 0x8017 | ENUM |     |     |     |
| 0x8018 | ENUM |     |     |     |
| 0x8019 | ENUM |     |     |     |
| 0x801A | ENUM |     |     |     |
| 0x801F | ENUM |     |     |     |
| 0x8020 | ENUM |     |     |     |
| 0x8021 | ENUM |     |     |     |
| 0x8022 | ENUM |     |     |     |
| 0x8023 | ENUM |     |     |     |
| 0x8024 |     |     |     |     |
| 0x8025 | ENUM |     |     |     |
| 0x8026 | ENUM |     |     |     |
| 0x8027 | ENUM |     |     |     |
| 0x802A | ENUM |     |     |     |
| 0x8031 | ENUM |     |     |     |
| 0x8032 | ENUM |     |     |     |
| 0x8033 | ENUM |     |     |     |
| 0x8034 | ENUM |     |     |     |
| 0x8037 | ENUM |     |     |     |
| 0x803B | ENUM |     |     |     |
| 0x803C | ENUM |     |     |     |
| 0x803F | ENUM |     |     |     |
| 0x8043 | ENUM |     |     |     |
| 0x8045 | ENUM |     |     |     |
| 0x8046 | ENUM |     |     |     |
| 0x8047 | ENUM |     |     |     |
| 0x8048 | ENUM |     |     |     |
| 0x8049 | ENUM |     |     |     |
| 0x804A | ENUM |     |     |     |
| 0x804B | ENUM |     |     |     |
| 0x804C | ENUM |     |     |     |
| 0x804D | ENUM |     |     |     |
| 0x804E | ENUM |     |     |     |
| 0x804F | ENUM |     |     |     |
| 0x8050 | ENUM |     |     |     |
| 0x8051 | ENUM |     |     |     |
| 0x8052 | ENUM |     |     |     |
| 0x8053 | ENUM |     |     |     |
| 0x8054 | ENUM |     |     |     |
| 0x8055 | ENUM |     |     |     |
| 0x8058 | ENUM |     |     |     |
| 0x8059 | ENUM |     |     |     |
| 0x805A | ENUM |     |     |     |
| 0x805B | ENUM |     |     |     |
| 0x805C | ENUM |     |     |     |
| 0x805D | ENUM |     |     |     |
| 0x805E | ENUM |     |     |     |
| 0x8061 | ENUM |     |     |     |
| 0x8062 |     |     |     |     |
| 0x8063 | ENUM |     |     |     |
| 0x8065 | ENUM |     |     |     |
| 0x8066 | ENUM |     |     |     |
| 0x8077 | ENUM |     |     |     |
| 0x8078 | ENUM |     |     |     |
| 0x8079 | ENUM |     |     |     |
| 0x807A | ENUM |     |     |     |
| 0x807B | ENUM |     |     |     |
| 0x807C | ENUM |     |     |     |
| 0x807D | ENUM |     |     |     |
| 0x807E | ENUM |     |     |     |
| 0x807F | ENUM |     |     |     |
| 0x8081 | ENUM |     |     |     |
| 0x8083 | ENUM |     |     |     |
| 0x808C | ENUM |     |     |     |
| 0x808D | ENUM |     |     |     |
| 0x808E | ENUM | false |     | LogicalAnd 0x0F |
| 0x808F | ENUM |     |     |     |
| 0x8092 | ENUM | false |     |     |
| 0x8099 | ENUM | false |     |     |
| 0x809C | ENUM |     |     |     |
| 0x809D | ENUM |     |     |     |
| 0x809E | ENUM |     |     |     |
| 0x80A5 | ENUM |     |     |     |
| 0x80A6 | ENUM |     |     |     |
| 0x80A7 | ENUM |     |     |     |
| 0x80A8 | ENUM |     |     |     |
| 0x80A9 | ENUM |     |     |     |
| 0x80AA | ENUM |     |     |     |
| 0x80AB | ENUM |     |     |     |
| 0x80AC |     |     |     |     |
| 0x80AE | ENUM |     |     |     |
| 0x80AF | ENUM |     |     |     |
| 0x80B1 | ENUM |     |     |     |
| 0x80B2 | ENUM |     |     |     |
| 0x80B4 | ENUM |     |     |     |
| 0x80B6 | ENUM |     |     |     |
| 0x80B8 | ENUM |     |     |     |
| 0x80B9 | ENUM |     |     |     |
| 0x80BC | ENUM |     |     |     |
| 0x80BE | ENUM |     |     |     |
| 0x80C1 | ENUM |     |     |     |
| 0x80CE | ENUM |     |     |     |
| 0x80D7 | ENUM |     |     |     |
| 0x80D8 | ENUM |     |     |     |
| 0x8200 | VAR |     |     |     |
| 0x8201 | VAR |     |     |     |
| 0x8202 | VAR | false |     |     |
| 0x8204 | VAR | true | Celsius | division by 10 |
| 0x8206 | VAR | true | kgfcm2 | division by 10 |
| 0x8208 | VAR | true | kgfcm2 | division by 10 |
| 0x820A | VAR | true | Celsius | division by 10 |
| 0x820C | VAR | true | Celsius | division by 10 |
| 0x820E | VAR | true | Celsius | division by 10 |
| 0x8210 |     |     |     |     |
| 0x8217 | VAR | false |     | division by 10 |
| 0x8218 | VAR | true | Celsius | division by 10 |
| 0x821A | VAR | true | Celsius | division by 10 |
| 0x821C | VAR | true | Celsius | division by 10 |
| 0x821E | VAR | true | Celsius | division by 10 |
| 0x8220 | VAR | true | Celsius | division by 10 |
| 0x8222 | VAR |     |     |     |
| 0x8223 | VAR | true | Celsius | division by 10 |
| 0x8224 | VAR | true | Celsius | division by 10 |
| 0x8225 | VAR | true | Celsius | division by 10 |
| 0x8226 | VAR | false |     |     |
| 0x8227 | VAR |     |     |     |
| 0x8228 |     |     |     |     |
| 0x8229 | VAR | false |     |     |
| 0x822A | VAR | false |     |     |
| 0x822B | VAR | false |     |     |
| 0x822C | VAR | false |     |     |
| 0x822D | VAR | false |     |     |
| 0x822E | VAR | false |     |     |
| 0x822F |     |     |     |     |
| 0x8230 | VAR |     |     |     |
| 0x8231 | VAR |     |     |     |
| 0x8233 | VAR |     |     | division by 8.6 (better 8.5 ?) |
| 0x8234 | VAR |     |     |     |
| 0x8235 | VAR | false |     |     |
| 0x8236 | VAR | false |     |     |
| 0x8237 | VAR | false |     |     |
| 0x8238 | VAR | false |     |     |
| 0x8239 | VAR |     |     |     |
| 0x823B | VAR | false |     |     |
| 0x823C | VAR | false |     |     |
| 0x823D | VAR | false |     |     |
| 0x823E | VAR | false |     |     |
| 0x823F | VAR | false |     |     |
| 0x8240 |     |     |     |     |
| 0x8241 |     |     |     |     |
| 0x8243 | VAR |     |     |     |
| 0x8244 |     |     |     |     |
| 0x8245 |     |     |     |     |
| 0x8247 | VAR |     |     |     |
| 0x8248 | VAR |     |     |     |
| 0x8249 | VAR |     |     |     |
| 0x824B | VAR |     |     |     |
| 0x824C | VAR |     |     |     |
| 0x824F | VAR | false |     | division by 10 |
| 0x8254 | VAR | true | Celsius | division by 10 |
| 0x8255 | VAR | true | Celsius | division by 10 |
| 0x825A | VAR | true | Celsius | division by 10 |
| 0x825B | VAR | true | Celsius | division by 10 |
| 0x825C | VAR | true | Celsius | division by 10 |
| 0x825D | VAR | true | Celsius | division by 10 |
| 0x825E | VAR | true | Celsius | division by 10 |
| 0x825F | VAR | true | Celsius | division by 10 |
| 0x8260 | VAR | true | Celsius | division by 10 |
| 0x8261 | VAR | true | Celsius | division by 10 |
| 0x8262 | VAR | true | Celsius | division by 10 |
| 0x8263 | VAR | true | Celsius | division by 10 |
| 0x8264 | VAR | true | Celsius | division by 10 |
| 0x8265 | VAR | true | Celsius | division by 10 |
| 0x8266 | VAR | true | Celsius | division by 10 |
| 0x8267 | VAR | true | Celsius | division by 10 |
| 0x8268 | VAR | true | Celsius | division by 10 |
| 0x826B | VAR | true | Celsius | division by 10 |
| 0x826C | VAR | true | Celsius | division by 10 |
| 0x826D | VAR | false |     |     |
| 0x826E | VAR | false |     |     |
| 0x826F | VAR | false |     |     |
| 0x8270 | VAR | false |     |     |
| 0x8271 | VAR | false |     |     |
| 0x8272 | VAR | false |     |     |
| 0x8273 | VAR | false |     |     |
| 0x8274 | VAR | false |     |     |
| 0x8275 | VAR | false |     |     |
| 0x8276 | VAR | false |     |     |
| 0x8277 | VAR | false |     |     |
| 0x8278 | VAR | false |     |     |
| 0x8279 |     |     |     |     |
| 0x827A | VAR | true | Celsius | division by 10 |
| 0x827E |     |     |     |     |
| 0x827F |     |     |     |     |
| 0x8280 | VAR | true | Celsius | division by 10 |
| 0x8281 | VAR | true | Celsius | division by 10 |
| 0x8282 |     |     |     |     |
| 0x8287 | VAR | false | HP  |     |
| 0x8298 |     |     |     |     |
| 0x829A | VAR | true | Celsius | division by 10 |
| 0x829B |     |     |     |     |
| 0x829C |     |     |     |     |
| 0x829D |     |     |     |     |
| 0x829F | VAR | true | Celsius | division by 10 |
| 0x82A0 | VAR | true | Celsius | division by 10 |
| 0x82A2 | VAR |     |     |     |
| 0x82A3 |     |     |     |     |
| 0x82A4 |     |     |     |     |
| 0x82A6 |     |     |     |     |
| 0x82A7 |     |     |     |     |
| 0x82A8 | VAR | true |     | division by 8.6 = kW |
| 0x82A9 | VAR | true |     |     |
| 0x82AF | VAR | false |     |     |
| 0x82B2 | VAR |     |     |     |
| 0x82B3 |     |     |     |     |
| 0x82B5 | VAR |     |     |     |
| 0x82B6 | VAR |     |     |     |
| 0x82B8 | VAR | true | kgfcm2 | division by 10 |
| 0x82B9 |     |     |     |     |
| 0x82BA |     |     |     |     |
| 0x82BC | VAR | true |     |     |
| 0x82BD | VAR | true |     |     |
| 0x82BE | VAR | true | Celsius | division by 10 |
| 0x82BF | VAR | true | Celsius | division by 10 |
| 0x82C0 |     |     |     |     |
| 0x82C1 |     |     |     |     |
| 0x82C2 |     |     |     |     |
| 0x82C3 |     |     |     |     |
| 0x82C4 |     |     |     |     |
| 0x82C8 | VAR | true | Celsius | division by 10 |
| 0x82C9 | VAR | true | Celsius | division by 10 |
| 0x82CA | VAR | false |     |     |
| 0x82CB | VAR | false |     | division by 10 |
| 0x82CC | VAR | false |     | division by 10 |
| 0x82CD | VAR | false |     |     |
| 0x82CE | VAR | false |     | division by 10 |
| 0x82CF | VAR | false |     |     |
| 0x82D0 | VAR | false | RPM | division by 10 |
| 0x82D1 | VAR |     |     |     |
| 0x82D2 | VAR | false |     |     |
| 0x82D3 |     |     |     |     |
| 0x82D4 | VAR |     |     |     |
| 0x82D5 |     |     |     |     |
| 0x82D6 |     |     |     |     |
| 0x82D9 | VAR |     |     |     |
| 0x82DA | VAR |     |     |     |
| 0x82DB | VAR | false |     |     |
| 0x82DC | VAR |     |     |     |
| 0x82DD | VAR |     |     |     |
| 0x82DE | VAR | true | Celsius | division by 10 |
| 0x82DF | VAR | true | Celsius | division by 10 |
| 0x82E0 | VAR | true | Celsius | division by 10 |
| 0x82E1 | VAR |     |     |     |
| 0x82E3 | VAR | false | kW  | division by 10.0 |
| 0x82E7 | VAR | true | Celsius | division by 10.0 |
| 0x82E8 | VAR | false |     |     |
| 0x82E9 | VAR | true | Celsius | division by 10 |
| 0x82ED | VAR |     |     |     |
| 0x82F5 | VAR | false |     |     |
| 0x82F6 | VAR |     |     |     |
| 0x82F9 | VAR | true | Celsius | division by 10 |
| 0x82FC | VAR | true |     | \-32767 up to 32767 |
| 0x82FD | VAR |     |     |     |
| 0x8401 | LVAR |     |     |     |
| 0x8404 | LVAR |     |     |     |
| 0x8405 | LVAR | false |     |     |
| 0x8406 | LVAR | false |     |     |
| 0x8408 | LVAR |     |     |     |
| 0x8409 | LVAR |     |     |     |
| 0x840B | LVAR | false |     |     |
| 0x840C | LVAR | false |     |     |
| 0x840E |     |     |     |     |
| 0x840F | LVAR |     |     |     |
| 0x8410 | LVAR |     |     |     |
| 0x8411 | LVAR |     |     |     |
| 0x8412 |     |     |     |     |
| 0x8413 | LVAR | false | kW  | division by 1000 |
| 0x8414 | LVAR | false | kW  | division by 1000 |
| 0x8415 | LVAR |     |     |     |
| 0x8416 | LVAR |     |     |     |
| 0x8417 | LVAR |     |     |     |
| 0x841F | LVAR |     |     |     |
| 0x8601 | STR |     |     |     |
| 0x8608 | STR |     |     |     |
| 0x860A | STR |     |     |     |
| 0x860C | STR |     |     |     |
| 0x860D | STR |     |     |     |
| 0x860F | STR |     |     |     |
| 0x8613 | STR |     |     |     |

## Implementations

There are multiple open source implementations of the encoding and decoding of the NASA bitstream. Here is a list of such known implementations.

*   C++: esphome\_samsung\_ac ([GitHub](https://github.com/omerfaruk-aran/esphome_samsung_hvac_bus/tree/main))
*   Swift: NASAKit ([GitHub](https://github.com/betaphi/NASAKit))