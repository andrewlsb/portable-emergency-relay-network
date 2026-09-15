# Arduino Relay Node

This folder contains the Arduino relay-node firmware for the Portable Emergency Communication Relay Network (PECRN).

## Platform

- Arduino UNO R4 WiFi
- RYLR998 LoRa Module
- UART communication between Arduino and RYLR998

## Node Addressing

| Device | LoRa Address |
|---|---:|
| Arduino Relay Node | 0 |
| STM32 Field Node | 1 |
| Base Station | 2 |

## Purpose

The Arduino Relay Node acts as the intermediate communication node between the STM32 Field Node and the Base Station.

The relay receives telemetry and emergency packets from the Field Node, acknowledges successful reception, temporarily stores packets in a queue, and reliably forwards them to the Base Station.

The relay is designed to retain packets when the Base Station is temporarily unavailable and retry delivery when communication becomes available again.

## Network Architecture

```text
STM32 Field Node
Address 1
      |
      | GPS / STATUS / SOS
      v
Arduino Relay Node
Address 0
      |
      | Reliable Forwarding
      v
Base Station
Address 2

Field Node <---- RACK ---- Relay Node
Relay Node <---- BACK ---- Base Station
```

## Protocol

PECRN currently uses protocol version `P1`.

Example GPS packet:

```text
P1,GPS,1,123,47.66769,-122.31284,6
```

Example status packet:

```text
P1,STATUS,1,124,GPS_FIX,6
```

Example SOS packet:

```text
P1,SOS,1,125,47.66769,-122.31284,FIX
```

## Relay Acknowledgement

After a packet has been successfully stored by the relay, the relay sends a `RACK` (Relay Acknowledgement) to the Field Node.

```text
P1,RACK,1,123
```

The Field Node retries transmission if the expected RACK is not received.

## Base Acknowledgement

After the Base Station receives a forwarded packet, it sends a `BACK` (Base Acknowledgement) to the relay.

```text
P1,BACK,1,123
```

The relay removes a packet from its queue only after the corresponding BACK has been received.

## Relay Queue

The relay maintains a 10-packet software queue.

This allows packets to remain stored if the Base Station is temporarily unavailable.

Normal GPS and STATUS telemetry uses FIFO ordering so older telemetry is delivered before newer telemetry.

## Retry and Backoff

If the relay does not receive a BACK from the Base Station within the acknowledgement timeout, it retries the packet.

Current configuration:

- Maximum attempts per retry cycle: 3
- BACK timeout: 2000 ms
- Retry backoff: 5000 ms
- Minimum radio TX gap: 350 ms

After the maximum number of attempts is reached, the packet remains in the queue and is retried later rather than being immediately discarded.

## SOS Priority

SOS packets receive priority over normal GPS and STATUS telemetry.

An SOS packet can therefore be selected for transmission ahead of queued normal telemetry while ordinary telemetry otherwise maintains FIFO ordering.

## Duplicate Detection

The relay implements two levels of duplicate detection.

### Active Queue Detection

If the Field Node retransmits a packet that is already stored in the relay queue, the relay does not create another copy.

Instead, it sends the RACK again.

### Completed Packet History

The relay also maintains a history of recently completed packets.

If a packet has already been successfully delivered to the Base Station but the Field Node retransmits it because it missed the original RACK, the relay recognizes the packet and does not forward it to the Base Station again.

The relay simply repeats the RACK.

## Queue-Full Protection

If all 10 relay queue entries are occupied, a newly received packet is not acknowledged.

This is intentional.

Sending a RACK would incorrectly tell the Field Node that the packet had been safely stored when no queue space was available.

## Radio Traffic Management

The relay implements transmission pacing to reduce collisions between LoRa traffic.

It also provides a short quiet period after receiving Field Node traffic before beginning queued transmissions toward the Base Station.

This gives Field-to-Relay traffic and RACK exchanges priority before the relay resumes forwarding queued packets.

## Implemented Features

- Field Node → Relay communication
- Relay → Base Station forwarding
- GPS packet forwarding
- STATUS packet forwarding
- SOS emergency packet forwarding
- Relay acknowledgements (RACK)
- Base acknowledgements (BACK)
- Packet retransmission
- Retry backoff
- 10-packet relay queue
- FIFO ordering for normal telemetry
- SOS priority
- Active duplicate detection
- Completed-packet duplicate detection
- Queue-full protection
- Radio transmission pacing
- Field traffic quiet period
- Temporary Base Station outage recovery

## Current Status

The three-node PECRN communication prototype has been tested with the STM32 Field Node, Arduino Relay Node, and Base Station operating together.

The relay has successfully demonstrated packet reception, acknowledgement, queueing, reliable forwarding, Base Station acknowledgement, retry behavior, and recovery of queued packets after temporary Base Station unavailability.
