#pragma once
#include <Arduino.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"
#include "IWeatherProvider.h"

class OpenMeteoClient : public IWeatherProvider {
public:
    bool update(const WeatherConfig& config, WeatherData& weatherData) override;

    static const char* conditionText(uint8_t weatherCode);

private:
    bool parseResponse(Stream& stream, WeatherData& weatherData, String& error);
};