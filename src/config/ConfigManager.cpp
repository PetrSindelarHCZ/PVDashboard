#include "ConfigManager.h"
#include <math.h>
#include <Preferences.h>
#include <ArduinoJson.h>

namespace {
String inferTimezoneId(const String& timezone) {
    if (timezone == "CET-1CEST,M3.5.0,M10.5.0/3") return "Europe/Prague";
    if (timezone == "GMT0BST,M3.5.0/1,M10.5.0") return "Europe/London";
    if (timezone == "EET-2EEST,M3.5.0/3,M10.5.0/4") return "Europe/Helsinki";
    return "";
}

void saveWeatherLocations(Preferences& preferences, const WeatherConfig& weather) {
    const uint8_t count = min<uint8_t>(weather.locationCount, MaxWeatherLocations);
    preferences.putUChar("wx_loc_n", count);
    preferences.putString("wx_active", weather.activeLocationId);
    for (uint8_t i = 0; i < MaxWeatherLocations; ++i) {
        const String suffix = String(i);
        const String idKey = "wx_loc_id" + suffix;
        const String nameKey = "wx_loc_name" + suffix;
        const String countryKey = "wx_loc_country" + suffix;
        const String latKey = "wx_loc_lat" + suffix;
        const String lonKey = "wx_loc_lon" + suffix;
        if (i < count) {
            const auto& location = weather.locations[i];
            preferences.putString(idKey.c_str(), location.id);
            preferences.putString(nameKey.c_str(), location.name);
            preferences.putString(countryKey.c_str(), location.country);
            preferences.putDouble(latKey.c_str(), location.latitude);
            preferences.putDouble(lonKey.c_str(), location.longitude);
        } else {
            preferences.remove(idKey.c_str());
            preferences.remove(nameKey.c_str());
            preferences.remove(countryKey.c_str());
            preferences.remove(latKey.c_str());
            preferences.remove(lonKey.c_str());
        }
    }
}

void loadWeatherLocations(Preferences& preferences, WeatherConfig& weather) {
    if (!preferences.isKey("wx_loc_n")) {
        // Migrace starší konfigurace s jedinou dvojicí souřadnic.
        const bool defaultPrague =
            fabs(weather.latitude - 50.0755) < 0.00001 &&
            fabs(weather.longitude - 14.4378) < 0.00001;
        weather.locationCount = 1;
        weather.locations[0].id = defaultPrague ? "praha" : "legacy";
        weather.locations[0].name = defaultPrague ? "Praha" : "Původní místo";
        weather.locations[0].country = defaultPrague ? "Česko" : "";
        weather.locations[0].latitude = weather.latitude;
        weather.locations[0].longitude = weather.longitude;
        weather.activeLocationId = weather.locations[0].id;
        saveWeatherLocations(preferences, weather);
        return;
    }

    weather.locationCount = min<uint8_t>(preferences.getUChar("wx_loc_n", 0), MaxWeatherLocations);
    for (uint8_t i = 0; i < weather.locationCount; ++i) {
        const String suffix = String(i);
        const String idKey = "wx_loc_id" + suffix;
        const String nameKey = "wx_loc_name" + suffix;
        const String countryKey = "wx_loc_country" + suffix;
        const String latKey = "wx_loc_lat" + suffix;
        const String lonKey = "wx_loc_lon" + suffix;
        auto& location = weather.locations[i];
        location.id = preferences.getString(idKey.c_str(), "");
        location.name = preferences.getString(nameKey.c_str(), "");
        location.country = preferences.getString(countryKey.c_str(), "");
        location.latitude = preferences.getDouble(latKey.c_str(), 0.0);
        location.longitude = preferences.getDouble(lonKey.c_str(), 0.0);
    }
    if (weather.locationCount == 0) {
        weather.locationCount = 1;
        weather.locations[0].id = "legacy";
        weather.locations[0].name = "Původní místo";
        weather.locations[0].latitude = weather.latitude;
        weather.locations[0].longitude = weather.longitude;
    }
    weather.activeLocationId = preferences.getString("wx_active", weather.locations[0].id);
    bool activeFound = false;
    for (uint8_t i = 0; i < weather.locationCount; ++i) {
        if (weather.locations[i].id == weather.activeLocationId) {
            activeFound = true;
            break;
        }
    }
    if (!activeFound) weather.activeLocationId = weather.locations[0].id;
    weather.syncActiveCoordinates();
}
}

