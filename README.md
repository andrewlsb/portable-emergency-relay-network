# Portable Emergency Communication Relay Network

A battery-powered embedded communication system designed to provide GPS location tracking and emergency messaging in environments without cellular or internet infrastructure.

## Status

- Milestone 1 Complete
- Milestone 2 Complete
- Milestone 3 Complete
- Milestone 4 Complete
- Current Milestone: 5

# Progress

- [x] GPS module integration
- [x] UART communication verified
- [x] GPS fix acquired
- [x] OLED display integration
- [x] GPS coordinates displayed on OLED
- [x] LoRa module verification
- [x] STM32 ↔ RYLR998 UART communication
- [x] Point-to-point LoRa communication
- [x] STM32 field unit → Arduino receiver wireless transmission
- [x] Bidirectional LoRa communication
- [x] GPS coordinate transmission over LoRa
- [ ] Multi-hop relay network
- [ ] Emergency message protocol
- [ ] Battery-powered deployment

## Project Goal

Build a portable emergency communication network capable of transmitting GPS coordinates and emergency SOS messages in environments without cellular or internet infrastructure.

The system uses GPS-enabled field nodes, LoRa wireless communication, relay nodes, and a base station to deliver emergency location information over a multi-hop network.

## Documentation

- Project Plan: `docs/project_plan.md`
- System Architecture: `docs/system_architecture.md`

## Current Milestone

**Current Task: Milestone 5 - Three-Node Relay Network**

The next stage of development will introduce a third LoRa node capable of receiving packets from the field unit and forwarding them toward a base station.

## Planned Features

- GPS location tracking
- OLED status display
- Emergency SOS message generation
- LoRa wireless communication
- Multi-hop relay forwarding
- PC-based base station dashboard
- Battery-powered field operation

## Hardware

### Current

- Arduino UNO R4 WiFi
- STM32 NUCLEO-F446RE
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display
- RYLR998 LoRa Modules (3)

### Future

- Battery Packs
- Enclosures

## Repository Structure

- `/docs` — project plan, BOM, architecture, and notes
- `/hardware` — wiring notes and diagrams
- `/software` — embedded firmware and test programs
- `/images` — project photos and screenshots

## Milestones

1. GPS test using Serial Monitor
2. OLED test display
3. GPS data displayed on OLED
4. LoRa communication and STM32 field node integration
5. Three-node relay network
6. Battery-powered final prototype

---

# Milestone 1 - GPS Integration

## Objective

Integrate the NEO-6M GPS module with the Arduino UNO R4 WiFi and verify location tracking.

## Results

- GPS communication established using UART
- TinyGPSPlus successfully decoded NMEA messages
- GPS fix successfully acquired
- Real-time latitude and longitude displayed in Serial Monitor

## Example Output

```text
Latitude: 47.667700
Longitude: -122.313024
Satellites: 6
```

## Evidence

![GPS Fix](images/milestone1_gps_fix.png)

---

# Milestone 2 - OLED Display Integration

## Objective

Integrate the SSD1306 OLED display with the Arduino UNO R4 WiFi and verify display functionality.

## Results

- OLED display successfully initialized
- I2C communication established using SDA and SCL
- Text successfully displayed on OLED screen
- Adafruit SSD1306 and GFX libraries integrated successfully

## Example Output

```text
HELLO
```

## Evidence

![OLED Test](images/milestone2_oled_hello.png)

---

# Milestone 3 - GPS and OLED Integration

## Objective

Integrate the NEO-6M GPS module and SSD1306 OLED display into a standalone embedded system.

## Results

- GPS data successfully displayed on OLED screen
- Real-time latitude and longitude updates verified
- Satellite count displayed on OLED
- GPS fix status displayed on OLED
- Standalone operation achieved without Serial Monitor

## Example Output

```text
GPS STATUS
SAT: 6
FIX: YES
LAT: 47.xxxxxx
LON: -122.xxxxxx
```

## Evidence

<img src="images/milestone3_gps_oled.png" width="450">

---

# Milestone 4 - LoRa Communication and STM32 Field Unit

## Objective

Develop an STM32-based field unit capable of acquiring GPS coordinates, displaying location information locally, and transmitting the coordinates wirelessly using RYLR998 LoRa transceivers.

The field unit uses the STM32 NUCLEO-F446RE as the primary microcontroller and communicates with the GPS, OLED display, and LoRa transceiver through multiple embedded communication interfaces.

## Results

- Successfully connected the RYLR998 LoRa module to the STM32 NUCLEO-F446RE
- Verified UART communication between the STM32 and RYLR998
- Configured RYLR998 modules for point-to-point LoRa communication
- Integrated the NEO-6M GPS module with the STM32
- Received NMEA GPS sentences through UART
- Parsed GPS latitude, longitude, fix status, and satellite count on the STM32
- Integrated the SSD1306 OLED display using I2C
- Displayed live GPS information on the OLED
- Formatted GPS coordinates into LoRa data packets
- Successfully transmitted live GPS coordinates from the STM32 field unit
- Successfully received LoRa GPS packets on an Arduino UNO R4 WiFi receiving node
- Verified continuous wireless GPS coordinate updates

## Field Unit Architecture

The STM32 field unit combines GPS acquisition, local display, and wireless transmission.

```text
NEO-6M GPS
     │
     │ UART
     ▼
STM32 NUCLEO-F446RE
     │
     ├──── I2C ────► SSD1306 OLED
     │
     └──── UART ───► RYLR998 LoRa
                          │
                          │ Wireless LoRa
                          ▼
                     RYLR998 LoRa
                          │
                          │ UART
                          ▼
                  Arduino UNO R4 WiFi
                          │
                          ▼
                     Serial Monitor
```

