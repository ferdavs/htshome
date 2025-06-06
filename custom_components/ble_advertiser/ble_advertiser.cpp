#include "ble_advertiser.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace ble_advertiser {

static const char *const TAG = "ble_advertiser";

// Configuration schema
class BLEAdvertiserComponent : public BLEAdvertiser, public Component {
 public:
  void dump_config() override {
    ESP_LOGCONFIG(TAG, "BLEAdvertiserComponent:");
  }
};

BLEAdvertiser *ble_advertiser;

BLEAdvertiser *make_ble_advertiser() {
  ble_advertiser = new BLEAdvertiser();
  return ble_advertiser;
}

void BLEAdvertiser::setup() {
  ESP_LOGI(TAG, "Setting up BLE Advertiser...");
  NimBLEDevice::init("HTS-HOME");
  NimBLEServer *server = NimBLEDevice::createServer();
  advertising_ = NimBLEDevice::getAdvertising();
  
  // Set advertising parameters
  advertising_->setMinInterval(0x20);  // 32ms
  advertising_->setMaxInterval(0x40);  // 64ms
  advertising_->setAdvertisementType(BLE_HCI_ADV_TYPE_ADV_IND);  // Connectable advertising
  
  // Set advertising power
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Maximum power
  
  ESP_LOGI(TAG, "BLE Advertiser setup complete");
  advertise();
}

void BLEAdvertiser::loop() {
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    advertise();
  }
}

void BLEAdvertiser::dump_config() {
  ESP_LOGCONFIG(TAG, "BLEAdvertiserComponent:");
  for (const auto &sensor : sensors_) {
    if (sensor.sensor) {
      ESP_LOGCONFIG(TAG, "  %s: %s", sensor.label.c_str(), sensor.sensor->get_name().c_str());
    }
  }
  ESP_LOGCONFIG(TAG, "  Bluetooth MAC Address: %s", NimBLEDevice::getAddress().toString().c_str());
}

std::string BLEAdvertiser::format_sensor_value(const SensorConfig &config) {
  if (!config.sensor || !config.sensor->has_state()) {
    return "";
  }
  
  float value = config.sensor->state * config.scale_factor;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.*f%s", config.precision, value, config.unit.c_str());
  return std::string(buffer);
}

void BLEAdvertiser::advertise() {
  if (sensors_.empty()) {
    ESP_LOGW(TAG, "No sensors configured");
    return;
  }

  ESP_LOGD(TAG, "Preparing advertisement data...");
  
  // Calculate payload size: 2 bytes for company ID + 2 bytes for sensor count + data
  int payload_size = 4;  // Company ID (2) + Sensor count (2)
  for (const auto &sensor : sensors_) {
    if (sensor.sensor && sensor.sensor->has_state()) {
      payload_size += 2;  // 2 bytes per sensor value
    }
  }
  
  uint8_t payload[24] = {0};  // Maximum possible size
  payload[0] = 0xFF;  // Company ID (LSB)
  payload[1] = 0xFF;  // Company ID (MSB)
  
  // Count active sensors
  uint16_t active_sensors = 0;
  for (const auto &sensor : sensors_) {
    if (sensor.sensor && sensor.sensor->has_state()) {
      active_sensors++;
    }
  }
  
  payload[2] = active_sensors >> 8;  // Sensor count (MSB)
  payload[3] = active_sensors & 0xFF;  // Sensor count (LSB)
  
  int payload_index = 4;  // Start after company ID and sensor count
  
  // Add sensor values
  for (const auto &sensor : sensors_) {
    if (sensor.sensor && sensor.sensor->has_state()) {
      float value = sensor.sensor->state * (1.0f / sensor.scale_factor); 
      int16_t scaled_value = (int16_t)value; 
      payload[payload_index++] = scaled_value >> 8;
      payload[payload_index++] = scaled_value & 0xFF;
      
      ESP_LOGD(TAG, "%s: %s", sensor.label.c_str(), format_sensor_value(sensor).c_str());
    }
  }
  
  // Stop current advertising
  advertising_->stop();
  
  // Create advertising data
  NimBLEAdvertisementData advData;
  
  // Add manufacturer data first
  std::string mfgData((char*)payload, payload_size);
  advData.setManufacturerData(mfgData);
  
  // Set flags
  advData.setFlags(0x06);
  
  // Set name
  advData.setName("HTS-HOME");
  
  // Add service UUIDs
  std::vector<NimBLEUUID> serviceUUIDs;
  serviceUUIDs.push_back(NimBLEUUID("181A"));  // Environmental Sensing Service
  serviceUUIDs.push_back(NimBLEUUID("181C"));  // User Data Service
  
  for (const auto& uuid : serviceUUIDs) {
    advData.setCompleteServices(uuid);
  }
  
  // Set the advertising data
  advertising_->setAdvertisementData(advData);
  
  // Create scan response data
  NimBLEAdvertisementData scanResponse;
  scanResponse.setFlags(0x06);
  scanResponse.setName("HTS-HOME");
  
  // Add manufacturer data to scan response
  scanResponse.setManufacturerData(mfgData);
  
  // Set the scan response data
  advertising_->setScanResponseData(scanResponse);
  
  // Start advertising
  advertising_->start();
  
  ESP_LOGD(TAG, "Advertisement started with %d sensors", active_sensors);
  
  // // Debug: Print the advertising data
  // std::string advDataStr = advData.getPayload();
  // ESP_LOGD(TAG, "Advertising data length: %d", advDataStr.length());
  // for (size_t i = 0; i < advDataStr.length(); i++) {
  //   ESP_LOGD(TAG, "Byte %d: 0x%02X", i, (uint8_t)advDataStr[i]);
  // }
  
  // // Debug: Print scan response data
  // std::string scanResponseStr = scanResponse.getPayload();
  // ESP_LOGD(TAG, "Scan response data length: %d", scanResponseStr.length());
  // for (size_t i = 0; i < scanResponseStr.length(); i++) {
  //   ESP_LOGD(TAG, "Scan Response Byte %d: 0x%02X", i, (uint8_t)scanResponseStr[i]);
  // }
  
  // // Debug: Print manufacturer data
  // ESP_LOGD(TAG, "Manufacturer data length: %d", mfgData.length());
  // for (size_t i = 0; i < mfgData.length(); i++) {
  //   ESP_LOGD(TAG, "Mfg Byte %d: 0x%02X", i, (uint8_t)mfgData[i]);
  // }
  
  // // Debug: Print sensor values
  // for (const auto &sensor : sensors_) {
  //   if (sensor.sensor && sensor.sensor->has_state()) {
  //     ESP_LOGD(TAG, "Sensor %s: raw value = %.2f, scaled = %.2f", 
  //              sensor.label.c_str(), 
  //              sensor.sensor->state,
  //              sensor.sensor->state * sensor.scale_factor);
  //   }
  // }
}

}  // namespace ble_advertiser
}  // namespace esphome
