import serial
import time
from datetime import datetime

# ============================================================
# PECRN BASE STATION - PROTOCOL V1
# ============================================================

SERIAL_PORT = "/dev/cu.usbserial-0001"
BAUD_RATE = 115200

PROTOCOL_VERSION = "P1"

RELAY_ADDRESS = 0
BASE_ADDRESS = 2

# Remember recently received packets so retries are not
# displayed/logged multiple times.
HISTORY_SIZE = 100

received_history = []
ser = None


# ============================================================
# HELPERS
# ============================================================

def timestamp():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def already_received(node_id, packet_id):
    return (node_id, packet_id) in received_history


def remember_packet(node_id, packet_id):
    received_history.append((node_id, packet_id))

    if len(received_history) > HISTORY_SIZE:
        received_history.pop(0)


# ============================================================
# SEND THROUGH RYLR998
# ============================================================

def send_lora(destination, payload):
    command = (
        f"AT+SEND={destination},"
        f"{len(payload.encode('utf-8'))},"
        f"{payload}\r\n"
    )

    ser.write(command.encode("utf-8"))
    ser.flush()


# ============================================================
# SEND BASE ACK
# ============================================================

def send_back(node_id, packet_id):
    payload = f"P1,BACK,{node_id},{packet_id}"

    send_lora(RELAY_ADDRESS, payload)

    print(f"[BACK -> RELAY] {payload}")


# ============================================================
# PARSE RYLR998 +RCV MESSAGE
# ============================================================

def parse_rcv(line):
    """
    RYLR998 format:

    +RCV=<address>,<length>,<data>,<RSSI>,<SNR>

    Because our data contains commas, we MUST use the
    length field instead of simply splitting the whole line.
    """

    if not line.startswith("+RCV="):
        return None

    try:
        first_comma = line.index(",")

        source = int(line[5:first_comma])

        second_comma = line.index(",", first_comma + 1)

        payload_length = int(
            line[first_comma + 1:second_comma]
        )

        payload_start = second_comma + 1
        payload_end = payload_start + payload_length

        if payload_end > len(line):
            return None

        payload = line[payload_start:payload_end]

        return source, payload

    except (ValueError, IndexError):
        return None


# ============================================================
# PROCESS APPLICATION PACKET
# ============================================================

