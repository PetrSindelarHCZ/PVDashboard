#include "WebServer.h"
#include "TimezoneUiPatch.h"
#include "NtpUiPatch.h"
#include "LiveSettingsUiPatch.h"
#include "WifiUiPatch.h"
#include "WifiKnownDialogPatch.h"
#include "WeatherSettingsUiPatch.h"
#include "DisplayPreviewUiPatch.h"
#include "NavigationUiPatch.h"
#include "LayoutEditorUiPatch.h"
#include "../display/DisplayPreview.h"
#include "../navigation/NavigationController.h"
#include "TimeService.h"
#include "NetworkDiagnostics.h"
#include "../diagnostics/Performance.h"
#include "../config/ConfigBackup.h"
#include "../layout/HomeLayout.h"
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
#include <math.h>

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

String homeLayoutResponseJson(const HomeLayoutConfig& config, const DataModel& dataModel) {
    JsonDocument doc;
    doc["screen"] = "home";
    doc["customized"] = config.customized;

    JsonObject bounds = doc["bounds"].to<JsonObject>();
    bounds["x"] = HomeLayout::ContentLeft;
    bounds["y"] = HomeLayout::ContentTop;
    bounds["width"] = HomeLayout::ContentRight - HomeLayout::ContentLeft;
    bounds["height"] = HomeLayout::ContentBottom - HomeLayout::ContentTop;

    JsonArray stored = doc["widgets"].to<JsonArray>();
    for (uint8_t i = 0; i < config.widgetCount && i < MaxHomeLayoutWidgets; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        JsonObject item = stored.add<JsonObject>();
        item["id"] = widget.id;
        item["type"] = widget.type;
        item["visible"] = widget.visible;
        item["x"] = widget.x;
        item["y"] = widget.y;
        item["width"] = widget.width;
        item["height"] = widget.height;
        item["showFrame"] = widget.showFrame;
        item["background"] = widget.background;
        item["inverseText"] = widget.inverseText;
        if (widget.type == "custom") {
            item["title"] = widget.title;
            JsonArray elements = item["elements"].to<JsonArray>();
            for (uint8_t e = 0; e < widget.elements.size() && e < MaxCustomWidgetElements; ++e) {
                const CustomWidgetElementConfig& element = widget.elements[e];
                JsonObject child = elements.add<JsonObject>();
                HomeLayout::serializeElement(child, element);
            }
        }
    }

    ScreenLayout effective;
    HomeLayout::buildResolved(config, dataModel, effective);
    JsonArray effectiveItems = doc["effectiveWidgets"].to<JsonArray>();
    for (uint8_t i = 0; i < effective.count(); ++i) {
        const LayoutWidget& widget = effective[i];
        JsonObject item = effectiveItems.add<JsonObject>();
        item["id"] = widget.id;
        item["type"] = HomeLayout::typeName(widget.type);
        item["x"] = widget.x;
        item["y"] = widget.y;
        item["width"] = widget.width;
        item["height"] = widget.height;
    }

    JsonArray supported = doc["supportedWidgets"].to<JsonArray>();
    const char* ids[] = {"weather-card", "energy-card", "indoor-card"};
    const char* types[] = {"weather", "energy", "indoor"};
    for (uint8_t i = 0; i < 3; ++i) {
        JsonObject item = supported.add<JsonObject>();
        item["id"] = ids[i];
        item["type"] = types[i];
        item["minWidth"] = HomeLayout::minWidth(types[i]);
        item["minHeight"] = HomeLayout::minHeight(types[i]);
    }

    JsonArray defaultWidgets = doc["defaultWidgets"].to<JsonArray>();
    const char* defaultIds[] = {"weather-card", "energy-card", "indoor-card"};
    for (const char* id : defaultIds) {
        HomeLayoutWidgetConfig widget;
        if (!HomeLayout::buildDefaultWidget(dataModel, id, widget)) continue;
        JsonObject item = defaultWidgets.add<JsonObject>();
        item["id"] = widget.id;
        item["type"] = widget.type;
        item["visible"] = widget.visible;
        item["x"] = widget.x;
        item["y"] = widget.y;
        item["width"] = widget.width;
        item["height"] = widget.height;
        item["showFrame"] = widget.showFrame;
        item["background"] = widget.background;
        item["inverseText"] = widget.inverseText;
    }

    JsonObject appearance = doc["cardAppearance"].to<JsonObject>();
    JsonArray backgrounds = appearance["backgrounds"].to<JsonArray>();
    backgrounds.add("white");
    backgrounds.add("black");

    JsonObject custom = doc["customWidget"].to<JsonObject>();
    custom["type"] = "custom";
    custom["idPrefix"] = "custom-";
    custom["minWidth"] = HomeLayout::minWidth("custom");
    custom["minHeight"] = HomeLayout::minHeight("custom");
    custom["maxElements"] = MaxCustomWidgetElements;

    JsonArray elementTypes = custom["elementTypes"].to<JsonArray>();
    const char* customTypes[] = {"text", "kpi", "progress", "sparkline"};
    for (const char* type : customTypes) {
        JsonObject item = elementTypes.add<JsonObject>();
        item["type"] = type;
        item["minWidth"] = HomeLayout::elementMinWidth(type);
        item["minHeight"] = HomeLayout::elementMinHeight(type);
    }

    JsonArray fontSizes = custom["fontSizes"].to<JsonArray>();
    fontSizes.add("auto");
    for (uint8_t px = 7; px <= 64; ++px) {
        fontSizes.add(String(px));
    }

    JsonArray alignments = custom["alignments"].to<JsonArray>();
    alignments.add("left");
    alignments.add("center");
    alignments.add("right");

    JsonArray graphStyles = custom["graphStyles"].to<JsonArray>();
    graphStyles.add("line");
    graphStyles.add("bars");

    JsonArray sources = custom["dataSources"].to<JsonArray>();
    auto addSource = [&sources](const String& id, const String& label, const String& unit,
                                uint8_t decimals, bool history) {
        JsonObject item = sources.add<JsonObject>();
        item["id"] = id;
        item["label"] = label;
        item["unit"] = unit;
        item["decimals"] = decimals;
        item["history"] = history;
    };
    addSource("solar.productionPowerW", "Výroba FVE", "W", 0, true);
    addSource("solar.houseConsumptionW", "Spotřeba domu", "W", 0, true);
    addSource("solar.gridPowerW", "Distribuce", "W", 0, false);
    addSource("solar.energyTodayKWh", "Výroba dnes", "kWh", 1, false);
    addSource("solar.batterySocPercent", "Baterie", "%", 0, false);
    addSource("solar.batteryPowerW", "Výkon baterie", "W", 0, false);
    addSource("azrouter.gridPowerW", "AZRouter síť celkem", "W", 0, false);
    addSource("azrouter.gridL1PowerW", "AZRouter síť L1", "W", 0, false);
    addSource("azrouter.gridL2PowerW", "AZRouter síť L2", "W", 0, false);
    addSource("azrouter.gridL3PowerW", "AZRouter síť L3", "W", 0, false);
    addSource("azrouter.gridL1VoltageV", "AZRouter napětí L1", "V", 1, false);
    addSource("azrouter.gridL2VoltageV", "AZRouter napětí L2", "V", 1, false);
    addSource("azrouter.gridL3VoltageV", "AZRouter napětí L3", "V", 1, false);
    addSource("azrouter.gridL1CurrentA", "AZRouter proud L1", "A", 1, false);
    addSource("azrouter.gridL2CurrentA", "AZRouter proud L2", "A", 1, false);
    addSource("azrouter.gridL3CurrentA", "AZRouter proud L3", "A", 1, false);
    addSource("azrouter.routedPowerW", "AZRouter vytěžování celkem", "W", 0, false);
    addSource("azrouter.routedL1PowerW", "AZRouter vytěžování L1", "W", 0, false);
    addSource("azrouter.routedL2PowerW", "AZRouter vytěžování L2", "W", 0, false);
    addSource("azrouter.routedL3PowerW", "AZRouter vytěžování L3", "W", 0, false);
    addSource("azrouter.routedEnergyTodayKWh", "AZRouter energie dnes", "kWh", 1, false);
    addSource("azrouter.routedEnergyWeekKWh", "AZRouter energie týden", "kWh", 1, false);
    addSource("azrouter.routedEnergyMonthKWh", "AZRouter energie měsíc", "kWh", 1, false);
    addSource("azrouter.routedEnergyYearKWh", "AZRouter energie rok", "kWh", 1, false);
    addSource("azrouter.routedEnergyTotalKWh", "AZRouter energie celkem", "kWh", 1, false);
    addSource("azrouter.systemTempC", "AZRouter jednotka", "°C", 1, false);
    addSource("weather.outdoorTempC", "Venkovní teplota", "°C", 1, false);
    addSource("weather.outdoorHumidityPercent", "Venkovní vlhkost", "%", 0, false);
    addSource("weather.surfacePressureHpa", "Tlak", "hPa", 0, false);
    addSource("weather.windSpeedKmh", "Vítr", "km/h", 1, false);
    addSource("inside.temperatureC", "BME280 teplota", "°C", 1, false);
    addSource("inside.humidityPercent", "BME280 vlhkost", "%", 0, false);
    addSource("inside.pressureHpa", "BME280 tlak", "hPa", 1, false);
    addSource("inside.livingRoomTempC", "Obývák (BME280)", "°C", 1, false);
    addSource("inside.bedroomTempC", "Ložnice", "°C", 1, false);
    addSource("inside.poolTempC", "Bazén uvnitř modelu", "°C", 1, false);
    addSource("pool.waterTempC", "Bazén voda", "°C", 1, false);
    addSource("pool.targetTempC", "Bazén cíl", "°C", 1, false);
    addSource("pool.ph", "Bazén pH", "", 1, false);
    addSource("pool.freeChlorineMgL", "Volný chlor", "mg/l", 2, false);
    addSource("pool.airTempC", "Bazén vzduch", "°C", 1, false);
    addSource("pool.airHumidityPercent", "Bazén vlhkost", "%", 0, false);
    addSource("system.wifiRssi", "Wi-Fi RSSI", "dBm", 0, false);
    addSource("system.uptimeSeconds", "Uptime", "s", 0, false);

    for (uint8_t i = 0;
         i < dataModel.rfSensors.sensorCount && i < MaxRfSensors;
         ++i) {
        const RfSensorData& sensor = dataModel.rfSensors.sensors[i];
        if (!sensor.configured || sensor.slotId.isEmpty()) continue;

        String baseLabel = sensor.name;
        if (baseLabel.isEmpty()) {
            baseLabel = sensor.protocol;
            baseLabel += " 0x";
            baseLabel += String(sensor.sensorId, HEX);
            if (sensor.channel > 0) {
                baseLabel += " CH";
                baseLabel += String(sensor.channel);
            }
        }

        if (sensor.hasTemperature) {
            addSource(
                "rf." + sensor.slotId + ".temperatureC",
                baseLabel + " – teplota",
                "°C",
                1,
                false);
        }
        if (sensor.hasHumidity) {
            addSource(
                "rf." + sensor.slotId + ".humidityPercent",
                baseLabel + " – vlhkost",
                "%",
                0,
                false);
        }
    }

    String response;
    serializeJson(doc, response);
    return response;
}
}

