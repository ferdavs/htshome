package dev.feruz;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

/**
 * This class provides methods to decode BLE advertisement data
 * from the ESPHome BLE Advertiser component.
 */
public class BLEAdvertiserDecoder {

    /**
     * Represents a sensor reading extracted from the advertisement data.
     */
    public static class SensorReading {
        private final String label;
        private final double value;
        private final String unit;
        private final int precision;
        private final long timestamp;

        /**
         * Constructs a new SensorReading object.
         *
         * @param label     The label of the sensor.
         * @param value     The value of the sensor.
         * @param unit      The unit of the sensor.
         * @param precision The precision of the sensor.
         */
        public SensorReading(String label, double value, String unit, int precision) {
            this.label = label;
            this.value = value;
            this.unit = unit;
            this.precision = precision;
            this.timestamp = System.currentTimeMillis();
        }

        /**
         * Returns the label of the sensor.
         *
         * @return The label of the sensor.
         */
        public String getLabel() {
            return label;
        }

        /**
         * Returns the value of the sensor.
         *
         * @return The value of the sensor.
         */
        public double getValue() {
            return value;
        }

        /**
         * Returns the unit of the sensor.
         *
         * @return The unit of the sensor.
         */
        public String getUnit() {
            return unit;
        }

        /**
         * Returns the precision of the sensor.
         *
         * @return The precision of the sensor.
         */
        public int getPrecision() {
            return precision;
        }

        /**
         * Returns the timestamp of the sensor reading.
         *
         * @return The timestamp of the sensor reading.
         */
        public long getTimestamp() {
            return timestamp;
        }

        /**
         * Returns the formatted value of the sensor.
         *
         * @return The formatted value of the sensor.
         */
        public String getFormattedValue() {
            if (unit.equals("s")) {
                return formatTimeValue(value);
            }
            return String.format(Locale.getDefault(), "%." + precision + "f%s", value, unit);
        }

        /**
         * Formats the time value in seconds to a human-readable string.
         *
         * @param seconds The time value in seconds.
         * @return The formatted time value.
         */
        private String formatTimeValue(double seconds) {
            if (seconds < 60) {
                return String.format(Locale.getDefault(), "%.0fs", seconds);
            }

            long totalSeconds = (long) seconds;
            long days = TimeUnit.SECONDS.toDays(totalSeconds);
            long hours = TimeUnit.SECONDS.toHours(totalSeconds) % 24;
            long minutes = TimeUnit.SECONDS.toMinutes(totalSeconds) % 60;
            long remainingSeconds = totalSeconds % 60;

            StringBuilder result = new StringBuilder();
            if (days > 0) {
                result.append(days)
                      .append("d ");
            }
            if (hours > 0 || days > 0) {
                result.append(hours)
                      .append("h ");
            }
            if (minutes > 0 || hours > 0 || days > 0) {
                result.append(minutes)
                      .append("m ");
            }
            if (remainingSeconds > 0 || result.length() == 0) {
                result.append(remainingSeconds)
                      .append("s");
            }

            return result.toString()
                         .trim();
        }

        @NonNull
        @Override
        public String toString() {
            return label + ": " + getFormattedValue();
        }
    }

    /**
     * Decodes the advertisement data and extracts the sensor reading.
     *
     * @param data The advertisement data.
     * @return The sensor reading.
     * @throws IllegalArgumentException If the advertisement data is invalid.
     */
    public static SensorReading decode(byte[] data) {
        if (data == null || data.length < 4) {
            throw new IllegalArgumentException("Invalid advertisement data");
        }

        // Find manufacturer data in the advertisement packet
        int index = 0;
        while (index < data.length) {
            // Read the length of the current AD structure
            int length = data[index] & 0xFF;
            if (length == 0) break;

            // Read the AD type
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

    /**
     * Decodes the manufacturer data and extracts the sensor reading.
     *
     * @param data The manufacturer data.
     * @return The sensor reading.
     * @throws IllegalArgumentException If the manufacturer data is invalid.
     */
    public static SensorReading decodeManufacturerData(byte[] data) {
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

        int labelLength = data[payloadIndex++] & 0xFF;
        if (payloadIndex + labelLength > data.length) {
            throw new IllegalArgumentException("Invalid label length");
        }
        String label = new String(data, payloadIndex, labelLength);
        payloadIndex += labelLength;

        int unitLength = data[payloadIndex++] & 0xFF;
        if (payloadIndex + unitLength > data.length) {
            throw new IllegalArgumentException("Invalid unit length");
        }
        String unit = new String(data, payloadIndex, unitLength);
        payloadIndex += unitLength;

        if (payloadIndex >= data.length) {
            throw new IllegalArgumentException("Missing precision");
        }
        int precision = data[payloadIndex++] & 0xFF;

        // Read value
        if (payloadIndex + 3 >= data.length) {
            throw new IllegalArgumentException("Missing value");
        }
        int rawValue = ((data[payloadIndex] & 0xFF) << 24) |
                ((data[payloadIndex + 1] & 0xFF) << 16) |
                ((data[payloadIndex + 2] & 0xFF) << 8) |
                (data[payloadIndex + 3] & 0xFF);

        // Calculate scale factor from precision (10^-precision)
        double scaleFactor = Math.pow(10, -precision);
        double value = rawValue * scaleFactor;
        return new SensorReading(label, value, unit, precision);
    }
}
