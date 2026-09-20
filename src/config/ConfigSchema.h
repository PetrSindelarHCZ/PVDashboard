#pragma once
#include <Arduino.h>
#include <vector>

struct SystemConfig {
    String hostname = "dashboard";
    String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
    String timezoneId = "Europe/Prague";
    String ntpServer = "pool.ntp.org";
};

struct WifiConfig {
    String ssid = "";
    String password = "";
    bool dhcp = true;
    String ipAddress = "";
    String subnetMask = "";
    String gateway = "";
    String dns1 = "";
    String dns2 = "";
};

constexpr uint8_t MaxHomeLayoutWidgets = 6;
constexpr uint8_t MaxCustomWidgetElements = 8;

struct CustomWidgetElementConfig {
    String id = "";
    String type = "";       // text | kpi | progress | sparkline
    String source = "";     // e.g. solar.productionPowerW
    String label = "";
    String unit = "";
    String text = "";
    int16_t x = 10;         // relative to the custom widget
    int16_t y = 45;
    int16_t width = 120;
    int16_t height = 50;
    uint8_t decimals = 1;
    float minValue = 0.0f;
    float maxValue = 100.0f;
    String fontSize = "auto";   // auto | 7..64 (pixel height)
    String align = "left";      // left | center | right
    bool showLabel = true;
    String graphStyle = "line"; // line | bars (sparkline only)
};

struct HomeLayoutWidgetConfig {
    String id = "";
    String type = "";
    bool visible = true;
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;

    // Card appearance is supported by both predefined and custom Home widgets.
    bool showFrame = true;
    String background = "white"; // white | black
    bool inverseText = false;

    // Used only by type == "custom". Predefined widgets keep these empty.
    String title = "";
    std::vector<CustomWidgetElementConfig> elements;
};

struct HomeLayoutConfig {
    bool customized = false;
    uint8_t widgetCount = 0;
    HomeLayoutWidgetConfig widgets[MaxHomeLayoutWidgets];
};

struct DisplayConfig {
    String defaultScreen = "home";
    uint32_t fullRefreshIntervalMinutes = 1440;
    HomeLayoutConfig homeLayout;
};

struct GoodWeConfig {
    bool enabled = true;
    String host = "";
    uint16_t port = 8899;
    uint32_t pollIntervalSeconds = 10;
};

struct AZRouterConfig {
    bool enabled = true;
    String host = "";
    uint16_t port = 8081;
    uint32_t pollIntervalSeconds = 10;
    String username = "";
    String password = "";
};

constexpr uint8_t MaxWeatherLocations = 8;

struct WeatherLocation {
    String id = "";
    String name = "";
    String country = "";
    double latitude = 0.0;
    double longitude = 0.0;
};

struct WeatherConfig {
    bool enabled = false;
    String provider = "open-meteo";

    // latitude/longitude zůstávají jako efektivní souřadnice aktivního místa,
    // aby poskytovatelé počasí nemuseli znát seznam lokalit.
    double latitude = 50.0755;
    double longitude = 14.4378;
    uint32_t pollIntervalSeconds = 1800;

    WeatherLocation locations[MaxWeatherLocations];
    uint8_t locationCount = 1;
    String activeLocationId = "praha";

    WeatherConfig() {
        locations[0].id = "praha";
        locations[0].name = "Praha";
        locations[0].country = "Česko";
        locations[0].latitude = latitude;
        locations[0].longitude = longitude;
    }

    const WeatherLocation* activeLocation() const {
        for (uint8_t i = 0; i < locationCount && i < MaxWeatherLocations; ++i) {
            if (locations[i].id == activeLocationId) return &locations[i];
        }
        return locationCount > 0 ? &locations[0] : nullptr;
    }

    void syncActiveCoordinates() {
        const WeatherLocation* location = activeLocation();
        if (!location) return;
        latitude = location->latitude;
        longitude = location->longitude;
        if (activeLocationId.isEmpty()) activeLocationId = location->id;
    }
};

struct PoolConfig {
    // Řídí viditelnost bazénových informací v UI/displeji.
    // Další bazénová nastavení (čidla, limity, technologie) lze později
    // přidat sem bez změny rozhraní ConfigManageru.
    bool enabled = true;
};

struct AppConfig {
    uint8_t schemaVersion = 11;
    SystemConfig system;
    WifiConfig wifi;
    DisplayConfig display;
    GoodWeConfig goodwe;
    AZRouterConfig azrouter;
    PoolConfig pool;
    WeatherConfig weather;
};
