#include "Max17048Sensor.h"

#include <Wire.h>
#include <math.h>

#include "../../data/DataModel.h"

bool Max17048Sensor::begin(FuelGaugeData& data) {
    // BME280 initializes the shared I2C bus first during DashboardApp::setup().
    // Re-running Wire.begin() here on every retry would disturb another device
    // when the fuel gauge is disconnected, so only keep the common bus speed.
    Wire.setClock(I2cClockHz);

    _initialized = false;
    _version = 0;

    uint16_t versionWord = 0;
    if (!readWord(RegisterVersion, versionWord)) {
        data.status.recordError("MAX17048 nenalezen na I2C 0x36");
        Serial.printf(
            "[MAX17048] Nenalezen. I2C SDA=GPIO%u, SCL=GPIO%u, adresa=0x%02X.\n",
            SdaPin,
            SclPin,
            Address);
        return false;
    }

    _version = versionWord;
    data.version = _version;
    _initialized = true;

    Serial.printf(
        "[MAX17048] Inicializovan na adrese 0x%02X (verze=0x%04X, SDA=GPIO%u, SCL=GPIO%u).\n",
        Address,
        _version,
        SdaPin,
        SclPin);

    return read(data);
}

bool Max17048Sensor::update(FuelGaugeData& data) {
    if (!_initialized) {
        return begin(data);
    }
    return read(data);
}

bool Max17048Sensor::readWord(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(Address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;

    const uint8_t received = Wire.requestFrom(Address, static_cast<uint8_t>(2));
    if (received != 2 || Wire.available() < 2) return false;

    const uint8_t msb = Wire.read();
    const uint8_t lsb = Wire.read();
    value = (static_cast<uint16_t>(msb) << 8) | lsb;
    return true;
}

bool Max17048Sensor::read(FuelGaugeData& data) {
    uint16_t vcellRaw = 0;
    uint16_t socRaw = 0;
    uint16_t crateRaw = 0;
    uint16_t statusRaw = 0;

    if (!readWord(RegisterVCell, vcellRaw) ||
        !readWord(RegisterSoc, socRaw) ||
        !readWord(RegisterCrate, crateRaw) ||
        !readWord(RegisterStatus, statusRaw)) {
        data.status.recordError("MAX17048 I2C chyba");
        _initialized = false;
        Serial.println(
            "[MAX17048] Chyba cteni; pri dalsim pollu probehne nova inicializace.");
        return false;
    }

    // Datasheet scaling:
    // VCELL register = 78.125 uV per 16-bit LSB,
    // SOC = 1/256 %, CRATE = signed 0.208 %/h.
    const float voltageV =
        static_cast<float>(vcellRaw) * 78.125e-6f;
    const float socPercent =
        static_cast<float>(socRaw) / 256.0f;
    const float changeRatePercentPerHour =
        static_cast<float>(static_cast<int16_t>(crateRaw)) * 0.208f;

    if (!isfinite(voltageV) ||
        !isfinite(socPercent) ||
        !isfinite(changeRatePercentPerHour) ||
        voltageV < 0.0f ||
        voltageV > 5.5f ||
        socPercent < 0.0f ||
        socPercent > 256.0f) {
        data.status.recordError("MAX17048 vratil neplatna data");
        _initialized = false;
        Serial.println(
            "[MAX17048] Neplatna data; pri dalsim pollu probehne nova inicializace.");
        return false;
    }

    data.voltageV = voltageV;
    data.socPercent = socPercent;
    data.changeRatePercentPerHour = changeRatePercentPerHour;

    // Alert descriptor bits are in the STATUS register MSB:
    // bit0 RI, bit1 VH, bit2 VL, bit3 VR, bit4 HD, bit5 SC.
    data.alertFlags = static_cast<uint8_t>((statusRaw >> 8) & 0x3F);
    data.alertPending = (data.alertFlags & 0x3E) != 0;
    data.version = _version;
    data.lastUpdateMs = millis();
    data.status.recordSuccess();

    Serial.printf(
        "[MAX17048] U=%.3f V, SOC=%.1f %%, dSOC=%+.2f %%/h, alerts=0x%02X\n",
        data.voltageV,
        data.socPercent,
        data.changeRatePercentPerHour,
        data.alertFlags);
    return true;
}
