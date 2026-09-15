# STM32 Field Unit

STM32 NUCLEO-F446RE firmware for the PECRN GPS + OLED + LoRa field unit.

## Hardware

- STM32 NUCLEO-F446RE
- RYLR998 LoRa module
- GPS module
- SSD1306 OLED display
- B1 push button for SOS

## Interfaces

- USART1 — RYLR998 LoRa communication at 115200 baud
- UART4 — GPS communication at 9600 baud
- USART2 — Debug output at 115200 baud
- I2C1 — SSD1306 OLED
- PC13 / B1 — SOS button interrupt

## Features

- GPS NMEA GGA parsing and coordinate validation
- GPS fix and GPS-loss detection
- Periodic GPS and STATUS telemetry
- Last-known GPS position storage
- SOS emergency transmission
- OLED status display
- Reliable Field → Relay communication
- Relay acknowledgement (`RACK`) handling
- Automatic retransmission on missing acknowledgement
- Interrupt-driven USART1 LoRa reception
- Software UART ring buffer
- Packet ID tracking
- SOS traffic priority

## Network

```text
STM32 Field Unit (Address 1)
          |
          | LoRa
          v
Arduino Relay (Address 0)
          |
          | LoRa
          v
Base Station (Address 2)
```

The Field Unit sends `GPS`, `STATUS`, and `SOS` packets to the relay using PECRN Protocol V1 (`P1`).

After receiving a packet, the relay responds with:

```text
P1,RACK,1,<packetID>
```

If the acknowledgement is not received, the Field Unit automatically retries the transmission.

## Files

- `main.c` — Main application firmware
- `Field_Unit.ioc` — STM32CubeMX configuration
- `ssd1306.c/.h` — SSD1306 OLED driver
- `ssd1306_fonts.c/.h` — OLED font data
- `ssd1306_conf.h` — OLED driver configuration
