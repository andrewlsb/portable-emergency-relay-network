import serial
import time

SERIAL_PORT = "/dev/cu.usbserial-0001"   # Update after CP2102 arrives
BAUD_RATE = 115200


def parse_packet(line):
    """
    Expected format:
    +RCV=<sender>,<length>,<payload>,<RSSI>,<SNR>
    """

    if not line.startswith("+RCV="):
        return None

    try:
        content = line[5:]

        first_comma = content.index(",")
        sender = content[:first_comma]

        remaining = content[first_comma + 1:]

        second_comma = remaining.index(",")
        payload_length = int(remaining[:second_comma])

        payload_start = second_comma + 1
        payload_end = payload_start + payload_length

        payload = remaining[payload_start:payload_end]

        radio_data = remaining[payload_end + 1:]
        radio_parts = radio_data.split(",")

        rssi = radio_parts[0]
        snr = radio_parts[1]

        return sender, payload, rssi, snr

    except (ValueError, IndexError):
        return None


def display_packet(sender, payload, rssi, snr):
    print("\n" + "=" * 45)
    print("        PECRN BASE STATION")
    print("=" * 45)

    print(f"SENDER:        {sender}")

    if payload.startswith("GPS,"):
        parts = payload.split(",")

        if len(parts) >= 3:
            latitude = parts[1]
            longitude = parts[2]

            print("TYPE:          GPS")
            print(f"LATITUDE:      {latitude}")
            print(f"LONGITUDE:     {longitude}")
        else:
            print("TYPE:          GPS")
            print(f"DATA:          {payload}")

    elif payload.startswith("SOS,"):
        print("TYPE:          SOS")
        print(f"DATA:          {payload}")

    else:
        print("TYPE:          UNKNOWN")
        print(f"DATA:          {payload}")

    print(f"RSSI:          {rssi} dBm")
    print(f"SNR:           {snr} dB")
    print("STATUS:        PACKET RECEIVED")
    print("=" * 45)


def main():
    print("PECRN Base Station")
    print("-------------------")
    print(f"Opening {SERIAL_PORT} at {BAUD_RATE} baud...")

    try:
        ser = serial.Serial(
            SERIAL_PORT,
            BAUD_RATE,
            timeout=1
        )

    except serial.SerialException as error:
        print("\nCould not open serial port.")
        print(error)
        print("\nUpdate SERIAL_PORT after connecting the CP2102.")
        return

    time.sleep(2)

    print("Connected.")
    print("Waiting for LoRa packets...\n")

    while True:
        try:
            line = ser.readline().decode(
                "utf-8",
                errors="ignore"
            ).strip()

            if not line:
                continue

            print(f"RAW: {line}")

            packet = parse_packet(line)

            if packet is not None:
                sender, payload, rssi, snr = packet
                display_packet(
                    sender,
                    payload,
                    rssi,
                    snr
                )

        except KeyboardInterrupt:
            print("\nBase station stopped.")
            ser.close()
            break


if __name__ == "__main__":
    main()
