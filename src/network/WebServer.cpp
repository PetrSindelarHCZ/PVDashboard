#include "WebServer.h"
#include "TimezoneUiPatch.h"
#include "LiveSettingsUiPatch.h"
#include "WifiUiPatch.h"
#include "../diagnostics/Performance.h"
#include "../config/ConfigBackup.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include "../../include/Version.h"
#include "../../include/FirmwareLimits.h"
#include <cstring>

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
}

void DashboardWebServer::enableTimezoneUiExtension() {
    if (_timezoneUiEnabled) return;
    _timezoneUiEnabled = true;

    // První shodný handler má v ESP32 WebServer prioritu. Proto musí být
    // rozšířený root registrován před setupRoutes() uvnitř begin().
    _server.on("/", HTTP_GET, [this]() { handleExtendedRoot(); });
    _server.on("/api/config/timezone", HTTP_GET, [this]() { handleApiTimezoneConfig(); });
    _server.on("/api/config/system-v2", HTTP_POST, [this]() { handleApiSystemConfigV2(); });

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
        _server.sendContent_P(LIVE_SETTINGS_UI_PATCH);
        _server.sendContent_P(WIFI_UI_PATCH);
        _server.sendContent_P(bodyEnd);
    } else {
        _server.sendContent_P(INDEX_HTML);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
        _server.sendContent_P(LIVE_SETTINGS_UI_PATCH);
        _server.sendContent_P(WIFI_UI_PATCH);
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
        system.ntpServer.length() > 253 ||
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
