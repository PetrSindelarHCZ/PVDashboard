#pragma once
#include <Arduino.h>
#include "DataSourceStatus.h"
#include "DataPoint.h"

// Fotovoltaická data (GoodWe)
struct SolarData {
    DataSourceStatus status;
    float productionPowerW = 0.0f;       // Výroba panelů (W)
    float houseConsumptionW = 0.0f;      // Spotřeba domu (W)
    float gridPowerW = 0.0f;             // Nákup (+) / Přetok (-) (W)
    float energyTodayKWh = 0.0f;         // Dnešní výroba (kWh)
    float batterySocPercent = 0.0f;      // Nabití baterie (%)
    float batteryPowerW = 0.0f;          // Nabíjení (+) / Vybíjení (-) (W)
    uint32_t lastUpdateMs = 0;
};

// AZ Router data (přetoky do bojleru)
struct AZRouterData {
    DataSourceStatus status;
    float gridPowerW = 0.0f;             // Měřený tok sítí (W)
    float routedPowerW = 0.0f;           // Výkon posílaný do zátěže (W)
    float routedEnergyTodayKWh = 0.0f;   // Dnešní vytěžená energie (kWh)
    float boilerTempC = 0.0f;            // Teplota bojleru (°C)
    uint32_t lastUpdateMs = 0;
};

// Data o počasí
struct WeatherData {
    DataSourceStatus status;
    float outdoorTempC = 18.6f;
    int outdoorHumidityPercent = 63;
    float tempMaxTodayC = 22.0f;
    float tempMinTodayC = 12.0f;
    String conditionText = "Slunecno";
};

// Data z vnitřních čidel
struct InsideData {
    float livingRoomTempC = 22.4f;
    float bedroomTempC = 21.8f;
    int co2Ppm = 612;
    float poolTempC = 25.1f;
};

// Data bazénu
struct PoolData {
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

// Systémová data
struct SystemData {
    String currentScreenId = "home";
    String timeStr = "--:--";
    String dateStr = "--.--.----";
    String dayOfWeekStr = "";
    bool wifiConnected = false;
    int8_t wifiRssi = 0;
    String ipAddress = "0.0.0.0";
    bool ntpSynced = false;
    uint32_t uptimeSeconds = 0;
    uint32_t freeHeapBytes = 0;
    String statusMessage = "Vse v poradku";
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
};
