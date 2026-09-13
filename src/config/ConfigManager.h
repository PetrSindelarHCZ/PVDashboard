#pragma once
#include <Arduino.h>
#include "ConfigSchema.h"

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    const AppConfig& get() const;
    void setWifi(const String& ssid, const String& password);
    void setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter);

private:
    AppConfig _config;
};
