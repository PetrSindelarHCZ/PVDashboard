#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/DisplayTaskStatus.h"
#include "../screens/ScreenManager.h"
#include "../update/OtaManager.h"

class DashboardWebServer {
public:
    using ScreenChangeCallback = std::function<void(const String& screenId)>;
    using RefreshCallback = std::function<void(bool full)>;
    using DisplayStatusCallback = std::function<DisplayTaskStatus()>;
    using SystemConfigCallback = std::function<void(const SystemConfig& system)>;
    using WifiConfigCallback = std::function<void(const String& ssid, const String& password)>;
    using WifiNetworkConfigCallback = std::function<void(const WifiConfig& wifi)>;
    using WifiScanCallback = std::function<String()>;
    using WifiKnownNetworksCallback = std::function<String()>;
    using WifiKnownNetworkActionCallback = std::function<bool(const String& ssid)>;
    using WifiDisconnectCallback = std::function<void()>;
    using SourceConfigCallback = std::function<void(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter)>;
    using WeatherConfigCallback = std::function<void(const WeatherConfig& weather)>;
    using FactoryResetCallback = std::function<bool()>;
    using ConfigImportCallback = std::function<bool(const AppConfig& config)>;

    DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager, const AppConfig& config);

    void begin();
    void enableTimezoneUiExtension();
    void loop();
    void onScreenChange(ScreenChangeCallback callback);
    void onRefresh(RefreshCallback callback);
    void onDisplayStatus(DisplayStatusCallback callback);
    void onSystemConfig(SystemConfigCallback callback);
    void onWifiConfig(WifiConfigCallback callback);
    void onWifiNetworkConfig(WifiNetworkConfigCallback callback) {
        _wifiNetworkConfigCallback = callback;

        _server.on("/api/network/config", HTTP_GET, [this]() {
            const auto& wifi = _config.wifi;
            JsonDocument doc;
            doc["dhcp"] = wifi.dhcp;
            doc["ipAddress"] = wifi.ipAddress;
            doc["subnetMask"] = wifi.subnetMask;
            doc["gateway"] = wifi.gateway;
            doc["dns1"] = wifi.dns1;
            doc["dns2"] = wifi.dns2;
            String response;
            serializeJson(doc, response);
            _server.sendHeader("Cache-Control", "no-store");
            _server.send(200, "application/json", response);
        });

        _server.on("/api/network/config", HTTP_POST, [this]() {
            if (!_wifiNetworkConfigCallback || !_server.hasArg("mode")) {
                _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Network configuration unavailable\"}");
                return;
            }

            WifiConfig wifi = _config.wifi;
            const String mode = _server.arg("mode");
            if (mode != "dhcp" && mode != "static") {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Neplatny rezim IP konfigurace\"}");
                return;
            }
            wifi.dhcp = mode == "dhcp";
            wifi.ipAddress = _server.arg("ipAddress");
            wifi.subnetMask = _server.arg("subnetMask");
            wifi.gateway = _server.arg("gateway");
            wifi.dns1 = _server.arg("dns1");
            wifi.dns2 = _server.arg("dns2");
            wifi.ipAddress.trim(); wifi.subnetMask.trim(); wifi.gateway.trim(); wifi.dns1.trim(); wifi.dns2.trim();

            auto validIp = [](const String& value, bool allowEmpty) {
                if (value.isEmpty()) return allowEmpty;
                IPAddress address;
                return address.fromString(value) && address != IPAddress(0, 0, 0, 0);
            };
            auto validMask = [](const String& value, bool allowEmpty) {
                if (value.isEmpty()) return allowEmpty;
                IPAddress mask;
                if (!mask.fromString(value)) return false;
                bool zeroSeen = false;
                bool oneSeen = false;
                for (int octet = 0; octet < 4; ++octet) {
                    const uint8_t byte = mask[octet];
                    for (int bit = 7; bit >= 0; --bit) {
                        const bool one = (byte & (1 << bit)) != 0;
                        if (one) {
                            oneSeen = true;
                            if (zeroSeen) return false;
                        } else {
                            zeroSeen = true;
                        }
                    }
                }
                return oneSeen;
            };

            const bool valid = wifi.dhcp
                ? validIp(wifi.ipAddress, true) && validMask(wifi.subnetMask, true) &&
                  validIp(wifi.gateway, true) && validIp(wifi.dns1, true) && validIp(wifi.dns2, true)
                : validIp(wifi.ipAddress, false) && validMask(wifi.subnetMask, false) &&
                  validIp(wifi.gateway, false) && validIp(wifi.dns1, false) && validIp(wifi.dns2, true);
            if (!valid) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Neplatna staticka IPv4 konfigurace\"}");
                return;
            }

            _server.send(200, "application/json", "{\"status\":\"saved\"}");
            _wifiNetworkConfigCallback(wifi);
        });
    }
    void onWifiScan(WifiScanCallback callback);
    void onWifiKnownNetworks(WifiKnownNetworksCallback callback);
    void onWifiConnectKnown(WifiKnownNetworkActionCallback callback);
    void onWifiForget(WifiKnownNetworkActionCallback callback);
    void onWifiDisconnect(WifiDisconnectCallback callback);
    void onSourceConfig(SourceConfigCallback callback);
    void onWeatherConfig(WeatherConfigCallback callback);
    void onFactoryReset(FactoryResetCallback callback);
    void onConfigImport(ConfigImportCallback callback);

private:
    WebServer _server;
    DataModel& _dataModel;
    ScreenManager& _screenManager;
    const AppConfig& _config;
    ScreenChangeCallback _screenCallback;
    RefreshCallback _refreshCallback;
    DisplayStatusCallback _displayStatusCallback;
    SystemConfigCallback _systemConfigCallback;
    WifiConfigCallback _wifiConfigCallback;
    WifiNetworkConfigCallback _wifiNetworkConfigCallback;
    WifiScanCallback _wifiScanCallback;
    WifiKnownNetworksCallback _wifiKnownNetworksCallback;
    WifiKnownNetworkActionCallback _wifiConnectKnownCallback;
    WifiKnownNetworkActionCallback _wifiForgetCallback;
    WifiDisconnectCallback _wifiDisconnectCallback;
    SourceConfigCallback _sourceConfigCallback;
    WeatherConfigCallback _weatherConfigCallback;
    FactoryResetCallback _factoryResetCallback;
    ConfigImportCallback _configImportCallback;
    OtaManager _otaManager;
    String _githubUpdateVersion;
    String _githubUpdateUrl;
    String _githubUpdateSha256;
    bool _timezoneUiEnabled = false;

    void beginLegacy();
    void setupRoutes();
    void handleRoot();
    void handleExtendedRoot();
    void handleApiTimezoneConfig();
    void handleApiSystemConfigV2();
    void loopLegacy();
    void handleApiStatus();
    void handleApiScreens();
    void handleApiActivateScreen(const String& screenId);
    void handleApiRefresh(bool full);
    void handleApiRestart();
    void handleApiFactoryReset();
    void handleApiConfigExport();
    void handleApiConfigImport();
    void handleApiSystemConfig();
    void handleApiWifiConfig();
    void handleApiWifiScan();
    void handleApiSourceConfig();
    void handleApiWeatherConfig();
    void handleApiCheckForUpdate();
    void handleApiGithubUpdate();
    void handleApiUpdateUpload();
    void handleApiUpdateComplete();
};
