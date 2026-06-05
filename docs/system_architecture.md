# System Architecture

## Final System Design

```text
STM32 + GPS + OLED
(Field Unit)
        │
        ▼
      LoRa
        │
        ▼
UNO R4 Relay Node
        │
        ▼
      LoRa
        │
        ▼
PC Base Station

Node A - Field Unit

Hardware:

* STM32 Nucleo Development Board
* NEO-6M GPS Module
* SSD1306 OLED Display
* RYLR998 LoRa Module
* Battery Pack

Responsibilities:

* Acquire GPS coordinates
* Display status information
* Generate SOS messages
* Transmit LoRa packets

Node B - Relay Node

Hardware:

* Arduino UNO R4 WiFi
* RYLR998 LoRa Module
* Battery Pack

Responsibilities:

* Receive LoRa packets
* Forward packets to the base station
* Filter duplicate messages

Node C - Base Station

Hardware:

* PC
* RYLR998 LoRa Module
* USB-to-UART Adapter

Responsibilities:

* Receive emergency messages
* Display GPS coordinates
* Log incoming packets
