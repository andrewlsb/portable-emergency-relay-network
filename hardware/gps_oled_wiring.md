# GPS + OLED Wiring

## Components

- Arduino UNO R4 WiFi
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display

## GPS Connections

GPS VCC -> Arduino 5V

GPS GND -> Arduino GND

GPS TX -> Arduino RX (Pin 0)

GPS RX -> Arduino TX (Pin 1)

## OLED Connections

OLED VCC -> Arduino 5V

OLED GND -> Arduino GND

OLED SDA -> Arduino SDA

OLED SCL -> Arduino SCL

## Power Distribution

Arduino 5V is connected to the breadboard power rail.

GPS VCC and OLED VCC share the same 5V rail.

Arduino GND is connected to the breadboard ground rail.

GPS GND and OLED GND share the same ground rail.

## System Architecture

NEO-6M GPS
    ↓ UART
Arduino UNO R4 WiFi
    ↓ I2C
SSD1306 OLED Display
