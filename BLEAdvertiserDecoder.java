package dev.feruz;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class BLEAdvertiserDecoder {
    // Cache to store the most recent readings for each sensor
    private static final Map<String, SensorReading> sensorCache = new ConcurrentHashMap<>();

    public static class SensorReading {
        private final String label;
        private final double value;
        private final String unit;
        private final int precision;
        private final long timestamp;

        public SensorReading(String label, double value, String unit, int precision) {
            this.label = label;
            this.value = value;
            this.unit = unit;
            this.precision = precision;
            this.timestamp = System.currentTimeMillis();
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

        public long getTimestamp() {
            return timestamp;
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
        if (data == null || data.length < 2) {
            throw new IllegalArgumentException("Invalid manufacturer data");
        }

        // Verify company ID (0xFFFF)
        if (data[0] != (byte) 0xFF || data[1] != (byte) 0xFF) {
            throw new IllegalArgumentException("Invalid company ID");
        }

        int payloadIndex = 2; // Start after company ID

        // Read single sensor data
        if (payloadIndex + 4 >= data.length) {
            throw new IllegalArgumentException("Incomplete sensor data");
        }

        // Read label
        int labelLength = data[payloadIndex++] & 0xFF;
        if (payloadIndex + labelLength > data.length) {
            throw new IllegalArgumentException("Invalid label length");
        }
        String label = new String(data, payloadIndex, labelLength);
        payloadIndex += labelLength;

        // Read unit
        int unitLength = data[payloadIndex++] & 0xFF;
        if (payloadIndex + unitLength > data.length) {
            throw new IllegalArgumentException("Invalid unit length");
        }
        String unit = new String(data, payloadIndex, unitLength);
        payloadIndex += unitLength;

        // Read precision
        if (payloadIndex >= data.length) {
            throw new IllegalArgumentException("Missing precision");
        }
        int precision = data[payloadIndex++] & 0xFF;

        // Read value
        if (payloadIndex + 1 >= data.length) {
            throw new IllegalArgumentException("Missing value");
        }
        int rawValue = ((data[payloadIndex] & 0xFF) << 8) | (data[payloadIndex + 1] & 0xFF);

        // Calculate scale factor from precision (1/10^precision)
        double scaleFactor = 1.0;
        for (int i = 0; i < precision; i++) {
            scaleFactor *= 0.1;
        }

        double value = rawValue * scaleFactor;
        SensorReading reading = new SensorReading(label, value, unit, precision);
        
        // Update cache with new reading
        sensorCache.put(label, reading);

        // Return all cached readings as a complete dataset
        return new SensorData(new ArrayList<>(sensorCache.values()));
    }

    // Helper method to get the most recent reading for a specific sensor
    public static SensorReading getLatestReading(String label) {
        return sensorCache.get(label);
    }

    // Helper method to clear old readings (e.g., if a sensor hasn't updated in a while)
    public static void clearOldReadings(long maxAgeMs) {
        long now = System.currentTimeMillis();
        sensorCache.entrySet().removeIf(entry -> 
            (now - entry.getValue().getTimestamp()) > maxAgeMs);
    }
} 