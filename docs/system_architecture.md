# System Architecture

## System Overview

The Portable Emergency Communication Relay Network (PECRN) is a three-node LoRa communication system designed to transmit GPS telemetry, system status, and emergency SOS information from a remote field unit to a PC-based base station through an intermediate relay.

The current implementation uses a fixed communication path:

```text
STM32 Field Unit
LoRa Address 1
       │
       │ GPS / STATUS / SOS
       ▼
Arduino Relay Node
LoRa Address 0
       │
       │ Forwarded Packet
       ▼
PC Base Station
LoRa Address 2
```

The system implements acknowledgements at both communication stages to improve delivery reliability.

---

# Node A - STM32 Field Unit

## Hardware

- STM32 NUCLEO-F446RE
- NEO-6M GPS module
- SSD1306 OLED display
- RYLR998 LoRa module
- B1 / PC13 SOS push button

## Interfaces

| STM32 Peripheral | Connected Device | Configuration |
|---|---|---|
| UART4 | NEO-6M GPS | 9600 baud |
| USART1 | RYLR998 LoRa | 115200 baud |
| USART2 | PC / Debug | 115200 baud |
| I2C1 | SSD1306 OLED | 100 kHz |
| EXTI | SOS Button | Falling-edge interrupt |

## Responsibilities

- Receive GPS NMEA data
- Parse GGA messages
- Validate GPS coordinates
- Detect GPS fix and GPS loss
- Store last-known valid position
- Display system information on the OLED
- Generate GPS telemetry
- Generate STATUS telemetry
- Generate emergency SOS packets
- Assign packet IDs
- Transmit packets to the Relay Node
- Wait for Relay acknowledgements (`RACK`)
- Retry packets when acknowledgement is not received
- Prioritize SOS transmission

## LoRa Reception

USART1 reception from the RYLR998 is interrupt-driven.

Received bytes are placed into a software ring buffer and processed outside the interrupt handler.

This allows the Field Unit to receive Relay acknowledgements while continuing other system operations.

---

# Node B - Arduino Relay Node

## Hardware

- Arduino UNO R4 WiFi
- RYLR998 LoRa module

## LoRa Address

```text
0
```

## Responsibilities

- Receive packets from the STM32 Field Unit
- Validate PECRN packets
- Detect duplicate packets
- Store accepted packets in a relay queue
- Send `RACK` acknowledgements to the Field Unit
- Forward queued packets to the Base Station
- Wait for Base Station `BACK` acknowledgements
- Retry unconfirmed transmissions
- Apply retry backoff
- Maintain FIFO ordering for normal telemetry
- Prioritize SOS packets
- Maintain completed-packet history
- Protect against queue overflow

## Relay Queue

The Relay Node contains a 10-packet software queue.

Normal `GPS` and `STATUS` packets are forwarded using FIFO ordering.

`SOS` packets receive priority over ordinary telemetry.

A packet remains in the Relay queue until successful Base Station delivery is confirmed with a `BACK` acknowledgement.

---

# Node C - PC Base Station

## Hardware

- PC / macOS
- RYLR998 LoRa module
- CP2102 USB-to-UART adapter

## Software

- Python 3
- `base_station.py`

## LoRa Address

```text
2
```

## Responsibilities

- Receive packets forwarded by the Relay Node
- Validate the LoRa source address
- Parse PECRN Protocol V1 packets
- Process GPS telemetry
- Process STATUS telemetry
- Process SOS emergency messages
- Display packet IDs
- Display GPS coordinates
- Display satellite information
- Send `BACK` acknowledgements to the Relay Node

The Base Station only accepts application packets forwarded from Relay Address `0`.

Direct Field Node transmissions that are physically received by the Base Station radio are intentionally ignored.

---

# PECRN Protocol

The current system uses PECRN Protocol Version 1:

```text
P1
```

General application packet format:

```text
P1,<TYPE>,<NODE_ID>,<PACKET_ID>,<DATA...>
```

## GPS Packet

```text
P1,GPS,1,<packetID>,<latitude>,<longitude>,<satellites>
```

Example:

```text
P1,GPS,1,286,47.66769,-122.31281,6
```

## STATUS Packet

```text
P1,STATUS,1,<packetID>,<status>,...
```

Example:

```text
P1,STATUS,1,285,GPS_FIX,6
```

## SOS Packet

```text
P1,SOS,1,<packetID>,...
```

Depending on GPS availability, an SOS packet can contain the current GPS position, the last-known position, or indicate that GPS information is unavailable.

## Relay Acknowledgement

```text
P1,RACK,1,<packetID>
```

`RACK` confirms that the Relay Node has accepted and stored the Field packet.

