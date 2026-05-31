# Wiring Notes

## GPS Module to Arduino UNO R4 WiFi

GPS uses UART communication.

| GPS Module | Arduino UNO R4 WiFi |
|---|---|
| VCC | 5V |
| GND | GND |
| TX | RX |
| RX | TX |

Important:
TX and RX are crossed:
GPS TX goes to Arduino RX.
GPS RX goes to Arduino TX.

## OLED Display to Arduino UNO R4 WiFi

OLED uses I2C communication.

| OLED Display | Arduino UNO R4 WiFi |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | SDA |
| SCL | SCL |

## Debugging Order

1. Test GPS alone first.
2. Test OLED alone second.
3. Combine GPS and OLED only after both work separately.
