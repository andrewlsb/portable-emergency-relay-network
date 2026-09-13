# Arduino Relay Node

This folder contains the relay-node firmware for Milestone 5 of the Portable Emergency Communication Relay Network.

## Platform

- Arduino UNO R4 WiFi
- RYLR998 LoRa Module

## Node Addressing

- STM32 Field Node: Address 1
- Arduino Relay Node: Address 0
- Base Station: Address 2

## Purpose

The relay node receives GPS packets from the STM32 field unit and automatically retransmits them to the base station.

## Packet Flow

```text
STM32 Field Node
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
