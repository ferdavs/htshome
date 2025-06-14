# ESPHome BLE Advertiser Component

## Description

This ESPHome component allows you to advertise sensor data over Bluetooth Low Energy (BLE). It is designed to be used with ESPHome and the NimBLE library.

## Usage

To use this component, you need to add it to your ESPHome configuration file (`htshome.yaml`). You can configure the advertising parameters, such as the update interval, advertising interval, device name, power level, and sensors to advertise.

## Installation

To use this component, add the following to your `htshome.yaml` file:

```yaml
external_components:
  - source: "./custom_components"
    components: ["ble_advertiser"]
```

## Configuration

The following configuration options are available:

*   **update\_interval** (Optional, Time): The interval at which the BLE advertisement is updated. Defaults to `2s`. Example: `2s`, `5s`, `10s`.
*   **min\_interval** (Optional, number): Minimum advertising interval in 0.625 ms units. Defaults to `400` (250ms). Example: `32`, `400`, `800`.
*   **max\_interval** (Optional, number): Maximum advertising interval in 0.625 ms units. Defaults to `800` (500ms). Example: `64`, `800`, `1280`.
*   **device\_name** (Optional, string): The name of the BLE device. Defaults to `HTS-HOME`. Example: `"HTS-HOME"`, `"My Sensor"`.
*   **power\_level** (Optional, int): The power level of the BLE advertisement. Defaults to `9`. Possible values: `0` to `9`.
*   **sensors** (Optional, list): A list of sensors to advertise. Each sensor must have the following options:
    *   **id** (Required, ID): The ID of the sensor.
    *   **label** (Required, string): The label of the sensor.
    *   **unit** (Optional, string): The unit of the sensor.
    *   **precision** (Optional, int): The number of decimal places to display for the sensor value. Defaults to 0.

## Example

```yaml
ble_advertiser:
  update_interval: 2s
  min_interval: 400
  max_interval: 800
  device_name: "HTS-HOME"
  power_level: 9
  sensors:
    - id: breath_voc
      label: VOC
      unit: ppm
      precision: 1
```

## License

This project is licensed under the MIT License.

## Example Decoder Implementations

*   Java/Android: See the `BLEAdvertiserDecoder.java` file.
*   Python: See the `ble_advertiser_decoder.py` file.
*   C++: See the `ble_advertiser_decoder.cpp` and `ble_advertiser_decoder.h` files.
*   C#: See the `BLEAdvertiserDecoder.cs` file.
