#include "ConfigManager.h"
#include <Preferences.h>

namespace {
String inferTimezoneId(const String& timezone) {
    if (timezone == "CET-1CEST,M3.5.0,M10.5.0/3") return "Europe/Prague";
    if (timezone == "GMT0BST,M3.5.0/1,M10.5.0") return "Europe/London";
    if (timezone == "EET-2EEST,M3.5.0/3,M10.5.0/4") return "Europe/Helsinki";
    return "";
}
}

ConfigManager::ConfigManager() {
    // Inicializace výchozí konfigurace dle ConfigSchema.h
    // _config již obsahuje výchozí hodnoty z definice struct AppConfig
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
    preferences.end();

    Serial.printf("[CONFIG] Načtena konfigurace (Schema v%u):\n", _config.schemaVersion);
    Serial.printf("  Wi-Fi SSID: '%s'\n", _config.wifi.ssid.c_str());
    Serial.printf("  Timezone: %s (%s)\n", _config.system.timezoneId.c_str(), _config.system.timezone.c_str());
    Serial.printf("  GoodWe: %s (host: %s:%u)\n", 
                  _config.goodwe.enabled ? "Povoleno" : "Zakázáno", 
                  _config.goodwe.host.c_str(), 
                  _config.goodwe.port);
    Serial.printf("  AZRouter: %s (host: %s:%u)\n", 
                  _config.azrouter.enabled ? "Povoleno" : "Zakázáno", 
                  _config.azrouter.host.c_str(), 
                  _config.azrouter.port);
    Serial.printf("  Pocasi: %s (%s, %.5f, %.5f, interval %lu s)\n",
                  _config.weather.enabled ? "Povoleno" : "Zakazano",
                  _config.weather.provider.c_str(),
                  _config.weather.latitude,
                  _config.weather.longitude,
                  _config.weather.pollIntervalSeconds);
    return true;
}

const AppConfig& ConfigManager::get() const {
    return _config;
}

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

void ConfigManager::setWifi(const String& ssid, const String& password) {
    _config.wifi.ssid = ssid;
    _config.wifi.password = password;

    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_password", password);
    preferences.end();
}

void ConfigManager::setSources(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter) {
    _config.goodwe = goodwe;
    _config.azrouter = azrouter;

    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putBool("gw_enabled", goodwe.enabled);
    preferences.putString("gw_host", goodwe.host);
    preferences.putUShort("gw_port", goodwe.port);
    preferences.putUInt("gw_interval", goodwe.pollIntervalSeconds);
    preferences.putBool("az_enabled", azrouter.enabled);
    preferences.putString("az_host", azrouter.host);
    preferences.putUShort("az_port", azrouter.port);
    preferences.putUInt("az_interval", azrouter.pollIntervalSeconds);
    preferences.end();
}

void ConfigManager::setWeather(const WeatherConfig& weather) {
    _config.weather = weather;

    Preferences preferences;
    preferences.begin("dashboard", false);
    preferences.putBool("wx_enabled", weather.enabled);
    preferences.putString("wx_provider", weather.provider);
    preferences.putDouble("wx_lat", weather.latitude);
    preferences.putDouble("wx_lon", weather.longitude);
    preferences.putUInt("wx_interval", weather.pollIntervalSeconds);
    preferences.end();
}

bool ConfigManager::resetToFactoryDefaults() {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) {
        Serial.println("[CONFIG] Nelze otevrit NVS pro tovarni reset.");
        return false;
    }

    const bool cleared = preferences.clear();
    preferences.end();
    if (!cleared) {
        Serial.println("[CONFIG] Tovarni reset NVS selhal.");
        return false;
    }

    _config = AppConfig{};
    Serial.println("[CONFIG] NVS vymazano, obnovena tovarni konfigurace.");
    return true;
}


bool ConfigManager::setUserConfiguration(const AppConfig& config) {
    Preferences preferences;
    if (!preferences.begin("dashboard", false)) {
        Serial.println("[CONFIG] Nelze otevrit NVS pro import.");
        return false;
    }
    preferences.putString("sys_hostname", config.system.hostname);
    preferences.putString("sys_timezone", config.system.timezone);
    preferences.putString("sys_tzid", config.system.timezoneId);
    preferences.putString("sys_ntp", config.system.ntpServer);
    preferences.putString("wifi_ssid", config.wifi.ssid);
    preferences.putString("wifi_password", config.wifi.password);
    preferences.putBool("gw_enabled", config.goodwe.enabled);
    preferences.putString("gw_host", config.goodwe.host);
    preferences.putUShort("gw_port", config.goodwe.port);
    preferences.putUInt("gw_interval", config.goodwe.pollIntervalSeconds);
    preferences.putBool("az_enabled", config.azrouter.enabled);
    preferences.putString("az_host", config.azrouter.host);
    preferences.putUShort("az_port", config.azrouter.port);
    preferences.putUInt("az_interval", config.azrouter.pollIntervalSeconds);
    preferences.putBool("wx_enabled", config.weather.enabled);
    preferences.putString("wx_provider", config.weather.provider);
    preferences.putDouble("wx_lat", config.weather.latitude);
    preferences.putDouble("wx_lon", config.weather.longitude);
    preferences.putUInt("wx_interval", config.weather.pollIntervalSeconds);
    preferences.end();
    _config.system = config.system;
    _config.wifi = config.wifi;
    _config.goodwe = config.goodwe;
    _config.azrouter = config.azrouter;
    _config.weather = config.weather;
    Serial.println("[CONFIG] YAML konfigurace importovana do NVS.");
    return true;
}
