#pragma once
#include <Arduino.h>

struct DataSourceStatus {
    bool available = false;
    uint32_t lastSuccessMs = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t errorCount = 0;
    String lastError = "";

    void recordSuccess() {
        available = true;
        lastSuccessMs = millis();
        lastAttemptMs = millis();
        lastError = "";
    }

    void recordError(const String& error) {
        available = false;
        lastAttemptMs = millis();
        errorCount++;
        lastError = error;
    }
};
