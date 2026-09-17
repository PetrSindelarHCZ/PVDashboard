#include "WebServer.h"
#include "TimezoneUiPatch.h"
#include "NtpUiPatch.h"
#include "LiveSettingsUiPatch.h"
#include "WifiUiPatch.h"
#include "WifiKnownDialogPatch.h"
#include "WeatherSettingsUiPatch.h"
#include "TimeService.h"
#include "NetworkDiagnostics.h"
#include "../diagnostics/Performance.h"
#include "../config/ConfigBackup.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>
#include "../../include/Version.h"
#include "../../include/FirmwareLimits.h"
#include <cstring>
#include <vector>

// Původní implementaci zachováváme beze změny. Rozšířené routy
// se registrují explicitně z DashboardApp ještě před begin().
#include "WebServerLegacy.inc"

namespace {
bool jsonContainsKnownSsid(const String& json, const String& ssid) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    for (JsonObject item : doc.as<JsonArray>()) {
        const char* candidate = item["ssid"] | "";
        if (ssid == candidate) return true;
    }
    return false;
}

constexpr uint8_t MaxCustomNtpServers = 8;
const char* BasicNtpServers[] = {
    "pool.ntp.org",
    "cz.pool.ntp.org",
    "europe.pool.ntp.org",
    "time.cloudflare.com",
    "time.google.com",
    "time.windows.com"
};

bool isBasicNtpServer(const String& server) {
    for (const char* basic : BasicNtpServers) {
        if (server.equalsIgnoreCase(basic)) return true;
    }
    return false;
}

bool isValidNtpServer(String server) {
    server.trim();
    if (server.isEmpty() || server.length() > 253) return false;
    for (size_t i = 0; i < server.length(); ++i) {
        const char c = server[i];
        const bool valid = isAlphaNumeric(c) || c == '.' || c == '-' || c == ':' || c == '[' || c == ']';
        if (!valid) return false;
    }
    return true;
}

std::vector<String> loadCustomNtpServers() {
    std::vector<String> servers;
    Preferences preferences;
    if (!preferences.begin("dashboard", true)) return servers;
    const uint8_t count = min<uint8_t>(preferences.getUChar("ntp_custom_n", 0), MaxCustomNtpServers);
    for (uint8_t i = 0; i < count; ++i) {
        const String key = "ntp_c" + String(i);
        const String server = preferences.getString(key.c_str(), "");
        if (!server.isEmpty()) servers.push_back(server);
    }
    preferences.end();
    return servers;
}

bool saveCustomNtpServers(const std::vector<String>& servers) {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) return false;
    preferences.putUChar("ntp_custom_n", static_cast<uint8_t>(servers.size()));
    for (uint8_t i = 0; i < MaxCustomNtpServers; ++i) {
        const String key = "ntp_c" + String(i);
        if (i < servers.size()) preferences.putString(key.c_str(), servers[i]);
        else preferences.remove(key.c_str());
    }
    preferences.end();
    return true;
}

String customNtpServersJson(const std::vector<String>& servers) {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (const auto& server : servers) array.add(server);
    String response;
    serializeJson(doc, response);
    return response;
}
}

