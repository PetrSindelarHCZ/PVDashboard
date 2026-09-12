#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    // Inicializace výchozí konfigurace
    _config.schemaVersion = 1;
    _config.system.hostname = "dashboard";
    _config.system.timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
    _config.system.ntpServer = "pool.ntp.org";
    
    _config.display.defaultScreen = "home";
    _config.display.fullRefreshIntervalMinutes = 1440;

    _config.goodwe.enabled = false;
    _config.goodwe.host = "192.168.1.100";
    _config.goodwe.port = 8899;

    _config.azrouter.enabled = false;
    _config.azrouter.host = "192.168.1.101";
    _config.azrouter.port = 80;
}

bool ConfigManager::begin() {
    Serial.println("[CONFIG] Nactena vychozi konfigurace (Schema v1).");
    return true;
}

const AppConfig& ConfigManager::get() const {
    return _config;
}

void ConfigManager::setWifi(const String& ssid, const String& password) {
    _config.wifi.ssid = ssid;
    _config.wifi.password = password;
}
