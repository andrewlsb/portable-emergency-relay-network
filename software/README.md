# Software

This folder contains the firmware and test programs developed for the Portable Emergency Communication Relay Network.

The software is organized by development stage so that each major hardware and communication feature can be tested independently before being integrated into the complete system.

## Software Structure

```text
software/
├── 01_gps_test/
├── 02_oled_test/
├── 03_gps_oled/
├── 04_lora_verification/
├── 05_stm32_field_unit/
└── README.md
```

---

## 01 - GPS Test

### Platform

Arduino UNO R4 WiFi

### Purpose

Verify communication with the NEO-6M GPS module and confirm that valid GPS data can be received and decoded.

### Features

- UART communication with NEO-6M
- NMEA data reception
- GPS fix detection
- Latitude decoding
- Longitude decoding
- Satellite count
- Serial Monitor output

### Result

Successfully acquired a GPS fix and displayed real-time GPS information through the Serial Monitor.

Example:

```text
Latitude: 47.667700
Longitude: -122.313024
Satellites: 6
```

Status: **Complete**

---

## 02 - OLED Test

### Platform

Arduino UNO R4 WiFi

### Purpose

Verify communication between the microcontroller and SSD1306 OLED display.

### Features

- I2C communication
- SSD1306 initialization
- Text rendering
- OLED display testing

### Result

The OLED successfully initialized and displayed test text.

Example:

```text
HELLO
```

Status: **Complete**

---

## 03 - GPS + OLED Integration

### Platform

Arduino UNO R4 WiFi

### Purpose

Combine the GPS and OLED subsystems into a standalone GPS display.

### Features

- GPS data acquisition
- GPS fix detection
- Latitude and longitude decoding
- Satellite count
- OLED status display
- Real-time coordinate updates

### Example Display

```text
GPS STATUS
SAT: 6
FIX: YES
LAT: 47.xxxxxx
LON: -122.xxxxxx
```

### Result

GPS information was successfully received, decoded, and displayed on the SSD1306 OLED.

This milestone verified that the GPS and display subsystems could operate together before migration to the STM32 field unit.

Status: **Complete**

---

## 04 - LoRa Verification

### Platform

Arduino UNO R4 WiFi + RYLR998

### Purpose

Verify UART communication with the RYLR998 LoRa transceiver before integrating LoRa communication into the STM32 field unit.

### Features

- RYLR998 UART communication
- AT command interface
- LoRa module configuration
- Point-to-point communication testing
- Bidirectional communication testing

### Verified Configuration

```text
NETWORKID=18
BAND=915000000
IPR=115200
PARAMETER=9,7,1,12
```

### Result

The RYLR998 successfully responded to AT commands and communicated with another RYLR998 module.

Point-to-point wireless communication was successfully verified.

Status: **Complete**

---

# 05 - STM32 Field Unit

### Platform

STM32 NUCLEO-F446RE

### Connected Hardware

- NEO-6M GPS
- SSD1306 OLED
- RYLR998 LoRa module

### Purpose

Integrate the GPS, OLED, and LoRa subsystems onto the STM32 NUCLEO-F446RE to create the first complete field unit.

This represents the transition from isolated Arduino subsystem testing to the primary embedded platform for the field node.

---

## STM32 Peripheral Configuration

| Peripheral | Device | Configuration |
|---|---|---|
| UART4 | NEO-6M GPS | 9600 baud |
| USART1 | RYLR998 LoRa | 115200 baud |
| USART2 | PC / Debug | 115200 baud |
| I2C1 | SSD1306 OLED | 100 kHz |

### Pin Assignment

```text
GPS
NEO-6M TX -> PA1 / UART4_RX

OLED
SCL -> PB8 / I2C1_SCL
SDA -> PB9 / I2C1_SDA

LoRa
RYLR998 TXD -> PA10 / USART1_RX
RYLR998 RXD -> PA9  / USART1_TX
```

---

## GPS Processing

The NEO-6M continuously sends NMEA sentences to the STM32 through UART4.

The STM32 collects incoming characters into a buffer and processes complete lines.

The firmware searches for GPGGA messages because they contain the information required by the field unit, including:

- Latitude
- Longitude
- GPS fix status
- Satellite count

Example:

```text
$GPGGA,043520.00,4740.05797,N,12218.76981,W,1,07,1.82,79.7,M,-18.7,M,,*5A
```