void DashboardWebServer::enableTimezoneUiExtension() {
    if (_timezoneUiEnabled) return;
    _timezoneUiEnabled = true;

    // První shodný handler má v ESP32 WebServer prioritu. Proto musí být
    // rozšířený root registrován před setupRoutes() uvnitř begin().
    _server.on("/", HTTP_GET, [this]() { handleExtendedRoot(); });
    _server.on("/api/config/timezone", HTTP_GET, [this]() { handleApiTimezoneConfig(); });
    _server.on("/api/config/system-v2", HTTP_POST, [this]() { handleApiSystemConfigV2(); });

    _server.on("/api/ntp/status", HTTP_GET, [this]() {
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", TimeService::getNtpStatusJson(_dataModel.system.wifiConnected));
    });

    _server.on("/api/ntp/custom", HTTP_GET, [this]() {
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", customNtpServersJson(loadCustomNtpServers()));
    });

    _server.on("/api/ntp/custom", HTTP_POST, [this]() {
        if (!_server.hasArg("action") || !_server.hasArg("server")) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing NTP setting\"}");
            return;
        }

        const String action = _server.arg("action");
        String server = _server.arg("server");
        server.trim();
        if (!isValidNtpServer(server)) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid NTP server\"}");
            return;
        }

        auto servers = loadCustomNtpServers();
        if (action == "add") {
            if (!isBasicNtpServer(server)) {
                bool exists = false;
                for (const auto& item : servers) {
                    if (item.equalsIgnoreCase(server)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    if (servers.size() >= MaxCustomNtpServers) {
                        _server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Maximum custom NTP servers reached\"}");
                        return;
                    }
                    servers.push_back(server);
                }
            }
        } else if (action == "delete") {
            for (auto it = servers.begin(); it != servers.end(); ++it) {
                if (!it->equalsIgnoreCase(server)) continue;
                servers.erase(it);
                break;
            }
        } else {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Unknown NTP action\"}");
            return;
        }

        if (!saveCustomNtpServers(servers)) {
            _server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save NTP servers\"}");
            return;
        }

        JsonDocument doc;
        doc["status"] = "ok";
        JsonArray array = doc["servers"].to<JsonArray>();
        for (const auto& item : servers) array.add(item);
        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    });

    // Diagnostika je záměrně omezena jen na nakonfigurované zdroje.
    // GoodWe používá UDP/Modbus, AZRouter TCP/HTTP.
    _server.on("/api/diagnostics/device", HTTP_GET, [this]() {
        if (!_server.hasArg("source")) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing source\"}");
            return;
        }

        const String source = _server.arg("source");
        String host;
        uint16_t port = 0;
        bool enabled = false;
        NetworkProbeTransport transport = NetworkProbeTransport::Tcp;

        if (source == "goodwe") {
            host = _config.goodwe.host;
            port = _config.goodwe.port;
            enabled = _config.goodwe.enabled;
            transport = NetworkProbeTransport::GoodWeUdp;
        } else if (source == "azrouter") {
            host = _config.azrouter.host;
            port = _config.azrouter.port;
            enabled = _config.azrouter.enabled;
            transport = NetworkProbeTransport::Tcp;
        } else {
            _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Unknown source\"}");
            return;
        }

        JsonDocument doc;
        doc["source"] = source;
        doc["enabled"] = enabled;
        doc["host"] = host;
        doc["port"] = port;
        doc["wifiConnected"] = _dataModel.system.wifiConnected;

        if (!enabled || !_dataModel.system.wifiConnected || host.isEmpty() || port == 0) {
            doc["tested"] = false;
            doc["resolved"] = false;
            doc["pingOk"] = false;
            doc["portOpen"] = false;
            doc["portProtocol"] = transport == NetworkProbeTransport::GoodWeUdp ? "UDP" : "TCP";
            if (!enabled) doc["message"] = "Source disabled";
            else if (!_dataModel.system.wifiConnected) doc["message"] = "Wi-Fi offline";
            else doc["message"] = "Invalid host or port";
        } else {
            const NetworkProbeResult probe = NetworkDiagnostics::probe(host, port, transport);
            doc["tested"] = true;
            doc["resolved"] = probe.resolved;
            doc["resolvedIp"] = probe.resolvedIp;
            doc["pingOk"] = probe.pingOk;
            doc["pingMs"] = probe.pingMs;
            doc["portOpen"] = probe.portOpen;
            doc["portMs"] = probe.portConnectMs;
            doc["portProtocol"] = probe.portProtocol;
            doc["message"] = probe.error;
        }
        doc["testedAtMs"] = millis();

        String response;
        serializeJson(doc, response);
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", response);
    });

    // Bezpečný Wi-Fi endpoint nikdy nevrací uložené heslo.
    _server.on("/api/config/wifi", HTTP_GET, [this]() {
        JsonDocument doc;
        doc["ssid"] = _config.wifi.ssid;
        doc["hasPassword"] = !_config.wifi.password.isEmpty();
        doc["connected"] = _dataModel.system.wifiConnected;
        doc["accessPoint"] = _dataModel.system.wifiAccessPoint;
        doc["ip"] = _dataModel.system.ipAddress;
        doc["rssi"] = _dataModel.system.wifiRssi;
        String response;
        serializeJson(doc, response);
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", response);
    });

    _server.on("/api/wifi/known", HTTP_GET, [this]() {
        if (!_wifiKnownNetworksCallback) {
            _server.send(503, "application/json", "[]");
            return;
        }
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", _wifiKnownNetworksCallback());
    });

    _server.on("/api/wifi/config-v2", HTTP_POST, [this]() {
        if (!_server.hasArg("ssid") || _server.arg("ssid").isEmpty()) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"SSID is required\"}");
            return;
        }
        if (!_wifiConfigCallback) {
            _server.send(503, "application/json",
                         "{\"status\":\"error\",\"message\":\"Wi-Fi configuration unavailable\"}");
            return;
        }

        const String ssid = _server.arg("ssid");
        const bool keepPassword = _server.arg("keepPassword") == "1";
        String password = _server.hasArg("password") ? _server.arg("password") : String();

        const auto hasControl = [](const String& value) {
            for (size_t i = 0; i < value.length(); ++i) {
                if (static_cast<uint8_t>(value[i]) < 0x20) return true;
            }
            return false;
        };

        if (ssid.length() > 32 || password.length() > 64 || hasControl(ssid) || hasControl(password)) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Invalid Wi-Fi setting\"}");
            return;
        }

        if (keepPassword) {
            if (ssid != _config.wifi.ssid || _config.wifi.password.isEmpty()) {
                _server.send(409, "application/json",
                             "{\"status\":\"error\",\"message\":\"Stored password is not available for this SSID\"}");
                return;
            }
            password = _config.wifi.password;
        }

        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _wifiConfigCallback(ssid, password);
    });

    _server.on("/api/wifi/connect-known", HTTP_POST, [this]() {
        if (!_server.hasArg("ssid") || _server.arg("ssid").isEmpty()) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID is required\"}");
            return;
        }
        if (!_wifiKnownNetworksCallback || !_wifiConnectKnownCallback) {
            _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Known Wi-Fi unavailable\"}");
            return;
        }
        const String ssid = _server.arg("ssid");
        if (!jsonContainsKnownSsid(_wifiKnownNetworksCallback(), ssid)) {
            _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Unknown Wi-Fi network\"}");
            return;
        }
        _server.send(200, "application/json", "{\"status\":\"connecting\"}");
        _wifiConnectKnownCallback(ssid);
    });

    _server.on("/api/wifi/disconnect", HTTP_POST, [this]() {
        if (!_wifiDisconnectCallback) {
            _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Wi-Fi disconnect unavailable\"}");
            return;
        }
        _server.send(200, "application/json", "{\"status\":\"disconnected\"}");
        _wifiDisconnectCallback();
    });

    _server.on("/api/wifi/forget", HTTP_POST, [this]() {
        if (!_server.hasArg("ssid") || _server.arg("ssid").isEmpty()) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID is required\"}");
            return;
        }
        if (!_wifiKnownNetworksCallback || !_wifiForgetCallback) {
            _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Known Wi-Fi unavailable\"}");
            return;
        }
        const String ssid = _server.arg("ssid");
        if (!jsonContainsKnownSsid(_wifiKnownNetworksCallback(), ssid)) {
            _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Unknown Wi-Fi network\"}");
            return;
        }
        _server.send(200, "application/json", "{\"status\":\"forgotten\"}");
        _wifiForgetCallback(ssid);
    });
}

