#include "Bme280Sensor.h"

#include <Wire.h>
#include <math.h>

#include "../../data/DataModel.h"

bool Bme280Sensor::begin(InsideData& data) {
    Wire.begin(SdaPin, SclPin);
    Wire.setClock(I2cClockHz);

    _initialized = false;
    _address = 0;

    if (_sensor.begin(PrimaryAddress, &Wire)) {
        _address = PrimaryAddress;
    } else if (_sensor.begin(SecondaryAddress, &Wire)) {
        _address = SecondaryAddress;
    } else {
        data.status.recordError("BME280 nenalezen na I2C 0x76/0x77");
        Serial.printf(
            "[BME280] Nenalezen. I2C SDA=GPIO%u, SCL=GPIO%u; zkouseno 0x%02X a 0x%02X.\n",
            SdaPin,
            SclPin,
            PrimaryAddress,
            SecondaryAddress);
        return false;
    }

    _sensor.setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::FILTER_X4,
        Adafruit_BME280::STANDBY_MS_1000);

    _initialized = true;
    Serial.printf(
        "[BME280] Inicializovan na adrese 0x%02X (SDA=GPIO%u, SCL=GPIO%u).\n",
        _address,
        SdaPin,
        SclPin);

    return read(data);
}

bool Bme280Sensor::update(InsideData& data) {
    if (!_initialized) {
        return begin(data);
    }
    return read(data);
}

bool Bme280Sensor::read(InsideData& data) {
    const float temperatureC = _sensor.readTemperature();
    const float humidityPercent = _sensor.readHumidity();
    const float pressureHpa = _sensor.readPressure() / 100.0f;

    if (!isfinite(temperatureC) ||
        !isfinite(humidityPercent) ||
        !isfinite(pressureHpa) ||
        humidityPercent < 0.0f ||
        humidityPercent > 100.0f ||
        pressureHpa < 300.0f ||
        pressureHpa > 1200.0f) {
        data.status.recordError("BME280 vratil neplatna data");
        _initialized = false;
        Serial.println("[BME280] Neplatna data; pri dalsim pollu probehne nova inicializace.");
        return false;
    }

    data.temperatureC = temperatureC;
    data.humidityPercent = static_cast<int>(lroundf(humidityPercent));
    data.pressureHpa = pressureHpa;

    // Soucasny Home layout pouziva pro prvni vnitrni teplotu toto pole.
    // Zachovame kompatibilitu bez zavislosti rendereru na konkretnim senzoru.
    data.livingRoomTempC = temperatureC;

    data.lastUpdateMs = millis();
    data.status.recordSuccess();

    Serial.printf(
        "[BME280] T=%.2f C, RH=%d %%, P=%.1f hPa\n",
        data.temperatureC,
        data.humidityPercent,
        data.pressureHpa);
    return true;
}
