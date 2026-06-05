# Project Plan

## Project Name

Portable Emergency Communication Relay Network

## Purpose

The purpose of this project is to design and build a portable embedded communication system that can transmit GPS location and emergency status information when normal infrastructure such as cellular networks, WiFi, or internet access is unavailable.

## Phase 1: GPS to Arduino

### Goal

Read GPS coordinates from the NEO-6M GPS module and print them to the Arduino Serial Monitor.

### Success Criteria

- Arduino receives GPS data
- Latitude and longitude are printed
- Satellite count is displayed

---

## Phase 2: OLED Display

### Goal

Display basic text on the SSD1306 OLED display.

### Success Criteria

- OLED turns on
- OLED displays test message
- Arduino communicates with OLED using I2C

---

## Phase 3: GPS to OLED

### Goal

Display GPS coordinates directly on the OLED screen.

### Success Criteria

- OLED displays latitude
- OLED displays longitude
- OLED displays satellite count or GPS fix status

---

## Phase 4: LoRa Communication

### Goal

Transmit GPS and emergency status data wirelessly between a field unit and relay node.

### Success Criteria

- STM32 field unit sends a packet
- Arduino UNO R4 relay node receives the packet
- GPS data and status information are successfully transmitted over LoRa

---

## Phase 5: Relay Network

### Goal

Forward packets from the field unit through the relay node to a PC-based base station.

### Success Criteria

- Relay node receives packets from the field unit
- Relay node forwards packets to the base station
- Base station receives packets through the relay
- Duplicate packets are filtered

---

## Phase 6: Final Prototype

### Goal

Build a battery-powered version with enclosure and documentation.

### Success Criteria

- System operates portably
- Wiring is organized
- GitHub repository includes code, photos, and documentation
- SOS messages include GPS coordinates
- Base station successfully displays received emergency messages