void DashboardWebServer::onWifiKnownNetworks(WifiKnownNetworksCallback callback) {
    _wifiKnownNetworksCallback = callback;
}

void DashboardWebServer::onWifiConnectKnown(WifiKnownNetworkActionCallback callback) {
    _wifiConnectKnownCallback = callback;
}

void DashboardWebServer::onWifiForget(WifiKnownNetworkActionCallback callback) {
    _wifiForgetCallback = callback;
}

void DashboardWebServer::onWifiDisconnect(WifiDisconnectCallback callback) {
    _wifiDisconnectCallback = callback;
}

void DashboardWebServer::handleExtendedRoot() {
    const char* bodyEnd = strstr(INDEX_HTML, "</body>");
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html; charset=utf-8", "");

    if (bodyEnd) {
        const size_t prefixLength = static_cast<size_t>(bodyEnd - INDEX_HTML);
        _server.sendContent_P(INDEX_HTML, prefixLength);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
        _server.sendContent_P(NTP_UI_PATCH);
        _server.sendContent_P(LIVE_SETTINGS_UI_PATCH);
        _server.sendContent_P(WIFI_UI_PATCH);
        _server.sendContent_P(WIFI_KNOWN_DIALOG_PATCH);
        _server.sendContent_P(WEATHER_SETTINGS_UI_PATCH);
        _server.sendContent_P(bodyEnd);
    } else {
        _server.sendContent_P(INDEX_HTML);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
        _server.sendContent_P(NTP_UI_PATCH);
        _server.sendContent_P(LIVE_SETTINGS_UI_PATCH);
        _server.sendContent_P(WIFI_UI_PATCH);
        _server.sendContent_P(WIFI_KNOWN_DIALOG_PATCH);
        _server.sendContent_P(WEATHER_SETTINGS_UI_PATCH);
    }
    _server.sendContent("");
}