ConfigManager::ConfigManager() {
}

bool ConfigManager::begin() {
    Preferences preferences;
    preferences.begin("dashboard", false);
    if (preferences.isKey("sys_hostname")) _config.system.hostname = preferences.getString("sys_hostname", _config.system.hostname);
    const bool hasStoredTimezone = preferences.isKey("sys_timezone");
    if (hasStoredTimezone) _config.system.timezone = preferences.getString("sys_timezone", _config.system.timezone);
    if (preferences.isKey("sys_tzid")) {
        _config.system.timezoneId = preferences.getString("sys_tzid", _config.system.timezoneId);
    } else if (hasStoredTimezone) {
        _config.system.timezoneId = inferTimezoneId(_config.system.timezone);
    }
    if (preferences.isKey("sys_ntp")) _config.system.ntpServer = preferences.getString("sys_ntp", _config.system.ntpServer);
    if (preferences.isKey("wifi_ssid")) _config.wifi.ssid = preferences.getString("wifi_ssid", _config.wifi.ssid);
    if (preferences.isKey("wifi_password")) _config.wifi.password = preferences.getString("wifi_password", _config.wifi.password);
    _config.wifi.dhcp = preferences.getBool("wifi_dhcp", _config.wifi.dhcp);
    if (preferences.isKey("wifi_ip")) _config.wifi.ipAddress = preferences.getString("wifi_ip", _config.wifi.ipAddress);
    if (preferences.isKey("wifi_mask")) _config.wifi.subnetMask = preferences.getString("wifi_mask", _config.wifi.subnetMask);
    if (preferences.isKey("wifi_gw")) _config.wifi.gateway = preferences.getString("wifi_gw", _config.wifi.gateway);
    if (preferences.isKey("wifi_dns1")) _config.wifi.dns1 = preferences.getString("wifi_dns1", _config.wifi.dns1);
    if (preferences.isKey("wifi_dns2")) _config.wifi.dns2 = preferences.getString("wifi_dns2", _config.wifi.dns2);

    _knownWifiNetworks.clear();
    const uint8_t knownCount = min<uint8_t>(preferences.getUChar("wifi_known_n", 0), MaxKnownWifiNetworks);
    for (uint8_t i = 0; i < knownCount; ++i) {
        const String ssidKey = "w_ssid" + String(i);
        const String passKey = "w_pass" + String(i);
        const String autoKey = "w_auto" + String(i);
        const String ssid = preferences.getString(ssidKey.c_str(), "");
        if (ssid.isEmpty()) continue;
        _knownWifiNetworks.push_back({ssid, preferences.getString(passKey.c_str(), ""), preferences.getBool(autoKey.c_str(), true)});
    }

    _config.goodwe.enabled = preferences.getBool("gw_enabled", _config.goodwe.enabled);
    if (preferences.isKey("gw_host")) _config.goodwe.host = preferences.getString("gw_host", _config.goodwe.host);
    _config.goodwe.port = preferences.getUShort("gw_port", _config.goodwe.port);
    _config.goodwe.pollIntervalSeconds = preferences.getUInt("gw_interval", _config.goodwe.pollIntervalSeconds);
    _config.azrouter.enabled = preferences.getBool("az_enabled", _config.azrouter.enabled);
    if (preferences.isKey("az_host")) _config.azrouter.host = preferences.getString("az_host", _config.azrouter.host);
    _config.azrouter.port = preferences.getUShort("az_port", _config.azrouter.port);
    _config.azrouter.pollIntervalSeconds = preferences.getUInt("az_interval", _config.azrouter.pollIntervalSeconds);
    _config.weather.enabled = preferences.getBool("wx_enabled", _config.weather.enabled);
    if (preferences.isKey("wx_provider")) _config.weather.provider = preferences.getString("wx_provider", _config.weather.provider);
    _config.weather.latitude = preferences.getDouble("wx_lat", _config.weather.latitude);
    _config.weather.longitude = preferences.getDouble("wx_lon", _config.weather.longitude);
    _config.weather.pollIntervalSeconds = preferences.getUInt("wx_interval", _config.weather.pollIntervalSeconds);
    loadWeatherLocations(preferences, _config.weather);
    preferences.end();

    if (!_config.wifi.ssid.isEmpty() && _config.wifi.ssid != "VASE_WIFI") {
        String storedPassword;
        if (!getKnownWifiPassword(_config.wifi.ssid, storedPassword)) rememberWifi(_config.wifi.ssid, _config.wifi.password, true);
    }

    Serial.printf("[CONFIG] Nactena konfigurace (Schema v%u):\n", _config.schemaVersion);
    Serial.printf("  Wi-Fi SSID: '%s' | znamych siti: %u | IP: %s\n",
                  _config.wifi.ssid.c_str(), static_cast<unsigned>(_knownWifiNetworks.size()),
                  _config.wifi.dhcp ? "DHCP" : _config.wifi.ipAddress.c_str());
    Serial.printf("  Timezone: %s (%s)\n", _config.system.timezoneId.c_str(), _config.system.timezone.c_str());
    Serial.printf("  GoodWe: %s (host: %s:%u)\n", _config.goodwe.enabled ? "Povoleno" : "Zakazano", _config.goodwe.host.c_str(), _config.goodwe.port);
    Serial.printf("  AZRouter: %s (host: %s:%u)\n", _config.azrouter.enabled ? "Povoleno" : "Zakazano", _config.azrouter.host.c_str(), _config.azrouter.port);
    const WeatherLocation* activeWeatherLocation = _config.weather.activeLocation();
    Serial.printf("  Pocasi: %s | mist: %u | aktivni: %s (%.5f, %.5f)\n",
                  _config.weather.enabled ? "Povoleno" : "Zakazano",
                  _config.weather.locationCount,
                  activeWeatherLocation ? activeWeatherLocation->name.c_str() : "-",
                  _config.weather.latitude, _config.weather.longitude);
    return true;
}

