#include "ble_advertiser.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ble_advertiser {

  static const char *const TAG = "ble_advertiser";

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
    NimBLEDevice::init(device_name_.c_str());
    NimBLEServer *server = NimBLEDevice::createServer();
    advertising_ = NimBLEDevice::getAdvertising();
    
    advertising_->setMinInterval(min_interval_); 
    advertising_->setMaxInterval(max_interval_); 
    advertising_->setAdvertisementType(BLE_HCI_ADV_TYPE_ADV_IND); 
    
    // Map power level (0-9) to ESP32 BLE power levels
    esp_power_level_t power_level;
    switch (power_level_) {
      case 0: power_level = ESP_PWR_LVL_N12; break;  // -12 dBm
      case 1: power_level = ESP_PWR_LVL_N9; break;   // -9 dBm
      case 2: power_level = ESP_PWR_LVL_N6; break;   // -6 dBm
      case 3: power_level = ESP_PWR_LVL_N3; break;   // -3 dBm
      case 4: power_level = ESP_PWR_LVL_N0; break;   // 0 dBm
      case 5: power_level = ESP_PWR_LVL_P3; break;   // 3 dBm
      case 6: power_level = ESP_PWR_LVL_P6; break;   // 6 dBm
      case 7: power_level = ESP_PWR_LVL_P9; break;   // 9 dBm
      case 8: power_level = ESP_PWR_LVL_P9; break;   // 9 dBm (max available)
      case 9: power_level = ESP_PWR_LVL_P9; break;   // 9 dBm (max available)
      default: power_level = ESP_PWR_LVL_P9; break;
    }
    NimBLEDevice::setPower(power_level);
    
    current_sensor_index_ = 0;
    ESP_LOGI(TAG, "BLE Advertiser setup complete");
    advertise();
  }

  void BLEAdvertiser::loop() {
    static uint32_t last = 0;
    uint32_t now = millis();
    
    // Handle millis() overflow
    if (now < last) {
      last = now;
      return;
    }
    
    // Convert update_interval from seconds to milliseconds
    uint32_t update_interval_ms = update_interval_ * 1000;
    
    // Check if it's time to update
    if (now - last >= update_interval_ms) {
      last = now;
      advertise();
    }
  }

  void BLEAdvertiser::dump_config() {
    ESP_LOGCONFIG(TAG, "BLEAdvertiserComponent:");
    ESP_LOGCONFIG(TAG, "  Update Interval: %u s", update_interval_);
    ESP_LOGCONFIG(TAG, "  Min Interval: %u", min_interval_);
    ESP_LOGCONFIG(TAG, "  Max Interval: %u", max_interval_);
    ESP_LOGCONFIG(TAG, "  Device Name: %s", device_name_.c_str());
    ESP_LOGCONFIG(TAG, "  Power Level: %u", power_level_);
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
    
    // Calculate scale factor from precision (1/10^precision)
    float scale_factor = 1.0f;
    for (int i = 0; i < config.precision; i++) {
      scale_factor *= 0.1f;
    }
    
    float value = config.sensor->state * scale_factor;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f%s", config.precision, value, config.unit.c_str());
    return std::string(buffer);
  }

  void BLEAdvertiser::advertise() {
    if (sensors_.empty()) {
      ESP_LOGW(TAG, "No sensors configured");
      return;
    }

    // Get the current sensor
    const auto &sensor = sensors_[current_sensor_index_];
    if (!sensor.sensor || !sensor.sensor->has_state()) {
      // Move to next sensor
      current_sensor_index_ = (current_sensor_index_ + 1) % sensors_.size();
      return;
    }

    ESP_LOGD(TAG, "Preparing advertisement data for %s...", sensor.label.c_str());
    
    // Calculate payload size for single sensor
    int payload_size = 2;  // Company ID (2)
    payload_size += 1 + sensor.label.length() + 1 + sensor.unit.length() + 1 + 2;  // Label + Unit + Precision + Value
    
    uint8_t payload[128] = {0};
    int payload_index = 0;
    
    // Company ID
    payload[payload_index++] = 0xFF;
    payload[payload_index++] = 0xFF;
    
    // Label length and label
    payload[payload_index++] = sensor.label.length();
    memcpy(&payload[payload_index], sensor.label.c_str(), sensor.label.length());
    payload_index += sensor.label.length();
    
    // Unit length and unit
    payload[payload_index++] = sensor.unit.length();
    memcpy(&payload[payload_index], sensor.unit.c_str(), sensor.unit.length());
    payload_index += sensor.unit.length();
    
    // Precision
    payload[payload_index++] = sensor.precision;
    
    // Calculate scale factor from precision
    float scale_factor = 1.0f;
    for (int i = 0; i < sensor.precision; i++) {
      scale_factor *= 0.1f;
    }
    
    // Value
    float value = sensor.sensor->state * (1.0f / scale_factor);
    int16_t scaled_value = (int16_t)value;
    payload[payload_index++] = scaled_value >> 8;
    payload[payload_index++] = scaled_value & 0xFF;
    
    ESP_LOGD(TAG, "%s: %s", sensor.label.c_str(), format_sensor_value(sensor).c_str());
    
    advertising_->stop();
    
    NimBLEAdvertisementData advData;
    
    std::string mfgData((char*)payload, payload_index);
    advData.setManufacturerData(mfgData);
    advData.setFlags(0x06);
    advData.setName(device_name_.c_str());
    
    std::vector<NimBLEUUID> serviceUUIDs;
    serviceUUIDs.push_back(NimBLEUUID("181A")); 
    serviceUUIDs.push_back(NimBLEUUID("181C")); 
    
    for (const auto& uuid : serviceUUIDs) {
      advData.setCompleteServices(uuid);
    }
    
    advertising_->setAdvertisementData(advData);
    
    NimBLEAdvertisementData scanResponse;
    scanResponse.setFlags(0x06);
    scanResponse.setName(device_name_.c_str());
    scanResponse.setManufacturerData(mfgData);
    
    advertising_->setScanResponseData(scanResponse);
    
    advertising_->start();
    
    // Move to next sensor for next advertisement
    current_sensor_index_ = (current_sensor_index_ + 1) % sensors_.size();
    
    ESP_LOGD(TAG, "Advertisement started for %s", sensor.label.c_str());
  }

}  // namespace ble_advertiser
}  // namespace esphome
