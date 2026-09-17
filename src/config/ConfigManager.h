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
    void setWifiNetworkConfig(const WifiConfig& wifi);
    String getKnownWifiNetworksJson() const;
    bool getKnownWifiPassword(const String& ssid, String& password) const;
    bool getAutoJoinWifiPassword(const String& ssid, String& password) const;
    bool getAutoJoinWifiNetworkAt(size_t enabledIndex, String& ssid, String& password) const;
    bool isWifiAutoConnectEnabled(const String& ssid) const;
    bool setWifiAutoConnectEnabled(const String& ssid, bool enabled);
    bool forgetWifi(const String& ssid);
    void setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter);
    void setWeather(const WeatherConfig& weather);
    bool resetToFactoryDefaults();
    bool setUserConfiguration(const AppConfig& config);

private:
    struct KnownWifiNetwork {
        String ssid;
        String password;
        bool autoConnect;

        KnownWifiNetwork(const String& networkSsid, const String& networkPassword, bool networkAutoConnect = true)
            : ssid(networkSsid), password(networkPassword), autoConnect(networkAutoConnect) {
        }
    };

    static constexpr size_t MaxKnownWifiNetworks = 8;

    AppConfig _config;
    std::vector<KnownWifiNetwork> _knownWifiNetworks;

    void rememberWifi(const String& ssid, const String& password, bool enableAutoConnect = true);
    void saveKnownWifiNetworks();
};

inline bool ConfigManager::getAutoJoinWifiNetworkAt(size_t enabledIndex, String& ssid, String& password) const {
    size_t current = 0;
    for (const auto& network : _knownWifiNetworks) {
        if (!network.autoConnect) continue;
        if (current++ != enabledIndex) continue;
        ssid = network.ssid;
        password = network.password;
        return true;
    }
    return false;
}