void DashboardWebServer::onHomeLayoutConfig(HomeLayoutConfigCallback callback) {
    _homeLayoutConfigCallback = callback;

    _server.on("/api/layout/home", HTTP_GET, [this]() {
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(
            200,
            "application/json",
            homeLayoutResponseJson(_config.display.homeLayout, _dataModel));
    });

    _server.on("/api/layout/home", HTTP_POST, [this]() {
        if (!_homeLayoutConfigCallback || !_server.hasArg("plain")) {
            _server.send(
                503,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Home layout configuration unavailable\"}");
            return;
        }

        HomeLayoutConfig layout;
        String error;
        if (!HomeLayout::parseJson(_server.arg("plain"), layout, &error)) {
            JsonDocument doc;
            doc["status"] = "error";
            doc["message"] = error;
            String response;
            serializeJson(doc, response);
            _server.send(400, "application/json", response);
            return;
        }

        if (!_homeLayoutConfigCallback(layout)) {
            _server.send(
                500,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Failed to save Home layout\"}");
            return;
        }

        _server.sendHeader("Cache-Control", "no-store");
        _server.send(
            200,
            "application/json",
            homeLayoutResponseJson(_config.display.homeLayout, _dataModel));
    });

    _server.on("/api/layout/home/reset", HTTP_POST, [this]() {
        if (!_homeLayoutConfigCallback) {
            _server.send(
                503,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Home layout configuration unavailable\"}");
            return;
        }

        HomeLayoutConfig layout;
        if (!_homeLayoutConfigCallback(layout)) {
            _server.send(
                500,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Failed to reset Home layout\"}");
            return;
        }

        _server.sendHeader("Cache-Control", "no-store");
        _server.send(
            200,
            "application/json",
            homeLayoutResponseJson(_config.display.homeLayout, _dataModel));
    });
}

