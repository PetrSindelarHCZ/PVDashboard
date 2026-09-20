#include "DataModel.h"

DataModel::DataModel() {
    // All telemetry starts unavailable/zero. Real integrations populate values
    // only after a successful poll; UI must use status/validity flags.
    updateSystemMetrics();
}

void DataModel::updateSystemMetrics() {
    system.uptimeSeconds = millis() / 1000;
    system.freeHeapBytes = ESP.getFreeHeap();

    if (system.ntpSynced && system.timeStr.length() >= 5 &&
        system.timeStr[0] >= '0' && system.timeStr[0] <= '9' &&
        system.timeStr[1] >= '0' && system.timeStr[1] <= '9' &&
        system.timeStr[3] >= '0' && system.timeStr[3] <= '9' &&
        system.timeStr[4] >= '0' && system.timeStr[4] <= '9') {
        const uint16_t hour = static_cast<uint16_t>((system.timeStr[0] - '0') * 10 + (system.timeStr[1] - '0'));
        const uint16_t minute = static_cast<uint16_t>((system.timeStr[3] - '0') * 10 + (system.timeStr[4] - '0'));
        if (hour < 24 && minute < 60) {
            sampleSolarHistory(static_cast<uint16_t>(hour * 60 + minute));
        }
    }

    if (!solar.enabled && !azrouter.enabled) {
        system.statusMessage = "Fotovoltaika vypnuta";
    } else if (solar.enabled && azrouter.enabled) {
        if (solar.status.available && azrouter.status.available) {
            system.statusMessage = "Vse v poradku";
        } else if (!solar.status.available && !azrouter.status.available) {
            system.statusMessage = "GoodWe a AZRouter nedostupne";
        } else if (!solar.status.available) {
            system.statusMessage = "GoodWe nedostupne";
        } else {
            system.statusMessage = "AZRouter nedostupny";
        }
    } else if (solar.enabled) {
        system.statusMessage = solar.status.available
            ? "GoodWe data dostupna"
            : "GoodWe nedostupne";
    } else {
        system.statusMessage = azrouter.status.available
            ? "AZRouter data dostupna"
            : "AZRouter nedostupny";
    }
}

void DataModel::sampleSolarHistory(uint16_t minuteOfDay) {
    if (!solar.enabled || !solar.status.available || minuteOfDay >= 24U * 60U) return;

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