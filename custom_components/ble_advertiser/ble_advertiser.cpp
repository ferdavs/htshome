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
  
  // Configure advertising
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);  // General Discoverable Mode + BR/EDR Not Supported
  advData.setName("HTS-HOME");
  
  // Add service UUIDs for environmental sensing
  std::vector<NimBLEUUID> serviceUUIDs;
  serviceUUIDs.push_back(NimBLEUUID("181A"));  // Environmental Sensing Service
  serviceUUIDs.push_back(NimBLEUUID("181C"));  // User Data Service
  
  // Add services to advertising data
  for (const auto& uuid : serviceUUIDs) {
    advData.setCompleteServices(uuid);  // Use setCompleteServices instead of setServiceData
  }
  
  advertising_->setAdvertisementData(advData);
  
  NimBLEAdvertisementData scanResponse;
  scanResponse.setFlags(0x06);
  scanResponse.setName("HTS-HOME");
  
  // Add services to scan response
  for (const auto& uuid : serviceUUIDs) {
    scanResponse.setCompleteServices(uuid);  // Use setCompleteServices instead of setServiceData
  }
  
  advertising_->setScanResponseData(scanResponse);
  
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
  if (temperature_) {
    ESP_LOGCONFIG(TAG, "  Temperature: %s", temperature_->get_name().c_str());
  }
  if (humidity_) {
    ESP_LOGCONFIG(TAG, "  Humidity: %s", humidity_->get_name().c_str());
  }
  if (pressure_) {
    ESP_LOGCONFIG(TAG, "  Pressure: %s", pressure_->get_name().c_str());
  }
  if (co2_) {
    ESP_LOGCONFIG(TAG, "  CO2: %s", co2_->get_name().c_str());
  }
  if (pm1_0_) {
    ESP_LOGCONFIG(TAG, "  PM1.0: %s", pm1_0_->get_name().c_str());
  }
  if (pm2_5_) {
    ESP_LOGCONFIG(TAG, "  PM2.5: %s", pm2_5_->get_name().c_str());
  }
  if (pm10_0_) {
    ESP_LOGCONFIG(TAG, "  PM10.0: %s", pm10_0_->get_name().c_str());
  }
  if (iaq_) {
    ESP_LOGCONFIG(TAG, "  IAQ: %s", iaq_->get_name().c_str());
  }
  if (battery_) {
    ESP_LOGCONFIG(TAG, "  Battery: %s", battery_->get_name().c_str());
  }
  ESP_LOGCONFIG(TAG, "  Bluetooth MAC Address: %s", NimBLEDevice::getAddress().toString().c_str());
}