void DashboardWebServer::enableTimezoneUiExtension() {
    if (_timezoneUiEnabled) return;
    _timezoneUiEnabled = true;

    // První shodný handler má v ESP32 WebServer prioritu. Proto musí být
    // rozšířený root registrován před setupRoutes() uvnitř begin().
    _server.on("/", HTTP_GET, [this]() { handleExtendedRoot(); });
    _server.on("/api/config/timezone", HTTP_GET, [this]() { handleApiTimezoneConfig(); });
    _server.on("/api/config/system-v2", HTTP_POST, [this]() { handleApiSystemConfigV2(); });

    _server.on("/api/display", HTTP_GET, [this]() {
        if (_displayPreview == nullptr) {
            _server.send(503, "application/json", "{\"ready\":false,\"reason\":\"preview_unavailable\"}");
            return;
        }
        _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        _server.send(200, "application/json", _displayPreview->metadataJson());
    });

    _server.on("/api/navigation", HTTP_GET, [this]() { handleApiNavigationState(); });
    _server.on("/api/navigation", HTTP_POST, [this]() { handleApiNavigationAction(); });

    _server.on("/api/display.bmp", HTTP_GET, [this]() {
        if (_displayPreview == nullptr || !_displayPreview->ready()) {
            _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Display preview is not ready\"}");
            return;
        }

        _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        _server.setContentLength(_displayPreview->bmpSize());
        _server.send(200, "image/bmp", "");
        WiFiClient client = _server.client();
        client.setNoDelay(true);
        const uint32_t previewStarted = millis();
        const bool previewOk = _displayPreview->writeBmp(client);
        const uint32_t previewElapsed = millis() - previewStarted;
        Serial.printf(
            "[WEB] BMP nahled: %s, %u B za %lu ms.\n",
            previewOk ? "OK" : "CHYBA",
            static_cast<unsigned>(_displayPreview->bmpSize()),
            static_cast<unsigned long>(previewElapsed));
    });

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

    _server.on("/api/weather/locations", HTTP_POST, [this]() {
        if (!_weatherConfigCallback || !_server.hasArg("action")) {
            _server.send(503, "application/json",
                         "{\"status\":\"error\",\"message\":\"Weather location configuration unavailable\"}");
            return;
        }

        WeatherConfig weather = _config.weather;
        const String action = _server.arg("action");

        auto invalidText = [](const String& value, size_t maxLength, bool allowEmpty) {
            if ((!allowEmpty && value.isEmpty()) || value.length() > maxLength) return true;
            for (size_t i = 0; i < value.length(); ++i) {
                if (static_cast<uint8_t>(value[i]) < 0x20) return true;
            }
            return false;
        };

        auto findLocation = [&weather](const String& id) -> int {
            for (uint8_t i = 0; i < weather.locationCount && i < MaxWeatherLocations; ++i) {
                if (weather.locations[i].id == id) return static_cast<int>(i);
            }
            return -1;
        };

        if (action == "select") {
            const String id = _server.arg("id");
            const int index = findLocation(id);
            if (index < 0) {
                _server.send(404, "application/json",
                             "{\"status\":\"error\",\"message\":\"Weather location not found\"}");
                return;
            }
            weather.activeLocationId = weather.locations[index].id;
        } else if (action == "add") {
            if (!_server.hasArg("id") || !_server.hasArg("name") ||
                !_server.hasArg("latitude") || !_server.hasArg("longitude")) {
                _server.send(400, "application/json",
                             "{\"status\":\"error\",\"message\":\"Missing weather location\"}");
                return;
            }

            const String id = _server.arg("id");
            const String name = _server.arg("name");
            const String country = _server.arg("country");
            const double latitude = _server.arg("latitude").toDouble();
            const double longitude = _server.arg("longitude").toDouble();

            bool idValid = !id.isEmpty() && id.length() <= 32;
            for (size_t i = 0; idValid && i < id.length(); ++i) {
                const char ch = id[i];
                idValid = isAlphaNumeric(ch) || ch == '-' || ch == '_';
            }

            if (!idValid || invalidText(name, 80, false) ||
                invalidText(country, 64, true) ||
                latitude < -90.0 || latitude > 90.0 ||
                longitude < -180.0 || longitude > 180.0 ||
                (latitude == 0.0 && longitude == 0.0)) {
                _server.send(400, "application/json",
                             "{\"status\":\"error\",\"message\":\"Invalid weather location\"}");
                return;
            }

            int existing = -1;
            for (uint8_t i = 0; i < weather.locationCount && i < MaxWeatherLocations; ++i) {
                if (fabs(weather.locations[i].latitude - latitude) < 0.00001 &&
                    fabs(weather.locations[i].longitude - longitude) < 0.00001) {
                    existing = i;
                    break;
                }
            }

            const bool replaceLegacy =
                weather.locationCount == 1 &&
                (weather.locations[0].id == "legacy" ||
                 weather.locations[0].name == "Původní místo");

            if (replaceLegacy) {
                auto& location = weather.locations[0];
                location.id = id;
                location.name = name;
                location.country = country;
                location.latitude = latitude;
                location.longitude = longitude;
                weather.activeLocationId = id;
            } else if (existing >= 0) {
                // Pokud už místo existuje, aktualizujeme jeho čitelný název
                // z geokódování a pouze ho zvolíme jako aktivní.
                auto& location = weather.locations[existing];
                location.name = name;
                location.country = country;
                location.latitude = latitude;
                location.longitude = longitude;
                weather.activeLocationId = location.id;
            } else {
                if (weather.locationCount >= MaxWeatherLocations) {
                    _server.send(409, "application/json",
                                 "{\"status\":\"error\",\"message\":\"Maximum weather locations reached\"}");
                    return;
                }
                if (findLocation(id) >= 0) {
                    _server.send(409, "application/json",
                                 "{\"status\":\"error\",\"message\":\"Weather location id already exists\"}");
                    return;
                }

                auto& location = weather.locations[weather.locationCount++];
                location.id = id;
                location.name = name;
                location.country = country;
                location.latitude = latitude;
                location.longitude = longitude;
                weather.activeLocationId = id;
            }
        } else if (action == "move") {
            const String id = _server.arg("id");
            const String direction = _server.arg("direction");
            const int index = findLocation(id);
            if (index < 0) {
                _server.send(404, "application/json",
                             "{\"status\":\"error\",\"message\":\"Weather location not found\"}");
                return;
            }

            int target = index;
            if (direction == "up") target = index - 1;
            else if (direction == "down") target = index + 1;
            else {
                _server.send(400, "application/json",
                             "{\"status\":\"error\",\"message\":\"Invalid weather location move\"}");
                return;
            }

            if (target < 0 || target >= weather.locationCount) {
                _server.send(409, "application/json",
                             "{\"status\":\"error\",\"message\":\"Weather location already at edge\"}");
                return;
            }

            const WeatherLocation tmp = weather.locations[index];
            weather.locations[index] = weather.locations[target];
            weather.locations[target] = tmp;
        } else if (action == "delete") {
            if (weather.locationCount <= 1) {
                _server.send(409, "application/json",
                             "{\"status\":\"error\",\"message\":\"At least one weather location is required\"}");
                return;
            }

            const String id = _server.arg("id");
            const int index = findLocation(id);
            if (index < 0) {
                _server.send(404, "application/json",
                             "{\"status\":\"error\",\"message\":\"Weather location not found\"}");
                return;
            }

            const bool activeDeleted = weather.activeLocationId == id;
            for (uint8_t i = static_cast<uint8_t>(index); i + 1 < weather.locationCount; ++i) {
                weather.locations[i] = weather.locations[i + 1];
            }
            --weather.locationCount;
            weather.locations[weather.locationCount] = WeatherLocation();
            if (activeDeleted || findLocation(weather.activeLocationId) < 0) {
                weather.activeLocationId = weather.locations[0].id;
            }
        } else {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Unknown weather location action\"}");
            return;
        }

        weather.syncActiveCoordinates();
        _weatherConfigCallback(weather);

        JsonDocument doc;
        doc["status"] = "ok";
        doc["activeLocationId"] = weather.activeLocationId;
        JsonArray array = doc["locations"].to<JsonArray>();
        for (uint8_t i = 0; i < weather.locationCount && i < MaxWeatherLocations; ++i) {
            const auto& location = weather.locations[i];
            JsonObject item = array.add<JsonObject>();
            item["id"] = location.id;
            item["name"] = location.name;
            item["country"] = location.country;
            item["latitude"] = location.latitude;
            item["longitude"] = location.longitude;
        }

        String response;
        serializeJson(doc, response);
        _server.sendHeader("Cache-Control", "no-store");
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

void DashboardWebServer::onRfSensorManagement(
    RfSensorStatusCallback statusCallback,
    RfSensorScanCallback scanCallback,
    RfSensorAddCallback addCallback,
    RfSensorRenameCallback renameCallback,
    RfSensorRemoveCallback removeCallback) {

    _rfSensorStatusCallback = statusCallback;
    _rfSensorScanCallback = scanCallback;
    _rfSensorAddCallback = addCallback;
    _rfSensorRenameCallback = renameCallback;
    _rfSensorRemoveCallback = removeCallback;

    _server.on("/api/rf-sensors", HTTP_GET, [this]() {
        if (!_rfSensorStatusCallback) {
            _server.send(
                503,
                "application/json",
                "{\"status\":\"error\",\"message\":\"RF sensor management unavailable\"}");
            return;
        }
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", _rfSensorStatusCallback());
    });

    _server.on("/api/rf-sensors/scan", HTTP_POST, [this]() {
        if (!_rfSensorScanCallback) {
            _server.send(
                503,
                "application/json",
                "{\"status\":\"error\",\"message\":\"RF sensor scan unavailable\"}");
            return;
        }

        long durationSeconds = 30;
        if (_server.hasArg("durationSeconds")) {
            durationSeconds = _server.arg("durationSeconds").toInt();
        }
        if (durationSeconds < 10) durationSeconds = 10;
        if (durationSeconds > 120) durationSeconds = 120;

        _rfSensorScanCallback(
            static_cast<uint32_t>(durationSeconds) * 1000UL);

        JsonDocument doc;
        doc["status"] = "started";
        doc["durationSeconds"] = durationSeconds;
        String response;
        serializeJson(doc, response);
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(200, "application/json", response);
    });

    _server.on("/api/rf-sensors/add", HTTP_POST, [this]() {
        if (!_rfSensorAddCallback || !_server.hasArg("bindingKey")) {
            _server.send(
                400,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Chybí identifikace čidla\"}");
            return;
        }

        String error;
        const String bindingKey = _server.arg("bindingKey");
        const String name = _server.hasArg("name") ? _server.arg("name") : "";
        if (!_rfSensorAddCallback(bindingKey, name, error)) {
            JsonDocument doc;
            doc["status"] = "error";
            doc["message"] = error.isEmpty() ? "Čidlo nelze přidat" : error;
            String response;
            serializeJson(doc, response);
            _server.send(409, "application/json", response);
            return;
        }

        _server.send(200, "application/json", "{\"status\":\"saved\"}");
    });

    _server.on("/api/rf-sensors/rename", HTTP_POST, [this]() {
        if (!_rfSensorRenameCallback || !_server.hasArg("slotId")) {
            _server.send(
                400,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Chybí interní ID čidla\"}");
            return;
        }

        String error;
        const String slotId = _server.arg("slotId");
        const String name = _server.hasArg("name") ? _server.arg("name") : "";
        if (!_rfSensorRenameCallback(slotId, name, error)) {
            JsonDocument doc;
            doc["status"] = "error";
            doc["message"] = error.isEmpty() ? "Čidlo nelze přejmenovat" : error;
            String response;
            serializeJson(doc, response);
            _server.send(404, "application/json", response);
            return;
        }

        _server.send(200, "application/json", "{\"status\":\"saved\"}");
    });

    _server.on("/api/rf-sensors/remove", HTTP_POST, [this]() {
        if (!_rfSensorRemoveCallback || !_server.hasArg("slotId")) {
            _server.send(
                400,
                "application/json",
                "{\"status\":\"error\",\"message\":\"Chybí interní ID čidla\"}");
            return;
        }

        String error;
        if (!_rfSensorRemoveCallback(_server.arg("slotId"), error)) {
            JsonDocument doc;
            doc["status"] = "error";
            doc["message"] = error.isEmpty() ? "Čidlo nelze odebrat" : error;
            String response;
            serializeJson(doc, response);
            _server.send(404, "application/json", response);
            return;
        }

        _server.send(200, "application/json", "{\"status\":\"removed\"}");
    });
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
        _server.sendContent_P(DISPLAY_PREVIEW_UI_PATCH);
        _server.sendContent_P(NAVIGATION_UI_PATCH);
        _server.sendContent_P(LAYOUT_EDITOR_UI_PATCH);
        _server.sendContent_P(bodyEnd);
    } else {
        _server.sendContent_P(INDEX_HTML);
        _server.sendContent_P(TIMEZONE_UI_PATCH);
        _server.sendContent_P(NTP_UI_PATCH);
        _server.sendContent_P(LIVE_SETTINGS_UI_PATCH);
        _server.sendContent_P(WIFI_UI_PATCH);
        _server.sendContent_P(WIFI_KNOWN_DIALOG_PATCH);
        _server.sendContent_P(WEATHER_SETTINGS_UI_PATCH);
        _server.sendContent_P(DISPLAY_PREVIEW_UI_PATCH);
        _server.sendContent_P(NAVIGATION_UI_PATCH);
        _server.sendContent_P(LAYOUT_EDITOR_UI_PATCH);
    }
    _server.sendContent("");
}