## Base Acknowledgement

```text
P1,BACK,1,<packetID>
```

`BACK` confirms that the Base Station successfully received the packet forwarded by the Relay.

---

# Reliable Communication Flow

## Field to Relay

```text
STM32 FIELD                          ARDUINO RELAY

     │                                     │
     │ P1,GPS/STATUS/SOS,...               │
     ├────────────────────────────────────►│
     │                                     │
     │          P1,RACK,1,<ID>             │
     │◄────────────────────────────────────┤
     │                                     │
```

If the Field Unit does not receive the expected `RACK`, it retransmits the packet.

The current Field firmware uses:

```text
RACK timeout     = 2000 ms
Maximum attempts = 3
Retry delay      = 100 ms
```

---

## Relay to Base

```text
ARDUINO RELAY                         BASE STATION

     │                                     │
     │ P1,GPS/STATUS/SOS,...               │
     ├────────────────────────────────────►│
     │                                     │
     │          P1,BACK,1,<ID>             │
     │◄────────────────────────────────────┤
     │                                     │
```

If `BACK` is not received, the Relay retries transmission.

The current Relay firmware uses:

```text
BACK timeout      = 2000 ms
Maximum attempts  = 3
Retry backoff     = 5000 ms
Relay queue       = 10 packets
Completed history = 32 packets
```

Packets that remain unconfirmed after the initial retry attempts are retained for later retransmission.

---

# Duplicate Protection

PECRN uses packet IDs to identify individual messages.

The Relay performs two levels of duplicate detection.

### Active Queue Detection

If the Field retransmits a packet that is already stored in the Relay queue, the Relay does not store another copy.

Instead, it sends the corresponding `RACK` again.

### Completed Packet Detection

The Relay maintains a history of recently completed packets.

If the Field retransmits a packet that has already reached the Base Station, the Relay does not forward it again.

Instead, the Relay repeats the `RACK`.

This prevents a lost acknowledgement from causing unnecessary duplicate delivery to the Base Station.

---

# End-to-End Architecture

```text
                         PECRN

                 ┌─────────────────┐
                 │   NEO-6M GPS    │
                 └────────┬────────┘
                          │ UART4
                          ▼
                ┌───────────────────┐
                │   STM32 FIELD     │
                │ NUCLEO-F446RE     │
                │    Address 1      │
                │                   │
                │ GPS Parser        │
                │ OLED Display      │
                │ SOS Input         │
                │ Packet Generator  │
                └─────────┬─────────┘
                          │ USART1
                          ▼
                     ┌─────────┐
                     │ RYLR998 │
                     └────┬────┘
                          │
                          │ LoRa
                          ▼
                ┌───────────────────┐
                │  ARDUINO RELAY    │
                │    Address 0      │
                │                   │
                │ Queue             │
                │ Duplicate Check   │
                │ Retry / Backoff   │
                │ SOS Priority      │
                └─────────┬─────────┘
                          │
                          │ LoRa
                          ▼
                     ┌─────────┐
                     │ RYLR998 │
                     └────┬────┘
                          │
                          ▼
                ┌───────────────────┐
                │   BASE STATION    │
                │    Address 2      │
                │                   │
                │ Python Software   │
                │ Packet Parser     │
                │ Terminal Output   │
                └───────────────────┘
```

---

# Verified Communication Path

The complete three-node communication path has been verified:

```text
GPS
 │
 ▼
STM32 FIELD
 │
 │ Packet
 ▼
ARDUINO RELAY
 │
 │ RACK → Field
 │
 │ Forwarded Packet
 ▼
BASE STATION
 │
 │ BACK
 ▼
ARDUINO RELAY
```

Example successfully delivered telemetry:

```text
P1,GPS,1,286,47.66769,-122.31281,6
```

The Base Station receives the packet from Relay Address `0`, displays the decoded GPS information, and returns:

```text
P1,BACK,1,286
```

The Relay then marks the packet as successfully delivered and removes it from its active queue.

---

# Current Architecture Status

The current PECRN prototype implements a fixed three-node relay topology:

```text
Field Node → Relay Node → Base Station
```

The architecture currently supports:

- GPS telemetry
- STATUS telemetry
- SOS emergency messages
- Packet identification
- Field-to-Relay acknowledgement
- Relay-to-Base acknowledgement
- Automatic retransmission
- Relay packet buffering
- FIFO telemetry delivery
- SOS priority
- Duplicate detection
- Retry backoff
- GPS validation
- Last-known GPS position
- Local OLED status display

Dynamic routing between multiple relay nodes is not currently implemented and remains a possible future expansion.