void BLEAdvertiser::advertise() {
  // Check if at least one sensor is available
  if (!temperature_ && !humidity_ && !pressure_ && !co2_ && !pm1_0_ && !pm2_5_ && !pm10_0_ && !iaq_ && !battery_) {
    ESP_LOGW(TAG, "No sensors available");
    return;
  }

  ESP_LOGD(TAG, "Preparing advertisement data...");
  
  // Define sensor flags
  const uint16_t FLAG_TEMPERATURE = 0x0001;  // Bit 0
  const uint16_t FLAG_HUMIDITY    = 0x0002;  // Bit 1
  const uint16_t FLAG_PRESSURE    = 0x0004;  // Bit 2
  const uint16_t FLAG_CO2         = 0x0008;  // Bit 3
  const uint16_t FLAG_PM1_0       = 0x0010;  // Bit 4
  const uint16_t FLAG_PM2_5       = 0x0020;  // Bit 5
  const uint16_t FLAG_PM10_0      = 0x0040;  // Bit 6
  const uint16_t FLAG_IAQ         = 0x0080;  // Bit 7
  const uint16_t FLAG_BATTERY     = 0x0100;  // Bit 8

  // Count available sensors to determine payload size
  int available_sensors = 0;
  uint16_t sensor_flags = 0x0000;

  if (temperature_) {
    available_sensors++;
    sensor_flags |= FLAG_TEMPERATURE;
  }
  if (humidity_) {
    available_sensors++;
    sensor_flags |= FLAG_HUMIDITY;
  }
  if (pressure_) {
    available_sensors++;
    sensor_flags |= FLAG_PRESSURE;
  }
  if (co2_) {
    available_sensors++;
    sensor_flags |= FLAG_CO2;
  }
  if (pm1_0_) {
    available_sensors++;
    sensor_flags |= FLAG_PM1_0;
  }
  if (pm2_5_) {
    available_sensors++;
    sensor_flags |= FLAG_PM2_5;
  }
  if (pm10_0_) {
    available_sensors++;
    sensor_flags |= FLAG_PM10_0;
  }
  if (iaq_) {
    available_sensors++;
    sensor_flags |= FLAG_IAQ;
  }
  if (battery_) {
    available_sensors++;
    sensor_flags |= FLAG_BATTERY;
  }

  // Calculate payload size: 2 bytes for company ID + 2 bytes per sensor + 2 bytes for flags
  int payload_size = 2 + (available_sensors * 2) + 2;
  uint8_t payload[24] = {0};  // Maximum possible size
  payload[0] = 0xFF;  // Company ID (LSB)
  payload[1] = 0xFF;  // Company ID (MSB)
  
  int payload_index = 2;  // Start after company ID
  
  // Temperature (2 bytes)
  if (temperature_ && temperature_->has_state()) {
    int16_t temp = (int16_t)(temperature_->state * 100);
    payload[payload_index++] = temp >> 8;
    payload[payload_index++] = temp & 0xFF;
    ESP_LOGD(TAG, "Temperature: %.2f°C", temperature_->state);
  }
  
  // Humidity (2 bytes)
  if (humidity_ && humidity_->has_state()) {
    uint16_t hum = (uint16_t)(humidity_->state * 100);
    payload[payload_index++] = hum >> 8;
    payload[payload_index++] = hum & 0xFF;
    ESP_LOGD(TAG, "Humidity: %.2f%%", humidity_->state);
  }
  
  // Pressure (2 bytes)
  if (pressure_ && pressure_->has_state()) {
    uint16_t pres = (uint16_t)(pressure_->state * 10);
    payload[payload_index++] = pres >> 8;
    payload[payload_index++] = pres & 0xFF;
    ESP_LOGD(TAG, "Pressure: %.2fhPa", pressure_->state);
  }
  
  // CO2 (2 bytes)
  if (co2_ && co2_->has_state()) {
    uint16_t co2 = (uint16_t)(co2_->state);
    payload[payload_index++] = co2 >> 8;
    payload[payload_index++] = co2 & 0xFF;
    ESP_LOGD(TAG, "CO2: %.2fppm", co2_->state);
  }
  
  // PM1.0 (2 bytes)
  if (pm1_0_ && pm1_0_->has_state()) {
    uint16_t pm1 = (uint16_t)(pm1_0_->state);
    payload[payload_index++] = pm1 >> 8;
    payload[payload_index++] = pm1 & 0xFF;
    ESP_LOGD(TAG, "PM1.0: %.2fµg/m³", pm1_0_->state);
  }
  
  // PM2.5 (2 bytes)
  if (pm2_5_ && pm2_5_->has_state()) {
    uint16_t pm2 = (uint16_t)(pm2_5_->state);
    payload[payload_index++] = pm2 >> 8;
    payload[payload_index++] = pm2 & 0xFF;
    ESP_LOGD(TAG, "PM2.5: %.2fµg/m³", pm2_5_->state);
  }
  
  // PM10.0 (2 bytes)
  if (pm10_0_ && pm10_0_->has_state()) {
    uint16_t pm10 = (uint16_t)(pm10_0_->state);
    payload[payload_index++] = pm10 >> 8;
    payload[payload_index++] = pm10 & 0xFF;
    ESP_LOGD(TAG, "PM10.0: %.2fµg/m³", pm10_0_->state);
  }
  
  // IAQ (2 bytes)
  if (iaq_ && iaq_->has_state()) {
    uint16_t iaq = (uint16_t)(iaq_->state);
    payload[payload_index++] = iaq >> 8;
    payload[payload_index++] = iaq & 0xFF;
    ESP_LOGD(TAG, "IAQ: %.2f", iaq_->state);
  }
  
  // Battery (2 bytes)
  if (battery_ && battery_->has_state()) {
    uint16_t batt = (uint16_t)(battery_->state * 100);
    payload[payload_index++] = batt >> 8;
    payload[payload_index++] = batt & 0xFF;
    ESP_LOGD(TAG, "Battery: %.2fV", battery_->state);
  }
  
  // Add sensor presence flags at the end
  payload[payload_index++] = sensor_flags >> 8;
  payload[payload_index++] = sensor_flags & 0xFF;

  // Log sensor presence flags
  ESP_LOGD(TAG, "Sensor presence flags: 0x%04X", sensor_flags);
  ESP_LOGD(TAG, "Available sensors: %d", available_sensors);
  ESP_LOGD(TAG, "Payload size: %d bytes", payload_size);

  // Log payload bytes
  ESP_LOGD(TAG, "Payload bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           payload[0], payload[1], payload[2], payload[3],
           payload[4], payload[5], payload[6], payload[7],
           payload[8], payload[9], payload[10], payload[11],
           payload[12], payload[13], payload[14], payload[15],
           payload[16], payload[17], payload[18], payload[19],
           payload[20], payload[21], payload[22], payload[23]);

  // Stop current advertising
  advertising_->stop();
  
  // Create advertising data
  NimBLEAdvertisementData advData;
  
  // Set flags (0x06 = General Discoverable Mode + BR/EDR Not Supported)
  advData.setFlags(0x06);
  
  // Set manufacturer data first
  std::string mfgData((char *)payload, payload_size);
  advData.setManufacturerData(mfgData);
  
  // Add service UUIDs for environmental sensing
  std::vector<NimBLEUUID> serviceUUIDs;
  serviceUUIDs.push_back(NimBLEUUID("181A"));  // Environmental Sensing Service
  serviceUUIDs.push_back(NimBLEUUID("181C"));  // User Data Service
  
  // Add services to advertising data
  for (const auto& uuid : serviceUUIDs) {
    advData.setCompleteServices(uuid);
  }
  
  // Set the advertising data
  advertising_->setAdvertisementData(advData);
  
  // Create scan response data with just the name
  NimBLEAdvertisementData scanResponse;
  scanResponse.setName("HTS-HOME");
  
  // Set the scan response data
  advertising_->setScanResponseData(scanResponse);
  
  // Start advertising
  advertising_->start();
  ESP_LOGD(TAG, "Advertising started with manufacturer data");
}

}  // namespace ble_advertiser
}  // namespace esphome
