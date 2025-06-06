package dev.feruz;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BLEAdvertiserDecoder {
    public static class SensorReading {
        private final String label;
        private final double value;
        private final String unit;
        private final int precision;

        public SensorReading(String label, double value, String unit, int precision) {
            this.label = label;
            this.value = value;
            this.unit = unit;
            this.precision = precision;
        }

        public String getLabel() {
            return label;
        }

        public double getValue() {
            return value;
        }

        public String getUnit() {
            return unit;
        }

        public int getPrecision() {
            return precision;
        }

        public String getFormattedValue() {
            return String.format("%." + precision + "f%s", value, unit);
        }

        @Override
        public String toString() {
            return label + ": " + getFormattedValue();
        }
    }

    public static class SensorData {
        private final List<SensorReading> readings;

        public SensorData(List<SensorReading> readings) {
            this.readings = readings;
        }

        public List<SensorReading> getReadings() {
            return readings;
        }

        public SensorReading getReading(String label) {
            return readings.stream()
                    .filter(r -> r.getLabel().equals(label))
                    .findFirst()
                    .orElse(null);
        }

        // Standard sensor accessors
        public Double getTemperature() {
            SensorReading reading = getReading("Temperature");
            return reading != null ? reading.getValue() : null;
        }

        public Double getHumidity() {
            SensorReading reading = getReading("Humidity");
            return reading != null ? reading.getValue() : null;
        }

        public Double getPressure() {
            SensorReading reading = getReading("Pressure");
            return reading != null ? reading.getValue() : null;
        }

        public Double getCO2() {
            SensorReading reading = getReading("CO2");
            return reading != null ? reading.getValue() : null;
        }

        public Double getPM1_0() {
            SensorReading reading = getReading("PM1.0");
            return reading != null ? reading.getValue() : null;
        }

        public Double getPM2_5() {
            SensorReading reading = getReading("PM2.5");
            return reading != null ? reading.getValue() : null;
        }

        public Double getPM10_0() {
            SensorReading reading = getReading("PM10.0");
            return reading != null ? reading.getValue() : null;
        }

        public Double getIAQ() {
            SensorReading reading = getReading("IAQ");
            return reading != null ? reading.getValue() : null;
        }

        public Double getBatteryVoltage() {
            SensorReading reading = getReading("Battery");
            return reading != null ? reading.getValue() : null;
        }

        // Helper method to get formatted value with unit
        public String getFormattedValue(String label) {
            SensorReading reading = getReading(label);
            return reading != null ? reading.getFormattedValue() : null;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("SensorData [");
            for (int i = 0; i < readings.size(); i++) {
                if (i > 0) sb.append(", ");
                sb.append(readings.get(i));
            }
            sb.append("]");
            return sb.toString();
        }
    }

    public static SensorData decode(byte[] data) {
        if (data == null || data.length < 4) {
            throw new IllegalArgumentException("Invalid advertisement data");
        }

        // Find manufacturer data in the advertisement packet
        int index = 0;
        while (index < data.length) {
            int length = data[index] & 0xFF;
            if (length == 0) break;
            
            int type = data[index + 1] & 0xFF;
            if (type == 0xFF) { // Manufacturer Specific Data
                // Extract manufacturer data (skip length and type bytes)
                byte[] manufacturerData = new byte[length - 1];
                System.arraycopy(data, index + 2, manufacturerData, 0, length - 1);
                return decodeManufacturerData(manufacturerData);
            }
            
            index += length + 1;
        }
        
        throw new IllegalArgumentException("No manufacturer data found in advertisement");
    }

    public static SensorData decodeManufacturerData(byte[] data) {
        if (data == null || data.length < 4) {
            throw new IllegalArgumentException("Invalid manufacturer data");
        }

        // Verify company ID (0xFFFF)
        if (data[0] != (byte) 0xFF || data[1] != (byte) 0xFF) {
            throw new IllegalArgumentException("Invalid company ID");
        }

        // Read sensor count
        int sensorCount = ((data[2] & 0xFF) << 8) | (data[3] & 0xFF);
        if (sensorCount <= 0 || sensorCount > 10) { // Sanity check
            throw new IllegalArgumentException("Invalid sensor count: " + sensorCount);
        }

        List<SensorReading> readings = new ArrayList<>();
        int payloadIndex = 4; // Start after company ID and sensor count

        // Default sensor configurations (can be overridden by actual data)
        Map<String, SensorConfig> sensorConfigs = new HashMap<>();
        sensorConfigs.put("Temperature", new SensorConfig("°C", 1, 0.01));
        sensorConfigs.put("Humidity", new SensorConfig("%", 1, 0.01));
        sensorConfigs.put("Pressure", new SensorConfig("hPa", 1, 0.01));
        sensorConfigs.put("CO2", new SensorConfig("ppm", 0, 0.01));
        sensorConfigs.put("PM1.0", new SensorConfig("µg/m³", 1, 0.01));
        sensorConfigs.put("PM2.5", new SensorConfig("µg/m³", 1, 0.01));
        sensorConfigs.put("PM10.0", new SensorConfig("µg/m³", 1, 0.01));
        sensorConfigs.put("IAQ", new SensorConfig("", 0, 0.01));
        sensorConfigs.put("Battery", new SensorConfig("V", 2, 0.01));

        // Read sensor values
        for (int i = 0; i < sensorCount && payloadIndex + 1 < data.length; i++) {
            int rawValue = ((data[payloadIndex] & 0xFF) << 8) | (data[payloadIndex + 1] & 0xFF);
            payloadIndex += 2;

            // Get sensor configuration (you might want to get this from the actual data)
            String label = getSensorLabel(i);
            SensorConfig config = sensorConfigs.getOrDefault(label, new SensorConfig("", 2, 1.0));
            
            double value = rawValue * config.scaleFactor;
            readings.add(new SensorReading(label, value, config.unit, config.precision));
        }

        return new SensorData(readings);
    }

    private static String getSensorLabel(int index) {
        // This should match the order of sensors in the ESPHome configuration
        String[] labels = {
            "Temperature", "Humidity", "Pressure", "CO2",
            "PM1.0", "PM2.5", "PM10.0", "IAQ", "Battery"
        };
        return index < labels.length ? labels[index] : "Sensor" + index;
    }

    private static class SensorConfig {
        final String unit;
        final int precision;
        final double scaleFactor;

        SensorConfig(String unit, int precision, double scaleFactor) {
            this.unit = unit;
            this.precision = precision;
            this.scaleFactor = scaleFactor;
        }
    }
} 