# PECRN Base Station

This folder contains the PC-side base station software for the Portable Emergency Communication Relay Network.

## Platform

- macOS
- Python 3
- CP2102 USB-to-UART adapter
- RYLR998 LoRa Module

## Node Address

The base station LoRa module will use:

```text
ADDRESS=2
NETWORKID=18
BAND=915000000
PARAMETER=9,7,1,12
UART=115200