def process_packet(source, payload):

    # We only expect application data from the relay.
    if source != RELAY_ADDRESS:
        print(f"[DROP] Unexpected LoRa source {source}")
        return

    parts = payload.split(",")

    if len(parts) < 4:
        print(f"[DROP] Malformed packet: {payload}")
        return

    if parts[0] != PROTOCOL_VERSION:
        print(f"[DROP] Unsupported protocol: {parts[0]}")
        return

    packet_type = parts[1]

    if packet_type not in ("GPS", "STATUS", "SOS"):
        print(f"[DROP] Unknown packet type: {packet_type}")
        return

    try:
        node_id = int(parts[2])
        packet_id = int(parts[3])
    except ValueError:
        print("[DROP] Invalid node/packet ID")
        return

    if node_id <= 0 or packet_id <= 0:
        print("[DROP] Invalid node/packet ID")
        return

    # ========================================================
    # DUPLICATE
    # ========================================================

    if already_received(node_id, packet_id):

        print(
            f"[DUPLICATE] Node {node_id} "
            f"Packet {packet_id}"
        )

        # VERY IMPORTANT:
        # ACK duplicates again because the previous BACK
        # may have been lost.
        send_back(node_id, packet_id)

        return

    # ========================================================
    # VALIDATE PACKET-SPECIFIC CONTENT BEFORE ACK
    # ========================================================

    if packet_type == "GPS":
        if len(parts) != 7:
            print("[DROP] Malformed GPS packet")
            return

        try:
            latitude = float(parts[4])
            longitude = float(parts[5])
            satellites = int(parts[6])
        except ValueError:
            print("[DROP] Invalid GPS values")
            return

        if not (-90.0 <= latitude <= 90.0):
            print("[DROP] Invalid latitude")
            return

        if not (-180.0 <= longitude <= 180.0):
            print("[DROP] Invalid longitude")
            return

    elif packet_type == "STATUS":
        if len(parts) < 5:
            print("[DROP] Malformed STATUS packet")
            return

    elif packet_type == "SOS":
        if len(parts) not in (5, 7):
            print("[DROP] Malformed SOS packet")
            return

        if len(parts) == 7:
            try:
                latitude = float(parts[4])
                longitude = float(parts[5])
            except ValueError:
                print("[DROP] Invalid SOS coordinates")
                return

            if not (-90.0 <= latitude <= 90.0):
                print("[DROP] Invalid SOS latitude")
                return

            if not (-180.0 <= longitude <= 180.0):
                print("[DROP] Invalid SOS longitude")
                return

    # ========================================================
    # PACKET IS VALID
    # ========================================================

    remember_packet(node_id, packet_id)

    print()
    print("=" * 60)

    print(
        f"[{timestamp()}] "
        f"Node {node_id} | Packet {packet_id}"
    )

    # ========================================================
    # GPS
    # ========================================================

    if packet_type == "GPS":

        latitude = float(parts[4])
        longitude = float(parts[5])
        satellites = int(parts[6])

        print("TYPE       : GPS")
        print(f"LATITUDE   : {latitude:.5f}")
        print(f"LONGITUDE  : {longitude:.5f}")
        print(f"SATELLITES : {satellites}")

    # ========================================================
    # STATUS
    # ========================================================

    elif packet_type == "STATUS":

        status = parts[4]

        print("TYPE       : STATUS")
        print(f"STATUS     : {status}")

        if status == "GPS_FIX" and len(parts) >= 6:
            print(f"SATELLITES : {parts[5]}")

    # ========================================================
    # SOS
    # ========================================================

    elif packet_type == "SOS":

        print()
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        print("!!!            SOS ALERT             !!!")
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

        if len(parts) == 5 and parts[4] == "NO_GPS":

            print("GPS        : NO GPS FIX")

        elif len(parts) == 7:

            latitude = float(parts[4])
            longitude = float(parts[5])
            gps_status = parts[6]

            print(f"LATITUDE   : {latitude:.5f}")
            print(f"LONGITUDE  : {longitude:.5f}")
            print(f"GPS STATUS : {gps_status}")

        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        print()

    print("=" * 60)

    # ========================================================
    # ACKNOWLEDGE ONLY AFTER VALID PROCESSING
    # ========================================================

    send_back(node_id, packet_id)


# ============================================================
# MAIN
# ============================================================

def main():

    global ser

    print()
    print("=" * 60)
    print("PECRN BASE STATION")
    print("Protocol: P1")
    print(f"Serial: {SERIAL_PORT}")
    print(f"Baud: {BAUD_RATE}")
    print("Base LoRa Address: 2")
    print("Relay LoRa Address: 0")
    print("=" * 60)
    print()

    try:
        ser = serial.Serial(
            SERIAL_PORT,
            BAUD_RATE,
            timeout=0.1
        )

    except serial.SerialException as e:

        print("[ERROR] Could not open Base LoRa.")
        print(e)
        return

    # Give USB serial a moment to settle.
    time.sleep(0.5)

    # Remove any old UART data.
    ser.reset_input_buffer()

    print("[BASE] Serial connection opened.")
    print("[BASE] Waiting for PECRN packets...")
    print()
    print("Press Control+C to stop.")
    print()

    try:

        while True:

            raw = ser.readline()

            if not raw:
                continue

            line = raw.decode(
                "utf-8",
                errors="replace"
            ).strip()

            if not line:
                continue

            # Ignore normal AT command confirmation.
            if line == "+OK":
                continue

            if line.startswith("+ERR"):
                print(f"[LORA ERROR] {line}")
                continue

            if not line.startswith("+RCV="):
                print(f"[LORA] {line}")
                continue

            result = parse_rcv(line)

            if result is None:
                print(
                    f"[ERROR] Could not parse: {line}"
                )
                continue

            source, payload = result

            print(
                f"[RX] LoRa source {source}: {payload}"
            )

            process_packet(source, payload)

    except KeyboardInterrupt:

        print()
        print("[BASE] Shutting down...")

    finally:

        if ser is not None and ser.is_open:
            ser.close()

        print("[BASE] Serial port closed.")


if __name__ == "__main__":
    main()
