# PECRN Base Station

PC-side base station software for the Portable Emergency Communication Relay Network (PECRN).

The Base Station receives telemetry forwarded by the Arduino Relay Node, displays the received data in the terminal, and sends acknowledgements back to the relay.

## Platform

- macOS
- Python 3
- CP2102 USB-to-UART adapter
- RYLR998 LoRa module

## Network Configuration

- Base Station LoRa Address: `2`
- Relay LoRa Address: `0`
- Field Node ID: `1`
- Protocol Version: `P1`

RYLR998 configuration:

```text
ADDRESS=2
NETWORKID=18
BAND=915000000
PARAMETER=9,7,1,12
UART=115200
```

## Packet Flow

```text
STM32 Field Unit
Address 1
     |
     | LoRa
     v
Arduino Relay
Address 0
     |
     | Forwarded LoRa packet
     v
Base Station
Address 2
     |
     | BACK acknowledgement
     v
Arduino Relay
```

## Features

- Receives forwarded Field Node telemetry
- Supports `GPS`, `STATUS`, and `SOS` packets
- Parses PECRN Protocol V1 (`P1`)
- Displays packet information in the terminal
- Displays GPS coordinates and satellite count
- Tracks packet IDs
- Sends Base acknowledgements (`BACK`) to the relay
- Ignores direct Field Node transmissions
- Provides serial and LoRa receive diagnostics

## Base Acknowledgement

When a valid packet is received from the Relay Node, the Base Station responds with:

```text
P1,BACK,<nodeID>,<packetID>
```

For example:

```text
P1,BACK,1,286
```

The Relay Node uses this acknowledgement to confirm successful end-to-end delivery and remove the packet from its queue.

## Example Output

```text
[RX] LoRa source 0: P1,GPS,1,286,47.66769,-122.31281,6

============================================================
Node 1 | Packet 286
TYPE       : GPS
LATITUDE   : 47.66769
LONGITUDE  : -122.31281
SATELLITES : 6
============================================================

[BACK -> RELAY] P1,BACK,1,286
```

Direct transmissions from the Field Node may also be detected by the Base Station radio, but they are intentionally ignored:

```text
[RX] LoRa source 1: P1,GPS,1,286,...
[DROP] Unexpected LoRa source 1
```

Only packets forwarded by the Relay Node (`Address 0`) are processed as valid Base Station traffic.

## Running the Base Station

Run the Python base station program from the terminal:

```bash
python3 base_station.py
```

The program will connect to the RYLR998 through the CP2102 USB-to-UART adapter and begin listening for forwarded PECRN packets.

## Files

- `base_station.py` — Main PECRN Base Station application
- `README.md` — Base Station software documentation
