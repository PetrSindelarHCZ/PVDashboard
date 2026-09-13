#include "ConfigManager.h"
#include <Preferences.h>

ConfigManager::ConfigManager() {
    // Inicializace výchozí konfigurace dle ConfigSchema.h
    // _config již obsahuje výchozí hodnoty z definice struct AppConfig
}

bool ConfigManager::begin() {
    Preferences preferences;
    preferences.begin("dashboard", false);
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
    preferences.end();

    Serial.printf("[CONFIG] Načtena konfigurace (Schema v%u):\n", _config.schemaVersion);
    Serial.printf("  Wi-Fi SSID: '%s'\n", _config.wifi.ssid.c_str());
    Serial.printf("  GoodWe: %s (host: %s:%u)\n", 
                  _config.goodwe.enabled ? "Povoleno" : "Zakázáno", 
                  _config.goodwe.host.c_str(), 
                  _config.goodwe.port);
    Serial.printf("  AZRouter: %s (host: %s:%u)\n", 
                  _config.azrouter.enabled ? "Povoleno" : "Zakázáno", 
                  _config.azrouter.host.c_str(), 
                  _config.azrouter.port);
    return true;
}

const AppConfig& ConfigManager::get() const {
    return _config;
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
