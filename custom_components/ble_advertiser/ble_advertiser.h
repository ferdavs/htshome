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

  void set_update_interval(uint32_t interval) { update_interval_ = interval; }
  void set_min_interval(uint16_t interval) { min_interval_ = interval; }
  void set_max_interval(uint16_t interval) { max_interval_ = interval; }
  void set_device_name(const std::string &name) { device_name_ = name; }
  void set_power_level(uint8_t level) { power_level_ = level; }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  std::vector<SensorConfig> sensors_;
  NimBLEAdvertising *advertising_{nullptr};
  uint32_t update_interval_{10000};
  uint16_t min_interval_{32};
  uint16_t max_interval_{64};
  std::string device_name_{"HTS-HOME"};
  uint8_t power_level_{9};
  size_t current_sensor_index_{0};

  void advertise();
  std::string format_sensor_value(const SensorConfig &config);
};

}  // namespace ble_advertiser
}  // namespace esphome
