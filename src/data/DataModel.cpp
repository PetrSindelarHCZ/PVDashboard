#include "DataModel.h"

DataModel::DataModel() {
    updateSystemMetrics();
    
    // Testovací data pro zobrazení
    solar.productionPowerW = 3400.0f;
    solar.houseConsumptionW = 1200.0f;
    solar.gridPowerW = -800.0f; // přetok 800W
    solar.energyTodayKWh = 18.4f;
    solar.batterySocPercent = 78.0f;
    solar.batteryPowerW = 1400.0f;
    azrouter.routedPowerW = 1400.0f;
    azrouter.routedEnergyTodayKWh = 6.2f;
    azrouter.boilerTempC = 56.5f;
}

void DataModel::updateSystemMetrics() {
    system.uptimeSeconds = millis() / 1000;
    system.freeHeapBytes = ESP.getFreeHeap();

    if (solar.status.available && azrouter.status.available) {
        system.statusMessage = "Vse v poradku";
    } else if (!solar.status.available && !azrouter.status.available) {
        system.statusMessage = "GoodWe a AZRouter nedostupne";
    } else if (!solar.status.available) {
        system.statusMessage = "GoodWe nedostupne";
    } else {
        system.statusMessage = "AZRouter nedostupny";
    }
}

void DataModel::sampleSolarHistory(uint16_t minuteOfDay) {
    if (!solar.status.available || minuteOfDay >= 24U * 60U) return;

    const uint16_t bucketMinute =
        static_cast<uint16_t>((minuteOfDay / SolarHistoryIntervalMinutes) * SolarHistoryIntervalMinutes);

    if (solar.historyCount > 0) {
        SolarHistorySample& last = solar.history[solar.historyCount - 1];

        // Clock wrapped to the next day: discard yesterday's volatile history.
        if (bucketMinute < last.minuteOfDay) {
            solar.historyCount = 0;
        } else if (bucketMinute == last.minuteOfDay) {
            // Keep the current 15-minute slot fresh without growing the buffer.
            last.productionPowerW = solar.productionPowerW;
            last.houseConsumptionW = solar.houseConsumptionW;
            return;
        }
    }

    if (solar.historyCount >= SolarHistorySampleCount) return;

    SolarHistorySample& sample = solar.history[solar.historyCount++];
    sample.minuteOfDay = bucketMinute;
    sample.productionPowerW = solar.productionPowerW;
    sample.houseConsumptionW = solar.houseConsumptionW;
}