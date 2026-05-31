# Portable Emergency Communication Relay Network

A battery-powered embedded communication system designed to provide GPS location tracking and emergency messaging in environments without cellular or internet infrastructure.

## Status

Planning Phase / Milestone 1 Preparation

## Project Goal

Build a portable emergency communication system using an Arduino UNO R4 WiFi, GPS module, OLED display, and future LoRa radio modules. The first milestone is to read live GPS coordinates and display them on an OLED screen.

## Current Milestone

GPS → Arduino UNO R4 WiFi → OLED Display

## Planned Features

- GPS location tracking
- OLED status display
- Emergency SOS message generation
- LoRa wireless communication
- Multi-hop relay forwarding
- Base station dashboard

## Hardware

Current:
- Arduino UNO R4 WiFi
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display

Future:
- LoRa SX1276 915 MHz Radio Modules
- ESP32 Development Boards
- Battery packs
- Enclosures

## Repository Structure

- `/docs` — project plan, BOM, and notes
- `/hardware` — wiring notes and diagrams
- `/software` — Arduino code for each milestone
- `/images` — project photos and screenshots

## Milestones

1. GPS test using Serial Monitor
2. OLED test display
3. GPS data displayed on OLED
4. LoRa message transmission
5. Three-node relay network
6. Battery-powered final prototype
