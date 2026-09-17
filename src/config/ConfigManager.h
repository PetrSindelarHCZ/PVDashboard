#pragma once
#include <Arduino.h>
#include <vector>
#include "ConfigSchema.h"

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    const AppConfig& get() const;
    void setSystem(const SystemConfig& system);
    void setWifi(const String& ssid, const String& password);
    String getKnownWifiNetworksJson() const;
    bool getKnownWifiPassword(const String& ssid, String& password) const;
    bool forgetWifi(const String& ssid);
    void setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter);
    void setWeather(const WeatherConfig& weather);
    bool resetToFactoryDefaults();
    bool setUserConfiguration(const AppConfig& config);

private:
    struct KnownWifiNetwork {
        String ssid;
        String password;
    };

    static constexpr size_t MaxKnownWifiNetworks = 8;

    AppConfig _config;
    std::vector<KnownWifiNetwork> _knownWifiNetworks;

    void rememberWifi(const String& ssid, const String& password);
    void saveKnownWifiNetworks();
};