void DashboardWebServer::handleApiNavigationState() {
    if (_navigationController == nullptr) {
        _server.send(503, "application/json",
                     "{\"status\":\"error\",\"message\":\"Navigation unavailable\"}");
        return;
    }

    const NavigationState& state = _navigationController->getState();
    NavigationLayout layout;
    _navigationController->buildCurrentLayout(layout);

    auto screenTitle = [this](const String& id) -> String {
        for (auto* screen : _screenManager.getAllScreens()) {
            if (screen && screen->getId().equalsIgnoreCase(id)) return screen->getTitle();
        }
        return id;
    };

    JsonDocument doc;
    doc["area"] = navigationAreaName(state.area);
    doc["activeScreen"] = _screenManager.getActiveScreenId();
    doc["activeTitle"] = screenTitle(_screenManager.getActiveScreenId());
    doc["sidebarScreen"] = state.sidebarScreenId;
    doc["sidebarTitle"] = screenTitle(state.sidebarScreenId);
    doc["focus"] = state.focusId;
    doc["subpageIndex"] = state.subpageIndex;
    doc["subpageCount"] = _dataModel.system.navigationSubpageCount;

    JsonArray elements = doc["elements"].to<JsonArray>();
    for (uint8_t i = 0; i < layout.count; ++i) {
        if (!layout.elements[i].enabled) continue;
        JsonObject item = elements.add<JsonObject>();
        item["id"] = layout.elements[i].id;
        item["x"] = layout.elements[i].bounds.x;
        item["y"] = layout.elements[i].bounds.y;
        item["width"] = layout.elements[i].bounds.width;
        item["height"] = layout.elements[i].bounds.height;
    }

    String response;
    serializeJson(doc, response);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json", response);
}

