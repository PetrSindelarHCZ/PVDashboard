#pragma once

#include <Arduino.h>

constexpr uint8_t MaxRfSensors = 16;
constexpr uint8_t MaxRfDiscoveredSensors = 24;

struct RfSensorObservation {
    String protocol;
    uint32_t sensorId = 0;
    uint8_t channel = 0;

    bool hasTemperature = false;
    float temperatureC = 0.0f;

    bool hasHumidity = false;
    int humidityPercent = 0;

    bool hasBattery = false;
    bool batteryOk = true;

    String bindingKey() const {
        String key = protocol;
        key += ":";
        key += String(sensorId, HEX);
        key += ":";
        key += String(channel);
        return key;
    }
};
