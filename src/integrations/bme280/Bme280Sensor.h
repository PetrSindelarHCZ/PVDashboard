#pragma once

#include <Arduino.h>
#include <Adafruit_BME280.h>

struct InsideData;

class Bme280Sensor {
public:
    static constexpr uint8_t SdaPin = 21;
    static constexpr uint8_t SclPin = 22;
    static constexpr uint8_t PrimaryAddress = 0x76;
    static constexpr uint8_t SecondaryAddress = 0x77;
    static constexpr uint32_t I2cClockHz = 100000;

    bool begin(InsideData& data);
    bool update(InsideData& data);

    bool isInitialized() const { return _initialized; }
    uint8_t address() const { return _address; }

private:
    Adafruit_BME280 _sensor;
    bool _initialized = false;
    uint8_t _address = 0;

    bool read(InsideData& data);
};
