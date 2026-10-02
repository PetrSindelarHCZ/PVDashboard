#pragma once
#include <Arduino.h>
#include "DataSourceStatus.h"
#include "DataPoint.h"
#include "../integrations/cc1101/RfSensorTypes.h"

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
    bool batteryPresent = false;
    float batterySocPercent = 0.0f;
    float batteryPowerW = 0.0f;
    uint32_t lastUpdateMs = 0;
    SolarHistorySample history[SolarHistorySampleCount];
    uint8_t historyCount = 0;
};

struct AZRouterData {
    bool enabled = true;
    DataSourceStatus status;
    bool authenticated = false;
    String authMode = "anonymous";

    float gridPowerW = 0.0f;
    bool hasGridPower = false;
    float gridPhasePowerW[3] = {};
    bool hasGridPhasePower[3] = {};
    float gridPhaseVoltageV[3] = {};
    bool hasGridPhaseVoltage[3] = {};
    float gridPhaseCurrentA[3] = {};
    bool hasGridPhaseCurrent[3] = {};
    bool gridPhaseConnected[3] = {};
    bool hasGridPhaseStatus[3] = {};

    float routedPowerW = 0.0f;
    bool hasRoutedPower = false;
    float routedPhasePowerW[3] = {};
    bool hasRoutedPhasePower[3] = {};

    float routedEnergyTotalKWh = 0.0f;
    bool hasRoutedEnergyTotal = false;
    float routedEnergyYearKWh = 0.0f;
    bool hasRoutedEnergyYear = false;
    float routedEnergyMonthKWh = 0.0f;
    bool hasRoutedEnergyMonth = false;
    float routedEnergyWeekKWh = 0.0f;
    bool hasRoutedEnergyWeek = false;
    float routedEnergyTodayKWh = 0.0f;
    bool hasRoutedEnergyToday = false;

    int8_t systemStatusCode = -1; // 0 online, 1 offline, 2 updating
    bool hasSystemStatus = false;
    bool hdoOn = false;
    bool hasHdo = false;
    int8_t modeCode = -1; // 0 summer, 1 winter
    bool hasMode = false;
    bool masterBoost = false;
    bool hasMasterBoost = false;

    float systemTempC = 0.0f;
    bool hasSystemTemp = false;

    // Device-level telemetry is still received for diagnostics/future use,
    // but is intentionally not presented as AZRouter master data.
    float boilerTempC = 0.0f;
    bool hasBoilerTemp = false;

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

constexpr uint8_t SensorGraphSampleCount = 24;
constexpr uint8_t SensorGraphPeriodCount = 8;
constexpr uint8_t SensorGraphPeriodHours[SensorGraphPeriodCount] = {
    1, 2, 4, 6, 12, 24, 48, 72
};

inline int8_t sensorGraphPeriodIndex(uint8_t hours) {
    for (uint8_t i = 0; i < SensorGraphPeriodCount; ++i) {
        if (SensorGraphPeriodHours[i] == hours) return static_cast<int8_t>(i);
    }
    return -1;
}

inline uint32_t sensorGraphBucketSeconds(uint8_t hours) {
    // 24 fixed slots per graph. 1 h => 150 s, 12 h => 1800 s, 72 h => 10800 s.
    return (static_cast<uint32_t>(hours) * 3600UL) / SensorGraphSampleCount;
}

struct InsideHistorySample {
    int16_t temperatureCenti = 0;
    uint8_t humidityPercent = 0;
    uint16_t pressureDeciHpa = 0;
    uint8_t flags = 0; // bit0 temperature, bit1 humidity, bit2 pressure
};

struct InsideHistorySeries {
    InsideHistorySample samples[SensorGraphSampleCount];
    uint8_t count = 0;
    uint8_t next = 0;
    uint32_t lastBucket = 0;
    bool wallClock = false;
};

struct InsideHistory {
    InsideHistorySeries series[SensorGraphPeriodCount];
};

struct InsideData {
    DataSourceStatus status;
    float temperatureC = 0.0f;
    int humidityPercent = 0;
    float pressureHpa = 0.0f;
    uint32_t lastUpdateMs = 0;

    // Heap-backed history keeps trend/graph support out of static DRAM.
    InsideHistory* history = nullptr;

    // Existing UI-compatible fields. livingRoomTempC is populated from BME280;
    // the remaining values stay as placeholders until their real sensors exist.
    float livingRoomTempC = 0.0f;
    float bedroomTempC = 21.8f;
    float poolTempC = 25.1f;
};

struct FuelGaugeData {
    DataSourceStatus status;
    float voltageV = 0.0f;
    float socPercent = 0.0f;
    float changeRatePercentPerHour = 0.0f;
    uint8_t alertFlags = 0;
    bool alertPending = false;
    uint16_t version = 0;
    uint32_t lastUpdateMs = 0;
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


struct RfSensorData {
    bool configured = false;
    bool available = false;
    String slotId = "";

    bool hasTemperature = false;
    float temperatureC = 0.0f;

    bool hasHumidity = false;
    int humidityPercent = 0;

    bool hasBattery = false;
    bool batteryOk = true;

    uint32_t lastUpdateMs = 0;
};

struct RfHistorySample {
    int16_t temperatureCenti = 0;
    uint8_t humidityPercent = 0;
    uint8_t flags = 0; // bit0 temperature, bit1 humidity
};

struct RfHistorySeries {
    RfHistorySample samples[SensorGraphSampleCount];
    uint8_t count = 0;
    uint8_t next = 0;
    uint32_t lastBucket = 0;
    bool wallClock = false;
};

struct RfSensorHistory {
    RfHistorySeries series[SensorGraphPeriodCount];
};

struct RfSensorsData {
    uint8_t sensorCount = 0;
    RfSensorData sensors[MaxRfSensors];

    // Allocated by RfSensorManager on the heap to avoid growing static DRAM.
    // Indexed by stable slot number: sensor1 -> 0 ... sensor16 -> 15.
    RfSensorHistory* history = nullptr;
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
    FuelGaugeData battery;
    PoolData pool;
    RfSensorsData rfSensors;
    SystemData system;

    void updateSystemMetrics();
    void sampleSolarHistory(uint16_t minuteOfDay);
};