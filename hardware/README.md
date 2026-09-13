# Hardware Documentation

This folder contains wiring diagrams, connection tables, and hardware notes for the Portable Emergency Communication Relay Network.

## Current Hardware

- STM32 NUCLEO-F446RE

- Arduino UNO R4 WiFi

- NEO-6M GPS Module

- SSD1306 0.96 inch I2C OLED Display

- RYLR998 LoRa Modules (3)

- Breadboards

- Jumper Wires

- USB Cables

## Current System Architecture

### Node A - STM32 Field Unit

The field unit collects GPS location data, displays system status locally, and transmits GPS coordinates over LoRa.

#### Hardware

- STM32 NUCLEO-F446RE

- NEO-6M GPS Module

- SSD1306 0.96 inch I2C OLED Display

- RYLR998 LoRa Module

- Breadboard and jumper wires

- USB power during development

#### Interfaces

- NEO-6M GPS → STM32 using UART4

- SSD1306 OLED → STM32 using I2C1

- RYLR998 LoRa → STM32 using USART1

- STM32 → PC debugging using USART2

#### Current Data Flow

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
