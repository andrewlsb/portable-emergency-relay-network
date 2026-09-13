# STM32 Field Unit Wiring

This document describes the hardware connections for the STM32 NUCLEO-F446RE field unit used in the Portable Emergency Communication Relay Network.

## Field Unit Hardware

- STM32 NUCLEO-F446RE
- NEO-6M GPS Module
- SSD1306 0.96 inch I2C OLED Display
- RYLR998 LoRa Module
- Breadboard
- Jumper Wires
- USB cable

## System Overview

The STM32 NUCLEO-F446RE acts as the main controller for the field unit.

It receives GPS data from the NEO-6M, displays the current position on the SSD1306 OLED, and transmits the GPS coordinates wirelessly using the RYLR998 LoRa module.

```text
NEO-6M GPS
     │
     │ UART4
     ▼
┌─────────────────────┐
│ STM32 NUCLEO-F446RE │
│                     │
│ GPS Parsing         │
│ Coordinate Display  │
│ LoRa Transmission   │
└──────┬────────┬─────┘
       │        │
     I2C1     USART1
       │        │
       ▼        ▼
   SSD1306    RYLR998
     OLED       LoRa
                  │
                  │ 915 MHz LoRa
                  ▼
               Relay Node
```

---

# NEO-6M GPS Wiring

The NEO-6M communicates with the STM32 using UART4 at 9600 baud.

| NEO-6M Pin | STM32 Pin | STM32 Function |
|---|---|---|
| VCC | 5V | GPS power |
| GND | GND | Common ground |
| TXD | A1 / PA1 | UART4_RX |
| RXD | Not connected | Not currently used |

## UART4 Configuration

```text
GPS TXD -> PA1
PA1 = UART4_RX

Baud Rate = 9600
Word Length = 8 bits
Parity = None
Stop Bits = 1
```

The GPS continuously sends NMEA messages to the STM32.

The current firmware receives GPS data through `UART4_RX`. Since the field unit only needs to receive GPS information, the GPS RXD connection is not currently required.

Example NMEA message:

```text
$GPGGA,043520.00,4740.05797,N,12218.76981,W,1,07,1.82,79.7,M,-18.7,M,,*5A
```

The STM32 extracts latitude, longitude, GPS fix status, and satellite count from the received GPS data.

---

# SSD1306 OLED Wiring

The SSD1306 OLED communicates with the STM32 using I2C1.

| SSD1306 Pin | STM32 Pin | STM32 Function |
|---|---|---|
| VCC | 5V | OLED power |
| GND | GND | Common ground |
| SCL | D15 / PB8 | I2C1_SCL |
| SDA | D14 / PB9 | I2C1_SDA |

## I2C1 Configuration

```text
OLED SCL -> PB8
OLED SDA -> PB9

PB8 = I2C1_SCL
PB9 = I2C1_SDA

Clock Speed = 100000 Hz
OLED Address = 0x3C
```

The OLED displays the GPS fix status, latitude, longitude, and satellite count.

Example display:

```text
GPS FIX

LAT 47.66768
LON -122.31284
SAT 9
```

---

# RYLR998 LoRa Wiring

The RYLR998 communicates with the STM32 using USART1.

| RYLR998 Pin | STM32 Pin | STM32 Function |
|---|---|---|
| VDD | 3.3V | LoRa module power |
| GND | GND | Common ground |
| TXD | PA10 | USART1_RX |
| RXD | PA9 | USART1_TX |

UART connections are crossed:

```text
RYLR998 TXD -> STM32 RX
RYLR998 RXD <- STM32 TX
```

Therefore:

```text
RYLR998 TXD -> PA10 / USART1_RX
RYLR998 RXD -> PA9  / USART1_TX
```

## USART1 Configuration

```text
PA9  = USART1_TX
PA10 = USART1_RX

Baud Rate = 115200
Word Length = 8 bits
Parity = None
Stop Bits = 1
```

## RYLR998 Configuration

The field unit LoRa module is configured as:

```text
ADDRESS=1
NETWORKID=18
BAND=915000000
PARAMETER=9,7,1,12
```

The STM32 generates a GPS payload after receiving a valid GPS fix.

Example payload:

```text
GPS,47.66768,-122.31279
```

The payload is inserted into an RYLR998 `AT+SEND` command.

Example:

```text
AT+SEND=0,23,GPS,47.66768,-122.31279
```

The RYLR998 then transmits the packet wirelessly to the receiving LoRa node.

Example received packet:

```text
+RCV=1,23,GPS,47.66768,-122.31279,-23,12
```

---

# USART2 Debug Connection

USART2 is configured for debugging and serial communication with the development computer.

## USART2 Configuration

```text
Baud Rate = 115200
Word Length = 8 bits
Parity = None
Stop Bits = 1
```

The STM32 NUCLEO-F446RE connects to the development computer through the onboard ST-LINK USB interface.

USART2 can be used to send debugging information to a serial terminal during development.

---

# User Button

The onboard user button is connected to:

```text
PC13 = User Button
```

The button is configured as an external GPIO interrupt.