## Communication Interfaces

The STM32 field unit currently uses:

- **UART4** — NEO-6M GPS communication
- **I2C1** — SSD1306 OLED communication
- **USART1** — RYLR998 LoRa communication
- **USART2** — debugging / PC serial communication

## GPS Processing

The NEO-6M sends standard NMEA GPS sentences to the STM32.

The firmware identifies GGA sentences and extracts:

- Latitude
- Longitude
- GPS fix status
- Satellite count

The NMEA coordinates are converted from degrees/minutes format into decimal degrees before transmission.

Example processed coordinates:

```text
Latitude: 47.66768
Longitude: -122.31279
```

## OLED Display

After acquiring a valid GPS fix, the STM32 displays the current position and satellite information on the SSD1306 OLED.

Example:

```text
GPS FIX

LAT 47.66768
LON -122.31284
SAT 9
```

## LoRa Packet Format

After obtaining a valid GPS position, the STM32 creates a payload containing the coordinates.

Example payload:

```text
GPS,47.66768,-122.31279
```

The payload is transmitted through the RYLR998 using its AT command interface.

Conceptually:

```text
GPS DATA
   ↓
STM32 parses coordinates
   ↓
GPS,47.66768,-122.31279
   ↓
RYLR998 transmitter
   ↓
~~~~ LoRa wireless link ~~~~
   ↓
RYLR998 receiver
   ↓
Arduino UNO R4 WiFi
```

## Example Received Data

The Arduino receiving node successfully received continuously updated GPS packets:

```text
+RCV=1,23,GPS,47.66768,-122.31279,-25,11
+RCV=1,23,GPS,47.66768,-122.31279,-23,12
+RCV=1,23,GPS,47.66768,-122.31279,-23,12
+RCV=1,23,GPS,47.66768,-122.31279,-22,11
+RCV=1,23,GPS,47.66768,-122.31279,-22,11
```

The RYLR998 receive response provides the sender address, payload length, received GPS payload, RSSI, and SNR.

---

## Milestone 4 Evidence

### LoRa Module Verification

<img src="images/milestone4_lora_setup.png" width="700">

The RYLR998 LoRa module successfully responded to AT commands during initial testing.

The module address, network ID, operating frequency, baud rate, and LoRa communication parameters were verified before integration with the STM32 field unit.

Verified configuration included:

```text
+OK
+ADDRESS=0
+NETWORKID=18
+BAND=915000000
+IPR=115200
+PARAMETER=9,7,1,12
```

### STM32 Field Unit Prototype

<img src="images/milestone4_stm32_field_unit.png" width="600">

The STM32 NUCLEO-F446RE field unit integrates the NEO-6M GPS module, SSD1306 OLED display, and RYLR998 LoRa transceiver.

The prototype demonstrates the hardware configuration used to acquire GPS information and transmit it wirelessly.

### Live GPS Display

<img src="images/milestone4_gps_oled.png" width="450">

The SSD1306 OLED displays live GPS information processed by the STM32, including GPS fix status, latitude, longitude, and satellite count.

### Wireless GPS Reception

<img src="images/milestone4_gps_lora_received.png" width="750">

Live GPS coordinates transmitted by the STM32 field unit were successfully received wirelessly by the Arduino UNO R4 WiFi receiving node through a second RYLR998 LoRa module.

Multiple consecutive packets confirm continuous GPS transmission rather than a single test message.

## Milestone 4 Outcome

Milestone 4 establishes a complete end-to-end embedded wireless communication path:

```text
GPS Acquisition
      ↓
STM32 GPS Parsing
      ↓
OLED Status Display
      ↓
LoRa Packet Generation
      ↓
RYLR998 Transmission
      ↓
Wireless LoRa Link
      ↓
RYLR998 Reception
      ↓
Arduino Receiving Node
```

The successful transmission of live GPS coordinates demonstrates that the field node can collect real-world sensor data, process it locally, and transmit it to another embedded node without cellular or internet infrastructure.

This provides the foundation for **Milestone 5**, where a third LoRa node will be introduced to create a multi-hop relay network.

---

# Milestone 5 - Three-Node Relay Network

## Objective

Extend the point-to-point LoRa communication system into a three-node network capable of forwarding messages through an intermediate relay.

### Planned Architecture

```text
FIELD NODE
STM32 + GPS + OLED + LoRa
          │
          │ LoRa
          ▼
     RELAY NODE
   MCU + RYLR998
          │
          │ LoRa
          ▼
     BASE STATION
   MCU + RYLR998
          │
          ▼
          PC
```

### Planned Tasks

- [ ] Configure third RYLR998 LoRa module
- [ ] Assign unique node addresses
- [ ] Implement relay packet reception
- [ ] Implement packet forwarding
- [ ] Define packet structure
- [ ] Add source and destination identifiers
- [ ] Prevent duplicate packet forwarding
- [ ] Verify Field → Relay → Base communication
- [ ] Test GPS coordinate forwarding
- [ ] Measure RSSI and communication reliability

---

# Future Development

After completing the three-node relay network, development will continue toward:

- Emergency SOS message generation
- Message identifiers and duplicate detection
- Multi-hop routing logic
- Packet acknowledgments
- Communication reliability testing
- Range testing
- Packet delivery ratio measurements
- Battery power management
- Portable enclosures
- PC-based emergency monitoring interface

The final goal is a portable network of embedded nodes capable of forwarding emergency location information across areas where conventional cellular or internet communication is unavailable.
