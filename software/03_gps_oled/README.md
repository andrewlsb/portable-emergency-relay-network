# GPS + OLED Integration

## Objective

Integrate the NEO-6M GPS module and SSD1306 OLED display into a standalone embedded system capable of displaying real-time GPS information.

## Hardware

- Arduino UNO R4 WiFi
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display

## Wiring

### GPS Module

GPS VCC -> Arduino 5V

GPS GND -> Arduino GND

GPS TX -> Arduino RX (Pin 0)

GPS RX -> Arduino TX (Pin 1)

### OLED Display

OLED VCC -> Arduino 5V

OLED GND -> Arduino GND

OLED SDA -> Arduino SDA

OLED SCL -> Arduino SCL

## Features

- Displays GPS fix status
- Displays satellite count
- Displays latitude
- Displays longitude
- Operates without Serial Monitor

## Example Output

GPS STATUS

SAT: 6

FIX: YES

LAT: 47.xxxxxx

LON: -122.xxxxxx

## Result

Successfully integrated GPS and OLED modules into a standalone embedded system. Real-time location data is displayed directly on the OLED screen.