const AppConfig& ConfigManager::get() const { return _config; }

void ConfigManager::setSystem(const SystemConfig& system) {
    _config.system = system;
    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putString("sys_hostname", system.hostname);
    preferences.putString("sys_timezone", system.timezone);
    preferences.putString("sys_tzid", system.timezoneId);
    preferences.putString("sys_ntp", system.ntpServer);
    preferences.end();
}

void ConfigManager::rememberWifi(const String& ssid, const String& password, bool enableAutoConnect) {
    if (ssid.isEmpty() || ssid == "VASE_WIFI") return;
    for (auto& network : _knownWifiNetworks) {
        if (network.ssid == ssid) {
            network.password = password;
            if (enableAutoConnect) network.autoConnect = true;
            saveKnownWifiNetworks();
            return;
        }
    }
    if (_knownWifiNetworks.size() >= MaxKnownWifiNetworks) _knownWifiNetworks.erase(_knownWifiNetworks.begin());
    _knownWifiNetworks.push_back({ssid, password, enableAutoConnect});
    saveKnownWifiNetworks();
}

void ConfigManager::saveKnownWifiNetworks() {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) return;
    preferences.putUChar("wifi_known_n", static_cast<uint8_t>(_knownWifiNetworks.size()));
    for (size_t i = 0; i < MaxKnownWifiNetworks; ++i) {
        const String ssidKey = "w_ssid" + String(i), passKey = "w_pass" + String(i), autoKey = "w_auto" + String(i);
        if (i < _knownWifiNetworks.size()) {
            preferences.putString(ssidKey.c_str(), _knownWifiNetworks[i].ssid);
            preferences.putString(passKey.c_str(), _knownWifiNetworks[i].password);
            preferences.putBool(autoKey.c_str(), _knownWifiNetworks[i].autoConnect);
        } else {
            preferences.remove(ssidKey.c_str()); preferences.remove(passKey.c_str()); preferences.remove(autoKey.c_str());
        }
    }
    preferences.end();
}

