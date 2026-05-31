# Project Plan

## Project Name

Portable Emergency Communication Relay Network

## Purpose

The purpose of this project is to design and build a portable embedded communication system that can transmit GPS location and emergency status information when normal infrastructure such as cellular networks, WiFi, or internet access is unavailable.

## Phase 1: GPS to Arduino

Goal:
Read GPS coordinates from the NEO-6M GPS module and print them to the Arduino Serial Monitor.

Success Criteria:
- Arduino receives GPS data
- Latitude and longitude are printed
- Satellite count is displayed

## Phase 2: OLED Display

Goal:
Display basic text on the SSD1306 OLED display.

Success Criteria:
- OLED turns on
- OLED displays test message
- Arduino communicates with OLED using I2C

## Phase 3: GPS to OLED

Goal:
Display GPS coordinates directly on the OLED screen.

Success Criteria:
- OLED displays latitude
- OLED displays longitude
- OLED displays satellite count or GPS fix status

## Phase 4: LoRa Communication

Goal:
Transmit GPS data wirelessly between two nodes.

Success Criteria:
- Node A sends a packet
- Node B receives the packet
- Message is displayed on the receiver

## Phase 5: Relay Network

Goal:
Add a third node that forwards packets between the field unit and base station.

Success Criteria:
- Relay node receives and forwards packets
- Base station receives packets through the relay
- Duplicate packets are filtered

## Phase 6: Final Prototype

Goal:
Build a battery-powered version with enclosure and documentation.

Success Criteria:
- System operates portably
- Wiring is organized
- GitHub repository includes code, photos, and documentation
