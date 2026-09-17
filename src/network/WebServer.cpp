#include "WebServer.h"
#include "TimezoneUiPatch.h"
#include <cstring>

// Původní implementace zůstává beze změny a je vložena do této překladové jednotky.
#include "WebServerLegacy.inc"

void DashboardWebServer::enableTimezoneUiExtension() {
    _server.removeRoute("/", HTTP_GET);
    _server.on("/", HTTP_GET, [this]() { handleExtendedRoot(); });
    _server.on("/api/config/timezone", HTTP_GET, [this]() { handleApiTimezoneConfig(); });
    _server.on("/api/config/system-v2", HTTP_POST, [this]() { handleApiSystemConfigV2(); });
}

void DashboardWebServer::handleExtendedRoot() {
    const char* bodyEnd = strstr(INDEX_HTML, "</body>");
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html; charset=utf-8", "");

    if (bodyEnd) {
        const size_t prefixLength = static_cast<size_t>(bodyEnd - INDEX_HTML);
        _server.sendContent_P(INDEX_HTML, prefixLength);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
        _server.sendContent_P(bodyEnd);
    } else {
        _server.sendContent_P(INDEX_HTML);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
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
