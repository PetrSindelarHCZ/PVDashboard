#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <functional>
#include <freertos/semphr.h>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/DisplayTaskStatus.h"
#include "../screens/ScreenManager.h"
#include "../update/OtaManager.h"
#include "NetworkDiagnostics.h"

class DisplayPreview;
class NavigationController;

class DashboardWebServer {
public:
    using ScreenChangeCallback = std::function<void(const String& screenId)>;
    using RefreshCallback = std::function<void(bool full)>;
    using DisplayStatusCallback = std::function<DisplayTaskStatus()>;
    using SystemConfigCallback = std::function<void(const SystemConfig& system)>;
    using WifiConfigCallback = std::function<void(const String& ssid, const String& password)>;
    using WifiNetworkConfigCallback = std::function<void(const WifiConfig& wifi)>;
    using SystemNetworkConfigCallback = std::function<void(const SystemConfig& system, const WifiConfig& wifi)>;
    using WifiScanCallback = std::function<String()>;
    using WifiKnownNetworksCallback = std::function<String()>;
    using WifiKnownNetworkActionCallback = std::function<bool(const String& ssid)>;
    using WifiDisconnectCallback = std::function<void()>;
    using SourceConfigCallback = std::function<void(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter)>;
    using PoolConfigCallback = std::function<void(const PoolConfig& pool)>;
    using WeatherConfigCallback = std::function<void(const WeatherConfig& weather)>;
    using HomeLayoutConfigCallback = std::function<bool(const HomeLayoutConfig& layout)>;
    using FactoryResetCallback = std::function<bool()>;
    using ConfigImportCallback = std::function<bool(const AppConfig& config)>;
    using RfSensorStatusCallback = std::function<String()>;
    using RfSensorScanCallback = std::function<void(uint32_t durationMs)>;
    using RfSensorAddCallback =
        std::function<bool(const String& bindingKey, const String& name, String& error)>;
    using RfSensorRenameCallback =
        std::function<bool(const String& slotId, const String& name, String& error)>;
    using RfSensorRemoveCallback =
        std::function<bool(const String& slotId, String& error)>;

    DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager, const AppConfig& config);

    void begin();
    void enableTimezoneUiExtension();
    void loop();
    void onScreenChange(ScreenChangeCallback callback);
    void onRefresh(RefreshCallback callback);
    void onDisplayStatus(DisplayStatusCallback callback);
    void setDisplayPreview(DisplayPreview* preview) { _displayPreview = preview; }
    void setNavigationController(NavigationController* controller) { _navigationController = controller; }
    void setNetworkClientGate(SemaphoreHandle_t gate) { _networkClientGate = gate; }
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

            const bool leaseAvailable = WiFi.status() == WL_CONNECTED;
            doc["leaseAvailable"] = leaseAvailable;
            if (leaseAvailable) {
                doc["currentIpAddress"] = WiFi.localIP().toString();
                doc["currentSubnetMask"] = WiFi.subnetMask().toString();
                doc["currentGateway"] = WiFi.gatewayIP().toString();
                doc["currentDns1"] = WiFi.dnsIP(0).toString();
                doc["currentDns2"] = WiFi.dnsIP(1).toString();
            }

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

        _server.on("/api/sources/test", HTTP_POST, [this]() {
            if (!_server.hasArg("source") || !_server.hasArg("host") || !_server.hasArg("port")) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Chybi zdroj, host nebo port\"}");
                return;
            }
            const String source = _server.arg("source");
            String host = _server.arg("host");
            host.trim();
            const long parsedPort = _server.arg("port").toInt();
            if (host.isEmpty() || parsedPort < 1 || parsedPort > 65535 || (source != "goodwe" && source != "azrouter")) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Neplatny cil testu\"}");
                return;
            }
            if (!_dataModel.system.wifiConnected) {
                _server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Wi-Fi neni pripojena\"}");
                return;
            }

            const uint16_t port = static_cast<uint16_t>(parsedPort);
            const NetworkProbeTransport transport = source == "goodwe"
                ? NetworkProbeTransport::GoodWeUdp
                : NetworkProbeTransport::Tcp;
            const NetworkProbeResult probe = NetworkDiagnostics::probe(host, port, transport);

            JsonDocument doc;
            doc["status"] = "ok";
            doc["source"] = source;
            doc["host"] = host;
            doc["port"] = port;
            doc["tested"] = true;
            doc["resolved"] = probe.resolved;
            doc["resolvedIp"] = probe.resolvedIp;
            doc["pingOk"] = probe.pingOk;
            doc["pingMs"] = probe.pingMs;
            doc["portOpen"] = probe.portOpen;
            doc["portMs"] = probe.portConnectMs;
            doc["portProtocol"] = probe.portProtocol;
            doc["message"] = probe.error;
            String response;
            serializeJson(doc, response);
            _server.sendHeader("Cache-Control", "no-store");
            _server.send(200, "application/json", response);
        });

        if (!_systemNetworkConfigCallback) {
            onSystemNetworkConfig([this](const SystemConfig& system, const WifiConfig& wifi) {
                const auto& currentSystem = _config.system;
                const auto& currentWifi = _config.wifi;
                const bool systemChanged = currentSystem.hostname != system.hostname ||
                                           currentSystem.ntpServer != system.ntpServer ||
                                           currentSystem.timezone != system.timezone ||
                                           currentSystem.timezoneId != system.timezoneId;
                const bool networkChanged = currentWifi.dhcp != wifi.dhcp ||
                                            currentWifi.ipAddress != wifi.ipAddress ||
                                            currentWifi.subnetMask != wifi.subnetMask ||
                                            currentWifi.gateway != wifi.gateway ||
                                            currentWifi.dns1 != wifi.dns1 ||
                                            currentWifi.dns2 != wifi.dns2;
                if (networkChanged && _wifiNetworkConfigCallback) _wifiNetworkConfigCallback(wifi);
                if (systemChanged && _systemConfigCallback) _systemConfigCallback(system);
            });
        }
    }

    void onSystemNetworkConfig(SystemNetworkConfigCallback callback) {
        _systemNetworkConfigCallback = callback;
        _server.on("/api/config/system-network", HTTP_POST, [this]() {
            if (!_systemNetworkConfigCallback) {
                _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"System configuration unavailable\"}");
                return;
            }

            const char* required[] = {"hostname", "ntpServer", "timezone", "timezoneId", "mode"};
            for (const char* name : required) {
                if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
                    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Chybi povinne nastaveni\"}");
                    return;
                }
            }

            SystemConfig system = _config.system;
            system.hostname = _server.arg("hostname");
            system.ntpServer = _server.arg("ntpServer");
            system.timezone = _server.arg("timezone");
            system.timezoneId = _server.arg("timezoneId");
            system.hostname.trim(); system.ntpServer.trim(); system.timezone.trim(); system.timezoneId.trim();

            auto hasControl = [](const String& value) {
                for (size_t i = 0; i < value.length(); ++i) if (static_cast<uint8_t>(value[i]) < 0x20) return true;
                return false;
            };
            bool hostnameValid = !system.hostname.isEmpty() && system.hostname.length() <= 32 &&
                                 system.hostname[0] != '-' && !system.hostname.endsWith("-");
            for (size_t i = 0; hostnameValid && i < system.hostname.length(); ++i) {
                const char c = system.hostname[i];
                hostnameValid = isAlphaNumeric(c) || c == '-';
            }
            bool ntpValid = !system.ntpServer.isEmpty() && system.ntpServer.length() <= 253;
            for (size_t i = 0; ntpValid && i < system.ntpServer.length(); ++i) {
                const char c = system.ntpServer[i];
                ntpValid = isAlphaNumeric(c) || c == '.' || c == '-' || c == ':' || c == '[' || c == ']';
            }
            const bool timezoneValid = !system.timezone.isEmpty() && system.timezone.length() <= 127 && !hasControl(system.timezone);
            const bool timezoneIdValid = !system.timezoneId.isEmpty() && system.timezoneId.length() <= 64 &&
                                         !hasControl(system.timezoneId) &&
                                         (system.timezoneId.startsWith("manual:") || system.timezoneId.indexOf('/') > 0);
            if (!hostnameValid || !ntpValid || hasControl(system.ntpServer) || !timezoneValid || !timezoneIdValid) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Neplatne systemove nastaveni\"}");
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
                        } else zeroSeen = true;
                    }
                }
                return oneSeen;
            };
            const bool networkValid = wifi.dhcp
                ? validIp(wifi.ipAddress, true) && validMask(wifi.subnetMask, true) &&
                  validIp(wifi.gateway, true) && validIp(wifi.dns1, true) && validIp(wifi.dns2, true)
                : validIp(wifi.ipAddress, false) && validMask(wifi.subnetMask, false) &&
                  validIp(wifi.gateway, false) && validIp(wifi.dns1, false) && validIp(wifi.dns2, true);
            if (!networkValid) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Neplatna staticka IPv4 konfigurace\"}");
                return;
            }

            _server.send(200, "application/json", "{\"status\":\"saved\"}");
            _systemNetworkConfigCallback(system, wifi);
        });
    }

    void onWifiScan(WifiScanCallback callback);
    void onWifiKnownNetworks(WifiKnownNetworksCallback callback);
    void onWifiConnectKnown(WifiKnownNetworkActionCallback callback);
    void onWifiForget(WifiKnownNetworkActionCallback callback);
    void onWifiDisconnect(WifiDisconnectCallback callback);
    void onSourceConfig(SourceConfigCallback callback);
    void onPoolConfig(PoolConfigCallback callback);
    void onWeatherConfig(WeatherConfigCallback callback);
    void onHomeLayoutConfig(HomeLayoutConfigCallback callback);
    void onFactoryReset(FactoryResetCallback callback);
    void onConfigImport(ConfigImportCallback callback);
    void onRfSensorManagement(
        RfSensorStatusCallback statusCallback,
        RfSensorScanCallback scanCallback,
        RfSensorAddCallback addCallback,
        RfSensorRenameCallback renameCallback,
        RfSensorRemoveCallback removeCallback);

