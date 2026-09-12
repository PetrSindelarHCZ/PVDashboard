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
    solar.status.available = true;

    azrouter.routedPowerW = 1400.0f;
    azrouter.routedEnergyTodayKWh = 6.2f;
    azrouter.boilerTempC = 56.5f;
    azrouter.status.available = true;
}

void DataModel::updateSystemMetrics() {
    system.uptimeSeconds = millis() / 1000;
    system.freeHeapBytes = ESP.getFreeHeap();
}
