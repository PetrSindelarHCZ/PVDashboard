#pragma once
#include <Arduino.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"

class OpenMeteoClient {
public:
    bool update(const WeatherConfig& config, WeatherData& weatherData);

    static const char* conditionText(uint8_t weatherCode);

private:
    bool parseResponse(Stream& stream, WeatherData& weatherData, String& error);
};