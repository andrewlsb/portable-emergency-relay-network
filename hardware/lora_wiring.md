# RYLR998 LoRa Wiring

## Components

- Arduino UNO R4 WiFi
- RYLR998 LoRa Module

## Connections

| RYLR998 | Arduino UNO R4 WiFi |
|----------|----------|
| VDD | 3.3V |
| GND | GND |
| TXD | D0 (RX1) |
| RXD | D1 (TX1) |
| RST | Not Connected |

## Notes

- UART communication is used between the Arduino and LoRa module.
- Serial Monitor baud rate: 115200
- LoRa module baud rate: 115200
- AT commands are used for module configuration and testing.

## Verification

Verified using:

AT

Response:

+OK

Additional configuration verified:

+ADDRESS=0

+NETWORKID=18

+BAND=915000000
