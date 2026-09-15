# Software

This folder contains the firmware and software developed for the Portable Emergency Communication Relay Network (PECRN).

The software is organized by development stage, beginning with individual hardware tests and progressing to the complete three-node communication system.

## Software Structure

```text
software/
├── 01_gps_test/
├── 02_oled_test/
├── 03_gps_oled/
├── 04_lora_verification/
├── 05_stm32_field_unit/
├── 06_arduino_relay/
├── 07_base_station/
└── README.md
```

---

# 01 - GPS Test

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

# 02 - OLED Test

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

Status: **Complete**

---

# 03 - GPS + OLED Integration

### Platform

Arduino UNO R4 WiFi

### Purpose

Combine the GPS and OLED subsystems into a standalone GPS display before migration to the STM32 platform.

### Features

- GPS data acquisition
- GPS fix detection
- Latitude and longitude decoding
- Satellite count
- OLED status display
- Real-time coordinate updates

### Result

GPS information was successfully received, decoded, and displayed on the SSD1306 OLED.

This milestone verified that the GPS and display subsystems could operate together.

Status: **Complete**

---

# 04 - LoRa Verification

### Platform

Arduino UNO R4 WiFi + RYLR998

### Purpose

Verify UART communication with the RYLR998 LoRa transceiver before integrating LoRa into the complete PECRN network.

### Features

- RYLR998 UART communication
- AT command interface
- LoRa module configuration
- Point-to-point communication
- Bidirectional communication testing

### Verified Configuration

```text
NETWORKID=18
BAND=915000000
IPR=115200
PARAMETER=9,7,1,12
```

### Result

The RYLR998 successfully responded to AT commands and exchanged wireless packets with another RYLR998 module.

Status: **Complete**

---

# 05 - STM32 Field Unit

### Platform

STM32 NUCLEO-F446RE

### Connected Hardware

- NEO-6M GPS
- SSD1306 OLED
- RYLR998 LoRa module
- B1 SOS push button

### Purpose

The STM32 Field Unit is the primary remote PECRN node.

It collects GPS information, displays local status, generates telemetry and emergency packets, and reliably transmits those packets to the Arduino Relay Node.

### Peripheral Configuration

| Peripheral | Device | Configuration |
|---|---|---|
| UART4 | NEO-6M GPS | 9600 baud |
| USART1 | RYLR998 LoRa | 115200 baud |
| USART2 | PC / Debug | 115200 baud |
| I2C1 | SSD1306 OLED | 100 kHz |
| EXTI | B1 SOS Button | Falling-edge interrupt |

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

SOS
B1 -> PC13
```

### Features

- NMEA GGA parsing
- GPS coordinate validation
- GPS fix detection
- GPS-loss detection
- Last-known position storage
- Periodic GPS telemetry
- Periodic STATUS telemetry
- SOS emergency packets
- OLED status display
- Packet ID generation
- Reliable Field → Relay transmission
- Relay acknowledgement (`RACK`)
- Automatic retransmission
- Interrupt-driven USART1 LoRa reception
- Software UART ring buffer
- SOS traffic priority

### Packet Examples

GPS:

```text
P1,GPS,1,286,47.66769,-122.31281,6
```

STATUS:

```text
P1,STATUS,1,285,GPS_FIX,6
```

SOS:

```text
P1,SOS,1,<packetID>,<latitude>,<longitude>,FIX
```

### Reliable Transmission

After transmitting a packet, the Field Unit waits for:

```text
P1,RACK,1,<packetID>
```

from the Relay Node.

If the expected RACK is not received, the Field Unit retries the packet.

Current configuration:

```text
RACK timeout: 2000 ms
Maximum attempts: 3
Retry delay: 100 ms
```

Status: **Complete**

---

# 06 - Arduino Relay Node

### Platform

Arduino UNO R4 WiFi + RYLR998

### Purpose

The Relay Node provides reliable intermediate storage and forwarding between the STM32 Field Unit and the Base Station.

### Network Address

```text
Relay Address = 0
Field Address = 1
Base Address  = 2
```

### Features

- Field packet reception
- `RACK` acknowledgement to Field Node
- Relay packet queue
- 10-packet queue capacity
- Strict FIFO forwarding for normal telemetry
- SOS packet priority
- Relay → Base retransmission
- Base acknowledgement (`BACK`) handling
- Retry backoff
- Active duplicate detection
- Completed-packet duplicate history
- Queue-full protection
- Radio TX pacing
- Field traffic priority / quiet period

### Field Acknowledgement

After safely storing a Field packet, the Relay sends:

```text
P1,RACK,1,<packetID>
```

The Field Node can then consider the packet accepted by the relay.

### Relay Queue

Packets are retained by the Relay until successful delivery to the Base Station is confirmed.

Normal GPS and STATUS telemetry follows FIFO ordering.

SOS packets receive priority over ordinary telemetry.

### Base Delivery

The Relay forwards the original Field packet to Base Address `2`.

Example:

```text
P1,GPS,1,286,47.66769,-122.31281,6
```

The Relay then waits for:

```text
P1,BACK,1,286
```

If no BACK is received, transmission is retried.

After repeated failures, the packet remains stored and is retried later using backoff rather than being discarded.

Status: **Complete**

---

# 07 - Base Station

### Platform

- macOS
- Python 3
- CP2102 USB-to-UART adapter
- RYLR998 LoRa module

### Purpose

The Base Station is the final destination for PECRN telemetry.

It receives packets forwarded by the Relay Node, parses and displays their contents, and acknowledges successful delivery.

### Network Address

```text
Base Address  = 2
Relay Address = 0
Field Node    = 1
```

### Features

- Serial communication with RYLR998
- PECRN Protocol V1 parsing
- GPS packet processing
- STATUS packet processing
- SOS packet processing
- Packet ID display
- GPS coordinate display
- Satellite count display
- Source-address validation
- Base acknowledgement (`BACK`) transmission

### Source Validation

The Base Station may physically receive direct transmissions from the Field Node because all nodes share the LoRa network.

Direct Field transmissions are intentionally ignored:

```text
[RX] LoRa source 1: P1,GPS,1,286,...
[DROP] Unexpected LoRa source 1
```

Only packets forwarded by Relay Address `0` are processed:

```text
[RX] LoRa source 0: P1,GPS,1,286,47.66769,-122.31281,6
```

### Base Acknowledgement

After successfully processing a packet, the Base Station sends:

```text
P1,BACK,<nodeID>,<packetID>
```

Example:

```text
P1,BACK,1,286
```

The Relay uses this acknowledgement to determine that the packet reached its final destination and can be removed from the queue.

Status: **Complete**

---

# PECRN Protocol V1

The completed system uses a simple application-layer protocol identified by:

```text
P1
```

General packet format:

```text
P1,<TYPE>,<NODE_ID>,<PACKET_ID>,<DATA...>
```

### GPS

```text
P1,GPS,1,<packetID>,<latitude>,<longitude>,<satellites>
```

### STATUS

```text
P1,STATUS,1,<packetID>,<status>,...
```

### SOS

```text
P1,SOS,1,<packetID>,...
```

### Relay Acknowledgement

```text
P1,RACK,1,<packetID>
```

### Base Acknowledgement

```text
P1,BACK,1,<packetID>
```

---

# Reliable Delivery Architecture

PECRN uses acknowledgements at both communication hops.

```text
             GPS / STATUS / SOS
