# GPS Test

## Objective

Verify communication between the Arduino UNO R4 WiFi and NEO-6M GPS module.

## Hardware

- Arduino UNO R4 WiFi
- NEO-6M GPS Module

## Wiring

GPS VCC -> Arduino 5V
GPS GND -> Arduino GND
GPS TX -> Arduino RX (Pin 0)
GPS RX -> Arduino TX (Pin 1)

## Results

Successfully acquired GPS fix with 6 satellites.

Example Output:

Latitude: 47.667700
Longitude: -122.313024
Satellites: 6
