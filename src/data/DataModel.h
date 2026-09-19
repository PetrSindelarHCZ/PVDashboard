#pragma once
#include <Arduino.h>
#include "DataSourceStatus.h"
#include "DataPoint.h"

constexpr size_t WeatherForecastDayCount = 4;
constexpr size_t WeatherHourlySlotsPerDay = 9;
constexpr size_t WeatherHourlySlotCount = WeatherForecastDayCount * WeatherHourlySlotsPerDay;
constexpr size_t SolarHistorySampleCount = 96; // 24 h * 4 samples/hour
constexpr uint16_t SolarHistoryIntervalMinutes = 15;

struct SolarHistorySample {
    uint16_t minuteOfDay = 0;
    float productionPowerW = 0.0f;
    float houseConsumptionW = 0.0f;
};

struct SolarData {
    bool enabled = true;
    DataSourceStatus status;
    float productionPowerW = 0.0f;
    float houseConsumptionW = 0.0f;
    float gridPowerW = 0.0f;
    float energyTodayKWh = 0.0f;
    float batterySocPercent = 0.0f;
    float batteryPowerW = 0.0f;
    uint32_t lastUpdateMs = 0;
    SolarHistorySample history[SolarHistorySampleCount];
    uint8_t historyCount = 0;
};

struct AZRouterData {
    bool enabled = true;
    DataSourceStatus status;
    float gridPowerW = 0.0f;
    float routedPowerW = 0.0f;
    float routedEnergyTodayKWh = 0.0f;
    float boilerTempC = 0.0f;
    uint32_t lastUpdateMs = 0;
};

struct DailyWeatherForecast {
    char date[11] = "";
    float tempMaxC = 0.0f;
    float tempMinC = 0.0f;
    float precipitationMm = 0.0f;
    float windMaxKmh = 0.0f;
    uint8_t precipitationProbabilityPercent = 0;
    bool hasPrecipitationProbability = false;
    uint8_t weatherCode = 0;
};

struct HourlyWeatherForecast {
    char date[11] = "";
    char time[6] = "";
    float tempC = 0.0f;
    float precipitationMm = 0.0f;
    float windKmh = 0.0f;
    uint8_t precipitationProbabilityPercent = 0;
    bool hasPrecipitationProbability = false;
    uint8_t weatherCode = 0;
};

struct WeatherData {
    DataSourceStatus status;
    bool enabled = false;
    String provider = "";
    String locationId = "";
    String locationName = "";
    uint8_t locationIndex = 0;
    uint8_t locationCount = 0;
    float outdoorTempC = 0.0f;
    int outdoorHumidityPercent = 0;
    float surfacePressureHpa = 0.0f;
    float windSpeedKmh = 0.0f;
    float currentPrecipitationMm = 0.0f;
    float tempMaxTodayC = 0.0f;
    float tempMinTodayC = 0.0f;
    uint8_t weatherCode = 0;
    String conditionText = "";
    DailyWeatherForecast daily[WeatherForecastDayCount];
    HourlyWeatherForecast hourly[WeatherHourlySlotCount];
    uint8_t dailyCount = 0;
    uint8_t hourlyCount = 0;
    uint32_t lastUpdateMs = 0;
};

struct InsideData {
    DataSourceStatus status;
    float temperatureC = 0.0f;
    int humidityPercent = 0;
    float pressureHpa = 0.0f;
    uint32_t lastUpdateMs = 0;

    // Existing UI-compatible fields. livingRoomTempC is populated from BME280;
    // the remaining values stay as placeholders until their real sensors exist.
    float livingRoomTempC = 0.0f;
    float bedroomTempC = 21.8f;
    float poolTempC = 25.1f;
};

struct PoolData {
    bool enabled = true;
    DataSourceStatus status;
    float waterTempC = 26.4f;
    float targetTempC = 26.5f;
    float ph = 7.2f;
    float freeChlorineMgL = 0.6f;
    float airTempC = 24.1f;
    int airHumidityPercent = 58;
    bool filtrationRunning = true;
    bool heatingActive = true;
};

struct SystemData {
    String currentScreenId = "home";

    // Runtime UI focus. This is intentionally separate from persisted
    // configuration; it is copied into display snapshots so e-ink and WebUI
    // preview render the same navigation state.
    String navigationArea = "sidebar";
    String navigationSidebarScreenId = "home";
    String navigationFocusId = "";
    uint8_t navigationSubpageIndex = 0;
    uint8_t navigationSubpageCount = 1;
    String timeStr = "";
    String dateStr = "";
    String dayOfWeekStr = "";
    bool wifiConnected = false;
    bool wifiAccessPoint = false;
    int8_t wifiRssi = 0;
    uint8_t wifiSignalLevel = 0; // 0 = offline, 1..3 = stable signal strength
    String ipAddress = "0.0.0.0";
    bool ntpSynced = false;
    uint32_t uptimeSeconds = 0;
    uint32_t freeHeapBytes = 0;
    String statusMessage = "Stav dat se nacita";
};

class DataModel {
public:
    DataModel();

    SolarData solar;
    AZRouterData azrouter;
    WeatherData weather;
    InsideData inside;
    PoolData pool;
    SystemData system;

    void updateSystemMetrics();
    void sampleSolarHistory(uint16_t minuteOfDay);
};