void ConfigManager::setWifi(const String& ssid, const String& password) {
    _config.wifi.ssid = ssid;
    _config.wifi.password = password;
    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_password", password);
    preferences.end();
    rememberWifi(ssid, password, true);
}

void ConfigManager::setWifiNetworkConfig(const WifiConfig& wifi) {
    _config.wifi.dhcp = wifi.dhcp;
    _config.wifi.ipAddress = wifi.ipAddress;
    _config.wifi.subnetMask = wifi.subnetMask;
    _config.wifi.gateway = wifi.gateway;
    _config.wifi.dns1 = wifi.dns1;
    _config.wifi.dns2 = wifi.dns2;
    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putBool("wifi_dhcp", wifi.dhcp);
    preferences.putString("wifi_ip", wifi.ipAddress);
    preferences.putString("wifi_mask", wifi.subnetMask);
    preferences.putString("wifi_gw", wifi.gateway);
    preferences.putString("wifi_dns1", wifi.dns1);
    preferences.putString("wifi_dns2", wifi.dns2);
    preferences.end();
}

String ConfigManager::getKnownWifiNetworksJson() const {
    JsonDocument doc; JsonArray array = doc.to<JsonArray>();
    for (const auto& network : _knownWifiNetworks) {
        JsonObject item = array.add<JsonObject>();
        item["ssid"] = network.ssid; item["hasPassword"] = !network.password.isEmpty();
        item["active"] = network.ssid == _config.wifi.ssid; item["autoConnect"] = network.autoConnect;
    }
    String response; serializeJson(doc, response); return response;
}

bool ConfigManager::getKnownWifiPassword(const String& ssid, String& password) const {
    for (const auto& network : _knownWifiNetworks) if (network.ssid == ssid) { password = network.password; return true; }
    return false;
}

bool ConfigManager::getAutoJoinWifiPassword(const String& ssid, String& password) const {
    for (const auto& network : _knownWifiNetworks) if (network.ssid == ssid && network.autoConnect) { password = network.password; return true; }
    return false;
}

bool ConfigManager::isWifiAutoConnectEnabled(const String& ssid) const {
    for (const auto& network : _knownWifiNetworks) if (network.ssid == ssid) return network.autoConnect;
    return false;
}

bool ConfigManager::setWifiAutoConnectEnabled(const String& ssid, bool enabled) {
    for (auto& network : _knownWifiNetworks) {
        if (network.ssid != ssid) continue;
        if (network.autoConnect == enabled) return true;
        network.autoConnect = enabled; saveKnownWifiNetworks(); return true;
    }
    return false;
}

bool ConfigManager::forgetWifi(const String& ssid) {
    for (auto it = _knownWifiNetworks.begin(); it != _knownWifiNetworks.end(); ++it) {
        if (it->ssid != ssid) continue;
        _knownWifiNetworks.erase(it); saveKnownWifiNetworks();
        if (_config.wifi.ssid == ssid) {
            _config.wifi.ssid = ""; _config.wifi.password = "";
            Preferences preferences; preferences.begin("dashboard", false);
            preferences.putString("wifi_ssid", ""); preferences.putString("wifi_password", ""); preferences.end();
        }
        return true;
    }
    return false;
}