The firmware uses:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN)
{
    if (GPIO_PIN == GPIO_PIN_13)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

This was used during development to verify GPIO interrupt functionality.

---

# Onboard LED

The STM32 NUCLEO-F446RE onboard LED LD2 is connected to:

```text
PA5 = LD2
```

The LED can be toggled using:

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
```

This provides a simple visual indicator for testing GPIO and interrupt behavior.

---

# Complete Field Unit Pin Map

| Device | Device Pin | STM32 Pin | Peripheral / Function |
|---|---|---|---|
| NEO-6M GPS | VCC | 5V | Power |
| NEO-6M GPS | GND | GND | Ground |
| NEO-6M GPS | TXD | PA1 / A1 | UART4_RX |
| NEO-6M GPS | RXD | Not connected | Not used |
| SSD1306 OLED | VCC | 5V | Power |
| SSD1306 OLED | GND | GND | Ground |
| SSD1306 OLED | SCL | PB8 / D15 | I2C1_SCL |
| SSD1306 OLED | SDA | PB9 / D14 | I2C1_SDA |
| RYLR998 LoRa | VDD | 3.3V | Power |
| RYLR998 LoRa | GND | GND | Ground |
| RYLR998 LoRa | TXD | PA10 | USART1_RX |
| RYLR998 LoRa | RXD | PA9 | USART1_TX |
| User Button | — | PC13 | GPIO EXTI |
| LD2 LED | — | PA5 | GPIO Output |

---

# Communication Interfaces

| Interface | Device | Baud / Speed | Purpose |
|---|---|---|---|
| UART4 | NEO-6M GPS | 9600 baud | Receive GPS NMEA data |
| I2C1 | SSD1306 OLED | 100 kHz | Display GPS information |
| USART1 | RYLR998 LoRa | 115200 baud | LoRa AT commands and data |
| USART2 | PC / ST-LINK | 115200 baud | Debugging |

---

# Data Flow

The complete field unit data path is:

```text
          NEO-6M GPS
               │
               │ NMEA
               │ UART4 @ 9600
               │ PA1
               ▼
     ┌─────────────────────┐
     │ STM32 NUCLEO-F446RE │
     │                     │
     │ 1. Receive NMEA     │
     │ 2. Parse GPGGA      │
     │ 3. Calculate LAT    │
     │ 4. Calculate LON    │
     │ 5. Read satellites  │
     │ 6. Update OLED      │
     │ 7. Build LoRa packet│
     └──────┬────────┬─────┘
            │        │
       I2C1 │        │ USART1
   PB8/PB9  │        │ PA9/PA10
            ▼        ▼
        SSD1306    RYLR998
          OLED       LoRa
                      │
                      │ 915 MHz
                      ▼
                 RYLR998
                 Relay Node
                      │
                      ▼
              Arduino UNO R4 WiFi
                      │
                      ▼
                Serial Monitor
```

---

# Power Distribution

During development, the STM32 NUCLEO-F446RE is powered through USB.

The STM32 and connected modules must share a common ground.

```text
STM32
 │
 ├── 5V ─────► GPS VCC
 │
 ├── 5V ─────► OLED VCC
 │
 ├── 3.3V ───► RYLR998 VDD
 │
 └── GND ─────┬──► GPS GND
              ├──► OLED GND
              └──► RYLR998 GND
```

The breadboard can be used to distribute power and ground to the connected modules.

Do not connect the STM32 3.3V output and 5V output to the same breadboard power rail.

---

# Current Field Unit Operation

The current field unit performs the following sequence:

```text
1. STM32 initializes peripherals
        ↓
2. OLED initializes
        ↓
3. GPS begins sending NMEA data
        ↓
4. STM32 receives GPS data through UART4
        ↓
5. STM32 searches for GPGGA messages
        ↓
6. Latitude and longitude are decoded
        ↓
7. GPS information is displayed on OLED
        ↓
8. STM32 creates GPS LoRa payload
        ↓
9. STM32 sends AT+SEND command through USART1
        ↓
10. RYLR998 transmits GPS packet
        ↓
11. Arduino relay node receives packet
```

---

# Verified Output

The field unit has successfully transmitted real GPS coordinates over LoRa.

Example transmitted GPS data:

```text
GPS,47.66776,-122.31289
```

Example packets received by the Arduino relay node:

```text
+RCV=1,23,GPS,47.66776,-122.31289,-31,11
+RCV=1,23,GPS,47.66776,-122.31289,-41,11
+RCV=1,23,GPS,47.66776,-122.31289,-39,11
+RCV=1,23,GPS,47.66776,-122.31289,-35,11
+RCV=1,23,GPS,47.66775,-122.31289,-35,10
```

This verifies the complete communication path:

```text
GPS
 ↓
STM32
 ↓
RYLR998 Field Unit
 ↓
LoRa Wireless Link
 ↓
RYLR998 Receiver
 ↓
Arduino UNO R4 WiFi
 ↓
Serial Monitor
```

---

# Current Hardware Status

- [x] STM32 NUCLEO-F446RE field unit configured
- [x] NEO-6M connected through UART4
- [x] GPS NMEA data received at 9600 baud
- [x] Valid GPS fix acquired
- [x] Latitude and longitude decoded
- [x] SSD1306 connected through I2C1
- [x] GPS fix displayed on OLED
- [x] Latitude displayed on OLED
- [x] Longitude displayed on OLED
- [x] Satellite count displayed on OLED
- [x] RYLR998 connected through USART1
- [x] STM32 ↔ RYLR998 UART communication verified
- [x] Point-to-point LoRa communication verified
- [x] Bidirectional LoRa communication verified
- [x] GPS coordinates transmitted over LoRa
- [x] Arduino receiver successfully receives GPS packets
- [ ] Third LoRa node integrated
- [ ] Automatic relay forwarding implemented
- [ ] Multi-hop communication implemented
- [ ] Emergency SOS message protocol implemented
- [ ] Battery-powered field deployment

---

# Next Step

The next hardware/software integration milestone is the three-node relay network.

```text
FIELD UNIT
STM32 + GPS + OLED + LoRa
        │
        │ LoRa
        ▼
RELAY NODE
Arduino UNO R4 WiFi + LoRa
        │
        │ LoRa
        ▼
BASE STATION
LoRa + PC
```

The relay node will receive packets from the STM32 field unit and retransmit them to the base station, creating the first multi-hop communication path in the project.
