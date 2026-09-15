#pragma once
#include <Arduino.h>
#include "ConfigSchema.h"

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    const AppConfig& get() const;
    void setSystem(const SystemConfig& system);
    void setWifi(const String& ssid, const String& password);
    void setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter);
    void setWeather(const WeatherConfig& weather);
    bool resetToFactoryDefaults();
    bool setUserConfiguration(const AppConfig& config);

private:
    AppConfig _config;
};