void DashboardWebServer::handleApiTimezoneConfig() {
    JsonDocument doc;
    doc["timezone"] = _config.system.timezone;
    doc["timezoneId"] = _config.system.timezoneId;
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void DashboardWebServer::handleApiSystemConfigV2() {
    const char* required[] = {"hostname", "ntpServer", "timezone", "timezoneId"};
    for (const char* name : required) {
        if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Missing system setting\"}");
            return;
        }
    }

    SystemConfig system;
    system.hostname = _server.arg("hostname");
    system.ntpServer = _server.arg("ntpServer");
    system.timezone = _server.arg("timezone");
    system.timezoneId = _server.arg("timezoneId");

    const auto hasControl = [](const String& value) {
        for (size_t i = 0; i < value.length(); ++i) {
            if (static_cast<uint8_t>(value[i]) < 0x20) return true;
        }
        return false;
    };

    bool hostnameValid = system.hostname.length() <= 32 &&
                         system.hostname[0] != '-' &&
                         !system.hostname.endsWith("-");
    for (size_t i = 0; hostnameValid && i < system.hostname.length(); ++i) {
        const char c = system.hostname[i];
        hostnameValid = isAlphaNumeric(c) || c == '-';
    }

    const bool timezoneIdValid = system.timezoneId.length() <= 64 &&
                                 (system.timezoneId.startsWith("manual:") ||
                                  system.timezoneId.indexOf('/') > 0);

    if (!hostnameValid ||
        !isValidNtpServer(system.ntpServer) ||
        system.timezone.length() > 127 ||
        !timezoneIdValid ||
        hasControl(system.ntpServer) ||
        hasControl(system.timezone) ||
        hasControl(system.timezoneId)) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Invalid system setting\"}");
        return;
    }

    if (!_systemConfigCallback) {
        _server.send(503, "application/json",
                     "{\"status\":\"error\",\"message\":\"System configuration unavailable\"}");
        return;
    }

    _server.send(200, "application/json", "{\"status\":\"saved\"}");
    _systemConfigCallback(system);
}