void DashboardWebServer::handleApiNavigationAction() {
    if (_navigationController == nullptr) {
        _server.send(503, "application/json",
                     "{\"status\":\"error\",\"message\":\"Navigation unavailable\"}");
        return;
    }
    if (!_server.hasArg("action")) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Navigation action is required\"}");
        return;
    }

    NavigationAction action;
    if (!parseNavigationAction(_server.arg("action"), action)) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Unknown navigation action\"}");
        return;
    }

    const bool handled = _navigationController->handleAction(action);
    const NavigationState& state = _navigationController->getState();
    NavigationLayout layout;
    _navigationController->buildCurrentLayout(layout);

    auto screenTitle = [this](const String& id) -> String {
        for (auto* screen : _screenManager.getAllScreens()) {
            if (screen && screen->getId().equalsIgnoreCase(id)) return screen->getTitle();
        }
        return id;
    };

    JsonDocument doc;
    doc["status"] = "ok";
    doc["handled"] = handled;
    doc["action"] = navigationActionName(action);
    doc["area"] = navigationAreaName(state.area);
    doc["activeScreen"] = _screenManager.getActiveScreenId();
    doc["activeTitle"] = screenTitle(_screenManager.getActiveScreenId());
    doc["sidebarScreen"] = state.sidebarScreenId;
    doc["sidebarTitle"] = screenTitle(state.sidebarScreenId);
    doc["focus"] = state.focusId;
    doc["subpageIndex"] = state.subpageIndex;
    doc["subpageCount"] = _dataModel.system.navigationSubpageCount;

    String response;
    serializeJson(doc, response);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json", response);
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
