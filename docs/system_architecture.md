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
