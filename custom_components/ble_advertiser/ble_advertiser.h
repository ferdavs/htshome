#pragma once

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <NimBLEAdvertising.h>
#include <vector>
#include <string>

namespace esphome {
namespace ble_advertiser {

struct SensorConfig {
  sensor::Sensor *sensor{nullptr};
  std::string label;
  std::string unit;
  float scale_factor{1.0f};
  uint8_t precision{2};

  SensorConfig(sensor::Sensor *sensor, const std::string &label, 
               const std::string &unit = "", float scale_factor = 1.0f, 
               uint8_t precision = 2)
      : sensor(sensor), label(label), unit(unit), 
        scale_factor(scale_factor), precision(precision) {}
};

class BLEAdvertiser : public Component {
 public:
  BLEAdvertiser() = default;

  void add_sensor(sensor::Sensor *sensor, const std::string &label, 
                 const std::string &unit = "", float scale_factor = 1.0f, 
                 uint8_t precision = 2) {
    sensors_.push_back(SensorConfig(sensor, label, unit, scale_factor, precision));
  }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  std::vector<SensorConfig> sensors_;
  NimBLEAdvertising *advertising_{nullptr};

  void advertise();
  std::string format_sensor_value(const SensorConfig &config);
};

}  // namespace ble_advertiser
}  // namespace esphome
