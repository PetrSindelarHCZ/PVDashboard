#pragma once
#include <Arduino.h>
#include "IWeatherProvider.h"

class MetNorwayClient : public IWeatherProvider {
public:
    bool update(const WeatherConfig& config, WeatherData& weatherData) override;
    uint32_t recommendedPollIntervalSeconds(uint32_t configuredSeconds) const override;

private:
    String _lastModified;
    uint32_t _cacheSeconds = 0;

    bool parseResponse(Stream& stream, WeatherData& weatherData, String& error);
    void updateCachePolicy(const String& dateHeader, const String& expiresHeader);
};