private:
    WebServer _server;
    DataModel& _dataModel;
    ScreenManager& _screenManager;
    const AppConfig& _config;
    ScreenChangeCallback _screenCallback;
    RefreshCallback _refreshCallback;
    DisplayStatusCallback _displayStatusCallback;
    DisplayPreview* _displayPreview = nullptr;
    NavigationController* _navigationController = nullptr;
    SemaphoreHandle_t _networkClientGate = nullptr;
    SystemConfigCallback _systemConfigCallback;
    WifiConfigCallback _wifiConfigCallback;
    WifiNetworkConfigCallback _wifiNetworkConfigCallback;
    SystemNetworkConfigCallback _systemNetworkConfigCallback;
    WifiScanCallback _wifiScanCallback;
    WifiKnownNetworksCallback _wifiKnownNetworksCallback;
    WifiKnownNetworkActionCallback _wifiConnectKnownCallback;
    WifiKnownNetworkActionCallback _wifiForgetCallback;
    WifiDisconnectCallback _wifiDisconnectCallback;
    SourceConfigCallback _sourceConfigCallback;
    PoolConfigCallback _poolConfigCallback;
    WeatherConfigCallback _weatherConfigCallback;
    HomeLayoutConfigCallback _homeLayoutConfigCallback;
    FactoryResetCallback _factoryResetCallback;
    ConfigImportCallback _configImportCallback;
    RfSensorStatusCallback _rfSensorStatusCallback;
    RfSensorScanCallback _rfSensorScanCallback;
    RfSensorAddCallback _rfSensorAddCallback;
    RfSensorRenameCallback _rfSensorRenameCallback;
    RfSensorRemoveCallback _rfSensorRemoveCallback;
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
    void handleApiNavigationState();
    void handleApiNavigationAction();
    void handleApiRestart();
    void handleApiFactoryReset();
    void handleApiConfigExport();
    void handleApiConfigImport();
    void handleApiSystemConfig();
    void handleApiWifiConfig();
    void handleApiWifiScan();
    void handleApiSourceConfig();
    void handleApiPoolConfig();
    void handleApiWeatherConfig();
    void handleApiCheckForUpdate();
    void handleApiGithubUpdate();
    void handleApiUpdateUpload();
    void handleApiUpdateComplete();
};
