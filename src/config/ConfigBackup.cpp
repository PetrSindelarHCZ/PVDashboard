#include "ConfigBackup.h"
#include <math.h>
#include <cstdlib>
#include <cerrno>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include "../layout/HomeLayout.h"

namespace {
constexpr uint32_t RequiredMaskV1 = (1UL << 20) - 1;
constexpr uint32_t RequiredMaskV2 = (1UL << 21) - 1;
constexpr uint32_t RequiredMaskV3 = (1UL << 27) - 1;
constexpr uint32_t RequiredMaskV4 = (1UL << 29) - 1;
constexpr uint32_t RequiredMaskV5 = (1UL << 30) - 1;
constexpr uint32_t RequiredMaskV6 = 0x7FFFFFFFUL;
constexpr uint32_t RequiredMaskV7 = 0xFFFFFFFFUL;

String quoteYaml(const String& value) {
    String output = "\"";
    output.reserve(value.length() + 8);
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < value.length(); ++i) {
        const uint8_t c = static_cast<uint8_t>(value[i]);
        if (c == '\\' || c == '"') { output += '\\'; output += static_cast<char>(c); }
        else if (c == '\n') output += "\\n";
        else if (c == '\r') output += "\\r";
        else if (c == '\t') output += "\\t";
        else if (c < 0x20) { output += "\\x"; output += hex[c >> 4]; output += hex[c & 0x0F]; }
        else output += static_cast<char>(c);
    }
    output += '"';
    return output;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseString(const String& scalar, String& value) {
    if (scalar.length() < 2 || scalar[0] != '"' || scalar[scalar.length() - 1] != '"') return false;
    value = "";
    for (size_t i = 1; i + 1 < scalar.length(); ++i) {
        char c = scalar[i];
        if (c != '\\') {
            if (c == '"') return false;
            value += c;
            continue;
        }
        if (++i >= scalar.length() - 1) return false;
        c = scalar[i];
        if (c == 'n') value += '\n';
        else if (c == 'r') value += '\r';
        else if (c == 't') value += '\t';
        else if (c == '\\' || c == '"') value += c;
        else if (c == 'x' && i + 2 < scalar.length() - 1) {
            const int high = hexValue(scalar[++i]);
            const int low = hexValue(scalar[++i]);
            if (high < 0 || low < 0 || (high == 0 && low == 0)) return false;
            value += static_cast<char>((high << 4) | low);
        } else return false;
    }
    return true;
}

bool parseBool(const String& scalar, bool& value) {
    if (scalar == "true") { value = true; return true; }
    if (scalar == "false") { value = false; return true; }
    return false;
}

bool parseLong(const String& scalar, long& value) {
    errno = 0; char* end = nullptr;
    value = strtol(scalar.c_str(), &end, 10);
    return errno == 0 && end != scalar.c_str() && *end == '\0';
}

bool parseDouble(const String& scalar, double& value) {
    errno = 0; char* end = nullptr;
    value = strtod(scalar.c_str(), &end);
    return errno == 0 && end != scalar.c_str() && *end == '\0';
}

bool invalidString(const String& value) {
    for (size_t i = 0; i < value.length(); ++i) {
        if (static_cast<uint8_t>(value[i]) < 0x20) return true;
    }
    return false;
}

bool validIpv4(const String& value, bool allowEmpty) {
    if (value.isEmpty()) return allowEmpty;
    IPAddress address;
    return address.fromString(value) && address != IPAddress(0, 0, 0, 0);
}

bool validSubnetMask(const String& value, bool allowEmpty) {
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
}

String inferTimezoneId(const String& timezone) {
    if (timezone == "CET-1CEST,M3.5.0,M10.5.0/3") return "Europe/Prague";
    if (timezone == "GMT0BST,M3.5.0/1,M10.5.0") return "Europe/London";
    if (timezone == "EET-2EEST,M3.5.0/3,M10.5.0/4") return "Europe/Helsinki";
    return "";
}


String serializeRfSensorsBackup(const RfSensorsConfig& rfSensors) {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    const uint8_t count = min<uint8_t>(rfSensors.sensorCount, MaxRfSensors);
    for (uint8_t i = 0; i < count; ++i) {
        const RfSensorConfig& sensor = rfSensors.sensors[i];
        JsonObject item = array.add<JsonObject>();
        item["slotId"] = sensor.slotId;
        item["protocol"] = sensor.protocol;
        item["sensorId"] = sensor.sensorId;
        item["channel"] = sensor.channel;
        item["name"] = sensor.name;
        item["temperature"] = sensor.hasTemperature;
        item["humidity"] = sensor.hasHumidity;
        item["battery"] = sensor.hasBattery;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

bool parseRfSensorsBackup(const String& json, RfSensorsConfig& rfSensors) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    JsonArray array = doc.as<JsonArray>();
    if (array.isNull() || array.size() > MaxRfSensors) return false;

    RfSensorsConfig parsed;
    for (JsonObject item : array) {
        RfSensorConfig sensor;
        sensor.slotId = item["slotId"] | "";
        sensor.protocol = item["protocol"] | "";
        sensor.sensorId = item["sensorId"] | 0UL;
        sensor.channel = item["channel"] | 0;
        sensor.name = item["name"] | "";
        sensor.hasTemperature = item["temperature"] | false;
        sensor.hasHumidity = item["humidity"] | false;
        sensor.hasBattery = item["battery"] | false;

        if (sensor.slotId.isEmpty() ||
            !sensor.slotId.startsWith("sensor") ||
            sensor.slotId.length() > 16 ||
            sensor.protocol.isEmpty() ||
            sensor.protocol.length() > 24 ||
            sensor.name.length() > 40 ||
            sensor.channel > 15 ||
            (!sensor.hasTemperature && !sensor.hasHumidity && !sensor.hasBattery) ||
            invalidString(sensor.name)) {
            return false;
        }

        for (uint8_t i = 0; i < parsed.sensorCount; ++i) {
            const RfSensorConfig& existing = parsed.sensors[i];
            if (existing.slotId == sensor.slotId ||
                (existing.protocol == sensor.protocol &&
                 existing.sensorId == sensor.sensorId &&
                 existing.channel == sensor.channel)) {
                return false;
            }
        }
        parsed.sensors[parsed.sensorCount++] = sensor;
    }

    rfSensors = parsed;
    return true;
}

String serializeWeatherLocations(const WeatherConfig& weather) {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    const uint8_t count = min<uint8_t>(weather.locationCount, MaxWeatherLocations);
    for (uint8_t i = 0; i < count; ++i) {
        const auto& location = weather.locations[i];
        JsonObject item = array.add<JsonObject>();
        item["id"] = location.id;
        item["name"] = location.name;
        item["country"] = location.country;
        item["latitude"] = location.latitude;
        item["longitude"] = location.longitude;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

bool parseWeatherLocations(const String& json, WeatherConfig& weather) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    JsonArray array = doc.as<JsonArray>();
    if (array.isNull() || array.size() == 0 || array.size() > MaxWeatherLocations) return false;

    weather.locationCount = 0;
    for (JsonObject item : array) {
        const String id = item["id"] | "";
        const String name = item["name"] | "";
        const String country = item["country"] | "";
        const double latitude = item["latitude"] | 999.0;
        const double longitude = item["longitude"] | 999.0;
        if (id.isEmpty() || id.length() > 32 || name.isEmpty() || name.length() > 80 ||
            country.length() > 64 || latitude < -90.0 || latitude > 90.0 ||
            longitude < -180.0 || longitude > 180.0 ||
            (latitude == 0.0 && longitude == 0.0) ||
            invalidString(id) || invalidString(name) || invalidString(country)) return false;
        for (uint8_t i = 0; i < weather.locationCount; ++i) {
            if (weather.locations[i].id == id) return false;
        }
        auto& location = weather.locations[weather.locationCount++];
        location.id = id;
        location.name = name;
        location.country = country;
        location.latitude = latitude;
        location.longitude = longitude;
    }
    return true;
}
}

String exportConfigurationYaml(const AppConfig& c) {
    String y;
    y.reserve(8192);
    y += "format: \"pvdashboard-config\"\nversion: 7\n";
    y += "system:\n  hostname: " + quoteYaml(c.system.hostname) + "\n";
    y += "  ntp_server: " + quoteYaml(c.system.ntpServer) + "\n";
    y += "  timezone: " + quoteYaml(c.system.timezone) + "\n";
    y += "  timezone_id: " + quoteYaml(c.system.timezoneId) + "\n";
    y += "wifi:\n  ssid: " + quoteYaml(c.wifi.ssid) + "\n";
    y += "  password: " + quoteYaml(c.wifi.password) + "\n";
    y += "  dhcp: " + String(c.wifi.dhcp ? "true" : "false") + "\n";
    y += "  ip_address: " + quoteYaml(c.wifi.ipAddress) + "\n";
    y += "  subnet_mask: " + quoteYaml(c.wifi.subnetMask) + "\n";
    y += "  gateway: " + quoteYaml(c.wifi.gateway) + "\n";
    y += "  dns1: " + quoteYaml(c.wifi.dns1) + "\n";
    y += "  dns2: " + quoteYaml(c.wifi.dns2) + "\n";
    y += "goodwe:\n  enabled: " + String(c.goodwe.enabled ? "true" : "false") + "\n";
    y += "  host: " + quoteYaml(c.goodwe.host) + "\n  port: " + String(c.goodwe.port) + "\n";
    y += "  interval_seconds: " + String(c.goodwe.pollIntervalSeconds) + "\n";
    y += "azrouter:\n  enabled: " + String(c.azrouter.enabled ? "true" : "false") + "\n";
    y += "  host: " + quoteYaml(c.azrouter.host) + "\n  port: " + String(c.azrouter.port) + "\n";
    y += "  interval_seconds: " + String(c.azrouter.pollIntervalSeconds) + "\n";
    y += "pool:\n  enabled: " + String(c.pool.enabled ? "true" : "false") + "\n";
    y += "rf_sensors:\n  sensors_json: " + quoteYaml(serializeRfSensorsBackup(c.rfSensors)) + "\n";
    y += "layout:\n  home_json: " + quoteYaml(HomeLayout::serializeJson(c.display.homeLayout)) + "\n";
    y += "weather:\n  enabled: " + String(c.weather.enabled ? "true" : "false") + "\n";
    y += "  provider: " + quoteYaml(c.weather.provider) + "\n";
    y += "  latitude: " + String(c.weather.latitude, 6) + "\n  longitude: " + String(c.weather.longitude, 6) + "\n";
    y += "  interval_seconds: " + String(c.weather.pollIntervalSeconds) + "\n";
    y += "  active_location_id: " + quoteYaml(c.weather.activeLocationId) + "\n";
    y += "  locations_json: " + quoteYaml(serializeWeatherLocations(c.weather)) + "\n";
    return y;
}

bool importConfigurationYaml(const String& yaml, AppConfig& config, String& error) {
    if (yaml.isEmpty() || yaml.length() > 24576) { error = "Empty or oversized YAML"; return false; }
    AppConfig parsed;
    String section;
    uint32_t seen = 0;
    long configVersion = 0;
    size_t offset = 0;
    uint16_t lineNumber = 0;
    while (offset < yaml.length()) {
        size_t end = yaml.indexOf('\n', offset);
        if (end == static_cast<size_t>(-1)) end = yaml.length();
        String raw = yaml.substring(offset, end);
        offset = end + 1; ++lineNumber;
        if (raw.endsWith("\r")) raw.remove(raw.length() - 1);
        if (raw.indexOf('\t') >= 0) { error = "Tabs are not allowed at line " + String(lineNumber); return false; }
        int indent = 0; while (indent < raw.length() && raw[indent] == ' ') ++indent;
        String line = raw.substring(indent); line.trim();
        if (line.isEmpty() || line.startsWith("#")) continue;
        const int colon = line.indexOf(':');
        if (colon <= 0) { error = "Invalid YAML at line " + String(lineNumber); return false; }
        String key = line.substring(0, colon); key.trim();
        String scalar = line.substring(colon + 1); scalar.trim();
        if (indent == 0 && scalar.isEmpty()) { section = key; continue; }
        if ((indent == 0 && !section.isEmpty()) || (indent != 0 && indent != 2)) {
            error = "Invalid indentation at line " + String(lineNumber); return false;
        }
        const String path = indent == 0 ? key : section + "." + key;
        String text; bool flag = false; long number = 0; double decimal = 0;
        bool ok = true; uint8_t bit = 0;
        if (path == "format") { ok = parseString(scalar, text) && text == "pvdashboard-config"; bit = 0; }
        else if (path == "version") { ok = parseLong(scalar, number) && (number >= 1 && number <= 7); configVersion = number; bit = 1; }
        else if (path == "system.hostname") { ok = parseString(scalar, parsed.system.hostname); bit = 2; }
        else if (path == "system.ntp_server") { ok = parseString(scalar, parsed.system.ntpServer); bit = 3; }
        else if (path == "system.timezone") { ok = parseString(scalar, parsed.system.timezone); bit = 4; }
        else if (path == "wifi.ssid") { ok = parseString(scalar, parsed.wifi.ssid); bit = 5; }
        else if (path == "wifi.password") { ok = parseString(scalar, parsed.wifi.password); bit = 6; }
        else if (path == "goodwe.enabled") { ok = parseBool(scalar, parsed.goodwe.enabled); bit = 7; }
        else if (path == "goodwe.host") { ok = parseString(scalar, parsed.goodwe.host); bit = 8; }
        else if (path == "goodwe.port") { ok = parseLong(scalar, number) && number >= 1 && number <= 65535; parsed.goodwe.port = number; bit = 9; }
        else if (path == "goodwe.interval_seconds") { ok = parseLong(scalar, number) && number >= 1 && number <= 3600; parsed.goodwe.pollIntervalSeconds = number; bit = 10; }
        else if (path == "azrouter.enabled") { ok = parseBool(scalar, parsed.azrouter.enabled); bit = 11; }
        else if (path == "azrouter.host") { ok = parseString(scalar, parsed.azrouter.host); bit = 12; }
        else if (path == "azrouter.port") { ok = parseLong(scalar, number) && number >= 1 && number <= 65535; parsed.azrouter.port = number; bit = 13; }
        else if (path == "azrouter.interval_seconds") { ok = parseLong(scalar, number) && number >= 1 && number <= 3600; parsed.azrouter.pollIntervalSeconds = number; bit = 14; }
        else if (path == "weather.enabled") { ok = parseBool(scalar, parsed.weather.enabled); bit = 15; }
        else if (path == "weather.provider") { ok = parseString(scalar, parsed.weather.provider) && (parsed.weather.provider == "open-meteo" || parsed.weather.provider == "met-no"); bit = 16; }
        else if (path == "weather.latitude") { ok = parseDouble(scalar, decimal) && decimal >= -90 && decimal <= 90; parsed.weather.latitude = decimal; bit = 17; }
        else if (path == "weather.longitude") { ok = parseDouble(scalar, decimal) && decimal >= -180 && decimal <= 180; parsed.weather.longitude = decimal; bit = 18; }
        else if (path == "weather.interval_seconds") { ok = parseLong(scalar, number) && number >= 900 && number <= 21600; parsed.weather.pollIntervalSeconds = number; bit = 19; }
        else if (path == "system.timezone_id") { ok = parseString(scalar, parsed.system.timezoneId); bit = 20; }
        else if (path == "wifi.dhcp") { ok = parseBool(scalar, parsed.wifi.dhcp); bit = 21; }
        else if (path == "wifi.ip_address") { ok = parseString(scalar, parsed.wifi.ipAddress); bit = 22; }
        else if (path == "wifi.subnet_mask") { ok = parseString(scalar, parsed.wifi.subnetMask); bit = 23; }
        else if (path == "wifi.gateway") { ok = parseString(scalar, parsed.wifi.gateway); bit = 24; }
        else if (path == "wifi.dns1") { ok = parseString(scalar, parsed.wifi.dns1); bit = 25; }
        else if (path == "wifi.dns2") { ok = parseString(scalar, parsed.wifi.dns2); bit = 26; }
        else if (path == "weather.active_location_id") { ok = parseString(scalar, parsed.weather.activeLocationId); bit = 27; }
        else if (path == "weather.locations_json") { ok = parseString(scalar, text) && parseWeatherLocations(text, parsed.weather); bit = 28; }
        else if (path == "pool.enabled") { ok = parseBool(scalar, parsed.pool.enabled); bit = 29; }
        else if (path == "layout.home_json") { ok = parseString(scalar, text) && HomeLayout::parseJson(text, parsed.display.homeLayout); bit = 30; }
        else if (path == "rf_sensors.sensors_json") { ok = parseString(scalar, text) && parseRfSensorsBackup(text, parsed.rfSensors); bit = 31; }
        else { error = "Unknown setting at line " + String(lineNumber); return false; }
        if (!ok || (seen & (1UL << bit))) { error = "Invalid or duplicate setting at line " + String(lineNumber); return false; }
        seen |= 1UL << bit;
    }

    const uint32_t requiredMask = configVersion == 7 ? RequiredMaskV7 :
                                  (configVersion == 6 ? RequiredMaskV6 :
                                  (configVersion == 5 ? RequiredMaskV5 :
                                  (configVersion == 4 ? RequiredMaskV4 :
                                  (configVersion == 3 ? RequiredMaskV3 :
                                  (configVersion == 2 ? RequiredMaskV2 : RequiredMaskV1)))));
    if (configVersion == 1 && !(seen & (1UL << 20))) parsed.system.timezoneId = inferTimezoneId(parsed.system.timezone);
    if (configVersion < 3) {
        parsed.wifi.dhcp = true;
        parsed.wifi.ipAddress = "";
        parsed.wifi.subnetMask = "";
        parsed.wifi.gateway = "";
        parsed.wifi.dns1 = "";
        parsed.wifi.dns2 = "";
    }

    if (configVersion < 4) {
        const bool defaultPrague =
            fabs(parsed.weather.latitude - 50.0755) < 0.00001 &&
            fabs(parsed.weather.longitude - 14.4378) < 0.00001;
        parsed.weather.locationCount = 1;
        parsed.weather.locations[0].id = defaultPrague ? "praha" : "legacy";
        parsed.weather.locations[0].name = defaultPrague ? "Praha" : "Původní místo";
        parsed.weather.locations[0].country = defaultPrague ? "Česko" : "";
        parsed.weather.locations[0].latitude = parsed.weather.latitude;
        parsed.weather.locations[0].longitude = parsed.weather.longitude;
        parsed.weather.activeLocationId = parsed.weather.locations[0].id;
    }

    if (configVersion < 5) {
        // Starší zálohy vznikly v době, kdy byla obrazovka Bazén vždy viditelná.
        parsed.pool.enabled = true;
    }

    if (configVersion < 6) {
        // Starší zálohy neměly uživatelsky uložený layout.
        parsed.display.homeLayout = HomeLayoutConfig{};
    }

    if (configVersion < 7) {
        // Starší zálohy neměly správu 433MHz čidel.
        parsed.rfSensors = RfSensorsConfig{};
    }

    bool activeLocationValid = false;
    for (uint8_t i = 0; i < parsed.weather.locationCount; ++i) {
        if (parsed.weather.locations[i].id == parsed.weather.activeLocationId) {
            activeLocationValid = true;
            break;
        }
    }
    if (activeLocationValid) parsed.weather.syncActiveCoordinates();

    const bool addressingValid = parsed.wifi.dhcp
        ? validIpv4(parsed.wifi.ipAddress, true) && validSubnetMask(parsed.wifi.subnetMask, true) &&
          validIpv4(parsed.wifi.gateway, true) && validIpv4(parsed.wifi.dns1, true) && validIpv4(parsed.wifi.dns2, true)
        : validIpv4(parsed.wifi.ipAddress, false) && validSubnetMask(parsed.wifi.subnetMask, false) &&
          validIpv4(parsed.wifi.gateway, false) && validIpv4(parsed.wifi.dns1, false) && validIpv4(parsed.wifi.dns2, true);

    if (seen != requiredMask || parsed.system.hostname.isEmpty() || parsed.system.hostname.length() > 32 ||
        parsed.system.ntpServer.isEmpty() || parsed.system.ntpServer.length() > 253 ||
        parsed.system.timezone.isEmpty() || parsed.system.timezone.length() > 127 || parsed.system.timezoneId.length() > 64 ||
        (parsed.goodwe.enabled && parsed.goodwe.host.isEmpty()) || (parsed.azrouter.enabled && parsed.azrouter.host.isEmpty()) ||
        invalidString(parsed.system.hostname) || invalidString(parsed.system.ntpServer) || invalidString(parsed.system.timezone) || invalidString(parsed.system.timezoneId) ||
        invalidString(parsed.wifi.ssid) || invalidString(parsed.wifi.password) || invalidString(parsed.wifi.ipAddress) ||
        invalidString(parsed.wifi.subnetMask) || invalidString(parsed.wifi.gateway) || invalidString(parsed.wifi.dns1) || invalidString(parsed.wifi.dns2) ||
        invalidString(parsed.goodwe.host) || invalidString(parsed.azrouter.host) || !addressingValid ||
        !activeLocationValid || parsed.weather.locationCount == 0 ||
        (parsed.weather.latitude == 0.0 && parsed.weather.longitude == 0.0)) {
        error = "YAML configuration is incomplete or invalid"; return false;
    }
    config = parsed;
    return true;
}
