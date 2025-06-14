using System;
using System.Collections.Generic;
using System.Linq;

public class BLEAdvertiserDecoder
{
    public class SensorReading
    {
        public string Label { get; set; }
        public double Value { get; set; }
        public string Unit { get; set; }
        public int Precision { get; set; }
    }

    public static SensorReading Decode(byte[] data)
    {
        if (data == null || data.Length < 4)
        {
            throw new ArgumentException("Invalid advertisement data");
        }

        int index = 0;
        while (index < data.Length)
        {
            int length = data[index];
            if (length == 0) break;

            int type = data[index + 1];
            if (type == 0xFF)
            {
                byte[] manufacturerData = new byte[length - 1];
                Array.Copy(data, index + 2, manufacturerData, 0, length - 1);
                return DecodeManufacturerData(manufacturerData);
            }

            index += length + 1;
        }

        throw new ArgumentException("No manufacturer data found in advertisement");
    }

    public static SensorReading DecodeManufacturerData(byte[] data)
    {
        if (data == null || data.Length < 2)
        {
            throw new ArgumentException("Invalid manufacturer data");
        }

        if (data[0] != 0xFF || data[1] != 0xFF)
        {
            throw new ArgumentException("Invalid company ID");
        }

        int payloadIndex = 2;

        if (payloadIndex + 4 > data.Length)
        {
            throw new ArgumentException("Incomplete sensor data");
        }

        int labelLength = data[payloadIndex++];
        string label = System.Text.Encoding.UTF8.GetString(data, payloadIndex, labelLength);
        payloadIndex += labelLength;

        int unitLength = data[payloadIndex++];
        string unit = System.Text.Encoding.UTF8.GetString(data, payloadIndex, unitLength);
        payloadIndex += unitLength;

        int precision = data[payloadIndex++];

        if (payloadIndex + 3 > data.Length)
        {
            throw new ArgumentException("Incomplete sensor value");
        }

        int rawValue = (data[payloadIndex] << 24) |
                       (data[payloadIndex + 1] << 16) |
                       (data[payloadIndex + 2] << 8) |
                       data[payloadIndex + 3];

        double scaleFactor = Math.Pow(10, -precision);
        double value = rawValue * scaleFactor;

        return new SensorReading { Label = label, Value = value, Unit = unit, Precision = precision };
    }
}
