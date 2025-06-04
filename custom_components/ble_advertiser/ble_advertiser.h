#pragma once

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <NimBLEAdvertising.h>

namespace esphome {
namespace ble_advertiser {

class BLEAdvertiser : public Component {
 public:
  BLEAdvertiser() = default;

  void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
  void set_humidity(sensor::Sensor *humidity) { humidity_ = humidity; }
  void set_pressure(sensor::Sensor *pressure) { pressure_ = pressure; }
  void set_co2(sensor::Sensor *co2) { co2_ = co2; }
  void set_pm1_0(sensor::Sensor *pm1_0) { pm1_0_ = pm1_0; }
  void set_pm2_5(sensor::Sensor *pm2_5) { pm2_5_ = pm2_5; }
  void set_pm10_0(sensor::Sensor *pm10_0) { pm10_0_ = pm10_0; }
  void set_iaq(sensor::Sensor *iaq) { iaq_ = iaq; }
  void set_battery(sensor::Sensor *battery) { battery_ = battery; }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *humidity_{nullptr};
  sensor::Sensor *pressure_{nullptr};
  sensor::Sensor *co2_{nullptr};
  sensor::Sensor *pm1_0_{nullptr};
  sensor::Sensor *pm2_5_{nullptr};
  sensor::Sensor *pm10_0_{nullptr};
  sensor::Sensor *iaq_{nullptr};
  sensor::Sensor *battery_{nullptr};
  NimBLEAdvertising *advertising_{nullptr};

  void advertise();
};

}  // namespace ble_advertiser
}  // namespace esphome
