import struct

def decode_ble_advertisement(data):
    if len(data) < 4:
        raise ValueError("Invalid advertisement data")

    # Find manufacturer data in the advertisement packet
    index = 0
    while index < len(data):
        length = data[index]
        if length == 0:
            break

        type = data[index + 1]
        if type == 0xFF:  # Manufacturer Specific Data
            # Extract manufacturer data (skip length and type bytes)
            manufacturer_data = data[index + 2:index + 2 + length - 1]
            return decode_manufacturer_data(manufacturer_data)

        index += length + 1

    raise ValueError("No manufacturer data found in advertisement")


def decode_manufacturer_data(data):
    if len(data) < 2:
        raise ValueError("Invalid manufacturer data")

    # Verify company ID (0xFFFF)
    if data[0] != 0xFF or data[1] != 0xFF:
        raise ValueError("Invalid company ID")

    payload_index = 2  # Start after company ID

    # Read single sensor data
    if payload_index + 4 > len(data):
        raise ValueError("Incomplete sensor data")

    label_length = data[payload_index]
    payload_index += 1
    label = data[payload_index:payload_index + label_length].decode("utf-8")
    payload_index += label_length

    unit_length = data[payload_index]
    payload_index += 1
    unit = data[payload_index:payload_index + unit_length].decode("utf-8")
    payload_index += unit_length

    precision = data[payload_index]
    payload_index += 1

    # Read value (4 bytes, big endian)
    if payload_index + 4 > len(data):
        raise ValueError("Incomplete sensor value")

    rawValue = struct.unpack(">i", data[payload_index:payload_index + 4])[0]

    # Calculate scale factor from precision (10^-precision)
    scale_factor = 10 ** -precision
    value = rawValue * scale_factor

    return {"label": label, "value": value, "unit": unit, "precision": precision}
