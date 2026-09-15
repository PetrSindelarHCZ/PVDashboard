#pragma once
#include <Arduino.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;
    virtual bool update(const WeatherConfig& config, WeatherData& weatherData) = 0;
    virtual uint32_t recommendedPollIntervalSeconds(uint32_t configuredSeconds) const {
        return configuredSeconds;
    }
};