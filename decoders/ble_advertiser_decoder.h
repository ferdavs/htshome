#ifndef BLE_ADVERTISER_DECODER_H
#define BLE_ADVERTISER_DECODER_H

#include <stdint.h>
#include <string>

struct SensorReading {
  std::string label;
  double value;
  std::string unit;
  int precision;
};

SensorReading decode_ble_advertisement(const uint8_t* data, size_t len);

#endif
