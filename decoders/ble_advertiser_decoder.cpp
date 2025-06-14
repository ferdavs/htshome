#include "ble_advertiser_decoder.h"
#include <cstring>
#include <cmath>

SensorReading decode_ble_advertisement(const uint8_t* data, size_t len) {
  if (data == nullptr || len < 4) {
    throw std::invalid_argument("Invalid advertisement data");
  }

  // Find manufacturer data in the advertisement packet
  size_t index = 0;
  while (index < len) {
    size_t length = data[index];
    if (length == 0) break;

    size_t type = data[index + 1];
    if (type == 0xFF) {  // Manufacturer Specific Data
      // Extract manufacturer data (skip length and type bytes)
      const uint8_t* manufacturer_data = data + index + 2;
      return decode_manufacturer_data(manufacturer_data, length - 1);
    }

    index += length + 1;
  }

  throw std::invalid_argument("No manufacturer data found in advertisement");
}

SensorReading decode_manufacturer_data(const uint8_t* data, size_t len) {
  if (data == nullptr || len < 2) {
    throw std::invalid_argument("Invalid manufacturer data");
  }

  // Verify company ID (0xFFFF)
  if (data[0] != 0xFF || data[1] != 0xFF) {
    throw std::invalid_argument("Invalid company ID");
  }

  size_t payload_index = 2;  // Start after company ID

  // Read single sensor data
  if (payload_index + 4 > len) {
    throw std::invalid_argument("Incomplete sensor data");
  }

  size_t label_length = data[payload_index++];
  std::string label((char*)data + payload_index, label_length);
  payload_index += label_length;

  size_t unit_length = data[payload_index++];
  std::string unit((char*)data + payload_index, unit_length);
  payload_index += unit_length;

  int precision = data[payload_index++];

  // Read value (4 bytes, big endian)
  if (payload_index + 4 > len) {
    throw std::invalid_argument("Incomplete sensor value");
  }

  int32_t raw_value = (static_cast<int32_t>(data[payload_index]) << 24) |
                      (static_cast<int32_t>(data[payload_index + 1]) << 16) |
                      (static_cast<int32_t>(data[payload_index + 2]) << 8) |
                      static_cast<int32_t>(data[payload_index + 3]);

  // Calculate scale factor from precision (10^-precision)
  double scale_factor = std::pow(10, -precision);
  double value = raw_value * scale_factor;

  return {label, value, unit, precision};
}