STM32 FIELD ─────────────────────► RELAY
    ▲                                │
    │            RACK                │
    └────────────────────────────────┘
                                     │
                                     │ GPS / STATUS / SOS
                                     ▼
                                  BASE STATION
                                     │
                                     │ BACK
                                     ▼
                                   RELAY
```

### Stage 1 — Field → Relay

1. Field generates a packet with a unique packet ID.
2. Field transmits the packet to Relay Address `0`.
3. Relay validates and stores the packet.
4. Relay sends `RACK`.
5. Field retries if RACK is not received.

### Stage 2 — Relay → Base

1. Relay selects a queued packet.
2. Relay forwards it to Base Address `2`.
3. Base validates and processes the packet.
4. Base sends `BACK`.
5. Relay removes the packet after BACK is received.
6. If BACK is missing, Relay retries and eventually applies retry backoff.

---

# Completed System Architecture

```text
                     PECRN

               STM32 FIELD NODE
              NUCLEO-F446RE
                     │
          ┌──────────┼──────────┐
          │          │          │
         GPS        OLED       SOS
        UART4       I2C1       EXTI
          │          │          │
          └──────────┼──────────┘
                     │
              Packet Generation
                     │
             GPS / STATUS / SOS
                     │
                  USART1
                     │
                 RYLR998
                     │
                     │ LoRa
                     ▼
              ARDUINO RELAY
                Address 0
                     │
              Queue + RACK
                     │
              Retry / Backoff
                     │
                 RYLR998
                     │
                     │ LoRa
                     ▼
               BASE STATION
                 Address 2
                     │
              Python Software
                     │
              Packet Display
                     │
                  BACK
                     │
                     ▼
              ARDUINO RELAY
```

---

# Verified End-to-End Operation

The complete three-node communication chain has been tested successfully.

Example Field telemetry:

```text
P1,GPS,1,286,47.66769,-122.31281,6
```

The Base Station received the forwarded packet from Relay Address `0`:

```text
[RX] LoRa source 0: P1,GPS,1,286,47.66769,-122.31281,6
```

The Base Station decoded the telemetry:

```text
Node 1 | Packet 286
TYPE       : GPS
LATITUDE   : 47.66769
LONGITUDE  : -122.31281
SATELLITES : 6
```

and returned:

```text
[BACK -> RELAY] P1,BACK,1,286
```

This verifies the complete communication path:

```text
GPS
 ↓
STM32 Field Node
 ↓
LoRa
 ↓
Arduino Relay
 ↓
LoRa
 ↓
Base Station
 ↓
BACK
 ↓
Arduino Relay
```

Status: **End-to-End Communication Verified**

---

# Development Progress

| Stage | Description | Status |
|---|---|---|
| 01 | GPS module test | Complete |
| 02 | OLED display test | Complete |
| 03 | GPS + OLED integration | Complete |
| 04 | RYLR998 LoRa verification | Complete |
| 05 | STM32 Field Unit | Complete |
| 06 | Arduino Relay Node | Complete |
| 07 | Python Base Station | Complete |
| 08 | End-to-end acknowledgement protocol | Complete |
| 09 | Relay queue and retry system | Complete |
| 10 | SOS priority | Complete |
| 11 | Battery / portable deployment | Future |

---

# Current Status

The core PECRN communication system is operational.

The current implementation provides:

- GPS telemetry
- System status telemetry
- Emergency SOS messaging
- Three-node LoRa communication
- Field-to-Relay acknowledgements
- Relay-to-Base acknowledgements
- Packet retransmission
- Relay packet buffering
- Duplicate detection
- FIFO telemetry delivery
- SOS priority
- GPS validation
- Local OLED status display

The next stage of development can focus on portable power, enclosure design, expanded field testing, and additional field or relay nodes.
