package dev.feruz;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

public class BLEAdvertiserDecoder {
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
            if (unit.equals("s")) {
                return formatTimeValue(value);
            }
            return String.format(Locale.getDefault(), "%." + precision + "f%s", value, unit);
        }

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

    public static SensorReading decode(byte[] data) {
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
