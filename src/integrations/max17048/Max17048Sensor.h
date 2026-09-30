#pragma once

#include <Arduino.h>

struct FuelGaugeData;

class Max17048Sensor {
public:
    static constexpr uint8_t Address = 0x36;
    static constexpr uint8_t SdaPin = 21;
    static constexpr uint8_t SclPin = 22;
    static constexpr uint32_t I2cClockHz = 100000;

    bool begin(FuelGaugeData& data);
    bool update(FuelGaugeData& data);

    bool isInitialized() const { return _initialized; }
    uint16_t version() const { return _version; }

private:
    static constexpr uint8_t RegisterVCell = 0x02;
    static constexpr uint8_t RegisterSoc = 0x04;
    static constexpr uint8_t RegisterVersion = 0x08;
    static constexpr uint8_t RegisterCrate = 0x16;
    static constexpr uint8_t RegisterStatus = 0x1A;

    bool _initialized = false;
    uint16_t _version = 0;

    bool readWord(uint8_t reg, uint16_t& value);
    bool read(FuelGaugeData& data);
};
