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
- [x] GPS fix acquired (6 satellites)
- [x] OLED display integration
- [x] GPS coordinates displayed on OLED
- [x] LoRa module verification
- [x] STM32 ↔ RYLR998 UART communication
- [x] Point-to-point LoRa communication
- [x] STM32 field unit → Arduino relay wireless transmission
- [x] Bidirectional LoRa communication
- [x] GPS coordinate transmission over LoRa
- [ ] Multi-hop relay network
- [ ] Emergency message protocol
- [ ] Battery-powered deployment

## Project Goal

Build a portable emergency communication network capable of transmitting GPS coordinates and emergency SOS messages in environments without cellular or internet infrastructure. The system uses GPS-enabled field nodes, LoRa wireless communication, relay nodes, and a base station to deliver emergency location information over a multi-hop network.

## Documentation

- Project Plan: docs/project_plan.md
- System Architecture: docs/system_architecture.md

## Current Milestone

Current Task: Milestone 5 - Three-Node Relay Network

## Planned Features

- GPS location tracking
- OLED status display
- Emergency SOS message generation
- LoRa wireless communication
- Multi-hop relay forwarding
- PC-based base station dashboard
- Battery-powered field operation

### Hardware

#### Current

- Arduino UNO R4 WiFi
- STM32 NUCLEO-F446RE
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display
- RYLR998 LoRa Modules (3)

#### Future

- Battery Packs
- Enclosures

## Repository Structure

- `/docs` — project plan, BOM, and notes
- `/hardware` — wiring notes and diagrams
- `/software` — STM32 and Arduino firmware for each milestone
- `/images` — project photos and screenshots

## Milestones

1. GPS test using Serial Monitor
2. OLED test display
3. GPS data displayed on OLED
4. LoRa communication and STM32 field node integration
5. Three-node relay network
6. Battery-powered final prototype

## Milestone 1 - GPS Integration

### Objective
Integrate the NEO-6M GPS module with the Arduino UNO R4 WiFi and verify location tracking.

### Results
- GPS communication established using UART
- TinyGPSPlus successfully decoded NMEA messages
- GPS fix acquired with 6 satellites
- Real-time latitude and longitude displayed in Serial Monitor

### Example Output

Latitude: 47.667700  
Longitude: -122.313024  
Satellites: 6

### Evidence

![GPS Fix](images/milestone1_gps_fix.png)

## Milestone 2 - OLED Display Integration

### Objective
Integrate the SSD1306 OLED display with the Arduino UNO R4 WiFi and verify display functionality.

### Results
- OLED display successfully initialized
- I2C communication established using SDA and SCL
- Text successfully displayed on OLED screen
- Adafruit SSD1306 and GFX libraries integrated successfully

### Example Output

HELLO

### Evidence

![OLED Test](images/milestone2_oled_hello.png)

## Milestone 3 - GPS and OLED Integration

### Objective
Integrate the NEO-6M GPS module and SSD1306 OLED display into a standalone embedded system.

### Results
- GPS data successfully displayed on OLED screen
- Real-time latitude and longitude updates verified
- Satellite count displayed on OLED
- GPS fix status displayed on OLED
- Standalone operation achieved without Serial Monitor

### Example Output

GPS STATUS  
SAT: 6  
FIX: YES  
LAT: 47.xxxxxx  
LON: -122.xxxxxx

### Evidence

![GPS OLED](images/milestone3_gps_oled.png)

## Milestone 4 - LoRa Communication and STM32 Field Node Integration

### Objective

Integrate the RYLR998 LoRa modules and establish wireless communication between nodes. Migrate the field unit to the STM32 NUCLEO-F446RE and integrate GPS acquisition, OLED output, and LoRa transmission into a single embedded field node.

### Results

- Successfully connected RYLR998 to the Arduino UNO R4 WiFi
- Verified RYLR998 UART communication using AT commands
- Confirmed module address configuration
- Confirmed network ID configuration
- Confirmed operation on the 915 MHz frequency band
- Verified 115200 baud rate configuration
- Established point-to-point LoRa communication
- Established bidirectional LoRa communication
- Integrated RYLR998 with STM32 using USART1
- Integrated NEO-6M GPS with STM32 using UART4 at 9600 baud
- Received GPS NMEA data directly on STM32
- Parsed GPGGA GPS messages on STM32
- Extracted latitude, longitude, GPS fix status, and satellite count
- Converted NMEA coordinates into decimal degrees
- Integrated SSD1306 OLED with STM32 using I2C
- Displayed live GPS coordinates on the STM32 field unit
- Transmitted live GPS coordinates from STM32 over LoRa
- Successfully received STM32 GPS packets on the Arduino receiver

### Evidence

#### LoRa Module Verification

![LoRa Verification](images/milestone4_lora_setup.png)

The RYLR998 LoRa module successfully responded to multiple AT commands. The module address, network ID, operating frequency, and baud rate were verified, confirming successful UART communication between the Arduino UNO R4 WiFi and the LoRa transceiver.

Verified Output:

+OK

+ADDRESS=0

+NETWORKID=18

+BAND=915000000

+IPR=115200

+PARAMETER=9,7,1,12

### STM32 Field Node

The STM32 NUCLEO-F446RE now serves as the main field unit.

The field node receives raw NMEA data from the NEO-6M GPS module through UART4. The STM32 identifies GPGGA messages and extracts latitude, longitude, GPS fix status, and satellite count.

The coordinates are converted from NMEA format into decimal degrees and displayed locally on the SSD1306 OLED.

The same coordinates are then formatted into a LoRa payload and transmitted through the RYLR998 connected to USART1.

### Current Field Node Architecture

NEO-6M GPS  
↓ UART4  

STM32 NUCLEO-F446RE  

↓ I2C → SSD1306 OLED  

↓ USART1  

RYLR998 LoRa  

↓ Wireless LoRa  

RYLR998 LoRa  

↓ UART  

Arduino UNO R4 WiFi  

↓ USB  

Serial Monitor

### GPS Coordinate Transmission

The STM32 field unit formats the GPS coordinates into a simple LoRa payload.

Example transmitted payload:

GPS,47.66776,-122.31289

Example received packets:

+RCV=1,23,GPS,47.66776,-122.31289,-41,11

+RCV=1,23,GPS,47.66776,-122.31289,-39,11

+RCV=1,23,GPS,47.66776,-122.31289,-35,11

+RCV=1,23,GPS,47.66775,-122.31289,-35,10

This demonstrates successful end-to-end communication:

GPS → STM32 → LoRa → Wireless Link → LoRa Receiver → Arduino

### Milestone 4 Outcome

A functional STM32-based field node was successfully developed.

The field unit can:

- Acquire a GPS fix
- Receive GPS data over UART
- Parse NMEA GPS messages
- Calculate decimal latitude and longitude
- Display GPS information on the OLED
- Communicate with the RYLR998 over UART
- Transmit GPS coordinates wirelessly using LoRa
- Receive messages from another LoRa node

Milestone 4 establishes the communication and embedded hardware foundation required for the multi-hop relay network.

## Milestone 5 - Three-Node Relay Network

### Objective

Develop a three-node LoRa network capable of forwarding GPS and emergency messages across multiple wireless hops.

### Planned Work

- Define a structured packet format
- Assign unique node IDs
- Add packet IDs
- Implement relay forwarding
- Implement duplicate packet detection
- Prevent relay loops
- Test Field Node → Relay Node → Base Station communication
- Verify GPS coordinate transmission across multiple hops
- Begin emergency SOS message implementation
