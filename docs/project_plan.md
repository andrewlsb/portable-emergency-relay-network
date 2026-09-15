# Project Plan

## Project Name

Portable Emergency Communication Relay Network (PECRN)

## Purpose

The purpose of this project is to design and build a portable embedded communication system capable of transmitting GPS location, system status, and emergency information when normal infrastructure such as cellular networks, WiFi, or internet access is unavailable.

The system uses LoRa communication between a remote STM32 field unit, an Arduino relay node, and a PC-based base station.

---

## Phase 1: GPS Verification

### Goal

Read GPS data from the NEO-6M GPS module and verify that valid location information can be decoded.

### Success Criteria

- GPS data is received through UART
- Valid GPS fix is detected
- Latitude and longitude are decoded
- Satellite count is displayed

### Status

**Complete**

---

## Phase 2: OLED Verification

### Goal

Verify communication with the SSD1306 OLED display.

### Success Criteria

- OLED initializes successfully
- Test messages are displayed
- I2C communication operates correctly

### Status

**Complete**

---

## Phase 3: GPS + OLED Integration

### Goal

Combine GPS acquisition and the OLED display before migration to the STM32 field platform.

### Success Criteria

- GPS position is acquired
- Latitude and longitude are displayed
- Satellite count and GPS status are displayed
- Display updates with current GPS information

### Status

**Complete**

---

## Phase 4: STM32 Field Unit + LoRa

### Goal

Integrate the GPS, OLED, LoRa transceiver, and SOS input with the STM32 NUCLEO-F446RE.

### Success Criteria

- STM32 receives GPS data through UART4
- GPS NMEA data is parsed and validated
- SSD1306 operates through I2C
- RYLR998 operates through USART1
- GPS and STATUS packets are generated
- SOS packets can be generated using the emergency button
- Field packets are transmitted to the relay
- USART1 LoRa reception operates using interrupts

### Status

**Complete**

---

## Phase 5: Reliable Three-Node Relay Network

### Goal

Create a reliable communication path from the STM32 Field Unit through the Arduino Relay Node to the PC-based Base Station.

### Architecture

```text
STM32 Field Unit
Address 1
      │
      │ GPS / STATUS / SOS
      ▼
Arduino Relay
Address 0
      │
      │ Forwarded packet
      ▼
Base Station
Address 2
```

### Success Criteria

- Relay receives Field Node packets
- Relay acknowledges stored packets using `RACK`
- Field Node retries packets when `RACK` is not received
- Relay stores packets in a queue
- Normal telemetry follows FIFO ordering
- SOS traffic receives priority
- Relay forwards packets to the Base Station
- Base Station processes forwarded packets
- Base Station acknowledges packets using `BACK`
- Relay retries packets when `BACK` is not received
- Retry backoff prevents continuous retransmission
- Active duplicate packets are detected
- Previously completed packets are detected
- Successfully delivered packets are removed from the relay queue

### Status

**Complete**

---

## Phase 6: End-to-End Protocol Verification

### Goal

Verify reliable operation of the complete PECRN communication protocol.

### Packet Types

```text
P1,GPS,<nodeID>,<packetID>,...
P1,STATUS,<nodeID>,<packetID>,...
P1,SOS,<nodeID>,<packetID>,...
P1,RACK,<nodeID>,<packetID>
P1,BACK,<nodeID>,<packetID>
```

### Success Criteria

- GPS telemetry reaches the Base Station
- STATUS telemetry reaches the Base Station
- SOS messages can be transported through the relay
- Every packet contains a packet ID
- Field-to-Relay acknowledgement operates correctly
- Relay-to-Base acknowledgement operates correctly
- Duplicate forwarding is prevented
- Packet loss triggers retransmission
- Base Station displays decoded packet information

### Status

**Complete**

---

## Phase 7: Portable Prototype

### Goal

Convert the working communication system into a portable field prototype.

### Planned Work

- Battery-powered operation
- Portable power management
- Organized wiring
- Field Unit enclosure
- Relay Node enclosure
- Extended outdoor range testing
- Packet delivery testing at increased distances
- Power-consumption measurements

### Status

**Future Work**

---

## Phase 8: System Expansion

### Goal

Extend PECRN beyond the initial three-node prototype.

### Potential Improvements

- Additional Field Nodes
- Additional Relay Nodes
- Multi-hop routing
- Automatic route discovery
- Improved packet scheduling
- Battery monitoring
- Environmental sensors
- Base Station logging
- Mapping received GPS coordinates
- Extended deployment testing

### Status

**Future Work**

---

# Current Project Status

The core three-node PECRN communication system is operational.

```text
GPS / SOS
    │
    ▼
STM32 FIELD NODE
    │
    │ LoRa
    ▼
ARDUINO RELAY
    │
    │ LoRa
    ▼
BASE STATION
```

Reliable delivery is implemented at both communication stages:

```text
FIELD ─── Packet ───► RELAY
FIELD ◄─── RACK ───── RELAY

RELAY ─── Packet ───► BASE
RELAY ◄─── BACK ───── BASE
```

The current prototype supports:

- GPS telemetry
- STATUS telemetry
- Emergency SOS packets
- Packet IDs
- Field-to-Relay acknowledgements
- Relay-to-Base acknowledgements
- Automatic retransmission
- Relay packet buffering
- FIFO telemetry delivery
- SOS priority
- Duplicate packet detection
- Retry backoff
- GPS validation and last-known-position handling
- OLED field status display

The next major development stage is converting the working bench prototype into a battery-powered portable system and performing extended field testing.