The NMEA latitude and longitude values are converted into decimal degrees before being displayed or transmitted.

---

## OLED Output

After acquiring a valid GPS fix, the STM32 updates the SSD1306 OLED.

Example:

```text
GPS FIX

LAT 47.66768
LON -122.31284
SAT 9
```

The OLED allows the field unit to operate independently without requiring a computer or Serial Monitor to view the current GPS status.

---

## LoRa Transmission

After receiving and decoding a valid GPS position, the STM32 creates a LoRa payload.

Example:

```text
GPS,47.66776,-122.31289
```

The firmware then generates an RYLR998 command:

```text
AT+SEND=0,23,GPS,47.66776,-122.31289
```

The command is transmitted from the STM32 to the RYLR998 through USART1.

The LoRa module then sends the packet wirelessly.

---

## Verified Wireless Output

The Arduino relay-side RYLR998 successfully received GPS packets transmitted by the STM32 field unit.

Example:

```text
+RCV=1,23,GPS,47.66776,-122.31289,-31,11
+RCV=1,23,GPS,47.66776,-122.31289,-41,11
+RCV=1,23,GPS,47.66776,-122.31289,-39,11
+RCV=1,23,GPS,47.66776,-122.31289,-35,11
+RCV=1,23,GPS,47.66775,-122.31289,-35,10
```

This verifies the complete data path:

```text
NEO-6M GPS
     ↓
STM32 UART4
     ↓
GPS Parser
     ↓
Latitude / Longitude
     ├────────────► SSD1306 OLED
     │
     ▼
LoRa Packet Generation
     ↓
STM32 USART1
     ↓
RYLR998 Field Unit
     ↓
915 MHz LoRa
     ↓
RYLR998 Receiver
     ↓
Arduino UNO R4 WiFi
     ↓
Serial Monitor
```

---

## Current STM32 Field Unit Features

- [x] STM32 peripheral configuration
- [x] GPS UART communication
- [x] NMEA message reception
- [x] GPGGA parsing
- [x] GPS fix detection
- [x] Latitude conversion
- [x] Longitude conversion
- [x] Satellite count extraction
- [x] SSD1306 I2C communication
- [x] GPS information displayed on OLED
- [x] RYLR998 UART communication
- [x] LoRa AT command generation
- [x] GPS payload generation
- [x] GPS coordinate transmission over LoRa
- [x] Wireless GPS packet reception verified
- [ ] Relay packet forwarding
- [ ] Multi-hop routing
- [ ] Emergency SOS packet
- [ ] Duplicate packet detection
- [ ] Battery operation

Status: **Complete for Milestone 4**

---

# Current Software Architecture

```text
FIELD UNIT
STM32 NUCLEO-F446RE

GPS Input
   │
   ▼
UART4 Driver
   │
   ▼
NMEA Buffer
   │
   ▼
GPGGA Parser
   │
   ├──────────────► OLED Display
   │
   ▼
GPS Payload
   │
   ▼
LoRa AT Command
   │
   ▼
USART1
   │
   ▼
RYLR998
   │
   │ Wireless
   ▼
RELAY NODE
Arduino UNO R4 WiFi
   │
   ▼
RYLR998 Receiver
```

---

# Development Progress

| Stage | Description | Status |
|---|---|---|
| 01 | GPS module test | Complete |
| 02 | OLED display test | Complete |
| 03 | GPS + OLED integration | Complete |
| 04 | RYLR998 LoRa verification | Complete |
| 05 | STM32 field unit integration | Complete |
| 06 | Three-node relay firmware | Next |
| 07 | Emergency message protocol | Planned |
| 08 | Battery deployment firmware | Planned |

---

# Next Development Stage

The next software stage is the three-node relay network.

The target architecture is:

```text
Node 1
STM32 FIELD UNIT
GPS + OLED + LoRa
       │
       │ GPS packet
       ▼
Node 2
ARDUINO RELAY
LoRa
       │
       │ Forwarded packet
       ▼
Node 3
BASE STATION
LoRa + PC
```

The relay firmware will need to:

1. Receive a LoRa packet from the field unit.
2. Identify the sender and packet contents.
3. Extract the GPS payload.
4. Forward the packet to the base station.
5. Prevent unnecessary packet retransmission.

This will create the project's first multi-hop communication path.
