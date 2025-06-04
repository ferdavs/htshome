import socket
import struct
from datetime import datetime, timezone

from rich.console import Console
from rich.table import Table

console = Console()
MAGIC_NUMBER = 0x4553
MAGIC_PING = 0x5048
ROLLING_CODE_KEY = 0x04
DATA_KEY = 0x01
PING_KEY = 0x03
SENSOR_KEY = 0x02
BINARY_SENSOR_KEY = 0x01
ZERO_FILL_KEY = 0x00
MAX_PACKET_SIZE = 508

TYPES = {
    "battery_voltage": float,
    "breath_voc": float,
    "co2": float,
    "humidity": float,
    "iaq_accuracy": int,
    "iaq_averaged": int,
    "iaq_env": int,
    "iaq_pm": int,
    "iaq_pm_env": int,
    "pm_10_0": float,
    "pm_1_0": float,
    "pm_2_5": float,
    "pmc_0_3": int,
    "pmc_0_5": int,
    "pmc_10_0": int,
    "pmc_1_0": int,
    "pmc_2_5": int,
    "pmc_5_0": int,
    "pressure": float,
    "temperature": float,
    "timestamp": str,
    "uptime_sensor": int,
    # "wifi_signal_rssi": int,
    "gas_resistance": float,
}


def round4(value):
    return (value + 3) & ~3


def get_uint32(buf):
    return struct.unpack_from("<I", buf)[0], buf[4:]


def get_uint16(buf):
    return struct.unpack_from("<H", buf)[0], buf[2:]


def get_byte(buf):
    return struct.unpack_from("<B", buf)[0], buf[1:]


def parse_udp_packet(buf) -> dict:
    sensors = {}
    buf_len = len(buf)
    if buf_len < 8:
        print("Bad length")
        return sensors

    magic, buf = get_uint16(buf)

    if magic != MAGIC_NUMBER:
        print(f"Bad magic {magic:X}")
        return sensors

    host_len, buf = get_byte(buf)

    buf = buf[round4(host_len + 1) :]

    while len(buf) > 0 and buf[0] == 0x00:
        buf = buf[1:]

    if len(buf) < 200:
        return sensors

    byte, buf = get_byte(buf)

    if byte == ROLLING_CODE_KEY:
        buf = buf[8:]
    elif byte != DATA_KEY:
        print(f"Expected rolling code or data key, got {byte:X}")
        return sensors
    sensors["timestamp"] = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
    while len(buf) > 0:
        key, buf = get_byte(buf)

        if key == ZERO_FILL_KEY:
            continue

        if key == BINARY_SENSOR_KEY:
            if len(buf) < 3:
                return
            sensor_value, buf = get_byte(buf)
            sensor_len, buf = get_byte(buf)
            sensor_name = buf[:sensor_len].decode("utf-8")
            buf = buf[sensor_len:]
            sensors[sensor_name] = bool(sensor_value)

        elif key == SENSOR_KEY:
            sensor_value, buf = get_uint32(buf)
            sensor_len, buf = get_byte(buf)
            sensor_name = buf[:sensor_len].decode("utf-8")
            buf = buf[sensor_len:]
            value = struct.unpack("<f", struct.pack("<I", sensor_value))

            sensors[sensor_name] = round(value[0], 1)

    return sensors


def value2str(key, update):
    return str(TYPES.get(key, str)(update[key]))


def listen_for_broadcasts(port=18511):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind(("", port))

    print(f"Listening for UDP broadcasts on port {port}...")
    sensors = {}
    print("\n")
    while True:
        data, addr = sock.recvfrom(MAX_PACKET_SIZE)
        try:
            update = parse_udp_packet(data)

            table = Table(title="Sensor Data", show_lines=True)
            table.add_column("#", style="cyan")
            table.add_column("Sensor", style="cyan")
            table.add_column("Value", style="white")
            table.add_column("Sensor", style="cyan")
            table.add_column("Value", style="white")
            # complete to even number
            if len(update) % 2 != 0:
                update[""] = ""
            half = len(update) // 2
            keys = sorted(list(update.keys()))

            # compare previous value to current value
            # if bigger, color red, if smaller color green
            # if equal, color white
            # changes = {k: "white" for k in keys}
            # if sensors:
            #     for k in keys:
            #         if k in sensors:
            #             if update[k] > sensors[k]:
            #                 changes[k] = "green"
            #             elif update[k] < sensors[k]:
            #                 changes[k] = "red"
            for i in range(half):
                table.add_row(
                    str(i + 1),
                    keys[i],
                    value2str(keys[i], update),
                    keys[i + half],
                    value2str(keys[i + half], update),
                    # style=changes[keys[i]],
                )

            print("\033[A\r" * 28)
            if update:
                console.print(table)
            sensors.update(update)
        except KeyError as e:
            print(f"Error: {e}")
            continue
        except Exception:
            continue


if __name__ == "__main__":
    listen_for_broadcasts()