void ConfigManager::setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter) {
    _config.goodwe = goodwe; _config.azrouter = azrouter;
    Preferences preferences; preferences.begin("dashboard", false);
    preferences.putBool("gw_enabled", goodwe.enabled); preferences.putString("gw_host", goodwe.host); preferences.putUShort("gw_port", goodwe.port); preferences.putUInt("gw_interval", goodwe.pollIntervalSeconds);
    preferences.putBool("az_enabled", azrouter.enabled); preferences.putString("az_host", azrouter.host); preferences.putUShort("az_port", azrouter.port); preferences.putUInt("az_interval", azrouter.pollIntervalSeconds); preferences.end();
}

void ConfigManager::setWeather(const WeatherConfig& weather) {
    WeatherConfig normalized = weather;
    normalized.syncActiveCoordinates();
    _config.weather = normalized;
    Preferences preferences; preferences.begin("dashboard", false);
    preferences.putBool("wx_enabled", normalized.enabled);
    preferences.putString("wx_provider", normalized.provider);
    preferences.putDouble("wx_lat", normalized.latitude);
    preferences.putDouble("wx_lon", normalized.longitude);
    preferences.putUInt("wx_interval", normalized.pollIntervalSeconds);
    saveWeatherLocations(preferences, normalized);
    preferences.end();
}

bool ConfigManager::resetToFactoryDefaults() {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) return false;
    const bool cleared = preferences.clear(); preferences.end();
    if (!cleared) return false;
    _config = AppConfig{}; _knownWifiNetworks.clear(); return true;
}

bool ConfigManager::setUserConfiguration(const AppConfig& config) {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) return false;
    preferences.putString("sys_hostname", config.system.hostname); preferences.putString("sys_timezone", config.system.timezone); preferences.putString("sys_tzid", config.system.timezoneId); preferences.putString("sys_ntp", config.system.ntpServer);
    preferences.putString("wifi_ssid", config.wifi.ssid); preferences.putString("wifi_password", config.wifi.password);
    preferences.putBool("wifi_dhcp", config.wifi.dhcp); preferences.putString("wifi_ip", config.wifi.ipAddress); preferences.putString("wifi_mask", config.wifi.subnetMask); preferences.putString("wifi_gw", config.wifi.gateway); preferences.putString("wifi_dns1", config.wifi.dns1); preferences.putString("wifi_dns2", config.wifi.dns2);
    preferences.putBool("gw_enabled", config.goodwe.enabled); preferences.putString("gw_host", config.goodwe.host); preferences.putUShort("gw_port", config.goodwe.port); preferences.putUInt("gw_interval", config.goodwe.pollIntervalSeconds);
    preferences.putBool("az_enabled", config.azrouter.enabled); preferences.putString("az_host", config.azrouter.host); preferences.putUShort("az_port", config.azrouter.port); preferences.putUInt("az_interval", config.azrouter.pollIntervalSeconds);
    WeatherConfig normalizedWeather = config.weather;
    normalizedWeather.syncActiveCoordinates();
    preferences.putBool("wx_enabled", normalizedWeather.enabled);
    preferences.putString("wx_provider", normalizedWeather.provider);
    preferences.putDouble("wx_lat", normalizedWeather.latitude);
    preferences.putDouble("wx_lon", normalizedWeather.longitude);
    preferences.putUInt("wx_interval", normalizedWeather.pollIntervalSeconds);
    saveWeatherLocations(preferences, normalizedWeather);
    preferences.end();
    _config.system = config.system; _config.wifi = config.wifi; _config.goodwe = config.goodwe; _config.azrouter = config.azrouter; _config.weather = normalizedWeather;
    rememberWifi(config.wifi.ssid, config.wifi.password, true);
    Serial.println("[CONFIG] YAML konfigurace importovana do NVS.");
    return true;
}
