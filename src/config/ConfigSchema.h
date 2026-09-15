#pragma once
#include <Arduino.h>

struct SystemConfig {
    String hostname = "dashboard";
    String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
    String ntpServer = "pool.ntp.org";
};

struct WifiConfig {
    String ssid = "VASE_WIFI";
    String password = "";
};

struct DisplayConfig {
    String defaultScreen = "home";
    uint32_t fullRefreshIntervalMinutes = 1440;
};

struct GoodWeConfig {
    bool enabled = true;
    String host = "192.168.88.51";
    uint16_t port = 8899;
    uint32_t pollIntervalSeconds = 10;
};

struct AZRouterConfig {
    bool enabled = true;
    String host = "192.168.88.51";
    uint16_t port = 8081;
    uint32_t pollIntervalSeconds = 10;
};

struct WeatherConfig {
    bool enabled = false;
    String provider = "open-meteo";
    double latitude = 0.0;
    double longitude = 0.0;
    uint32_t pollIntervalSeconds = 1800;
};

struct AppConfig {
    uint8_t schemaVersion = 2;
    SystemConfig system;
    WifiConfig wifi;
    DisplayConfig display;
    GoodWeConfig goodwe;
    AZRouterConfig azrouter;
    WeatherConfig weather;
};