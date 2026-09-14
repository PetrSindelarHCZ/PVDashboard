#include "Performance.h"

namespace Performance {
static Stats values[Count];
static portMUX_TYPE valuesMux = portMUX_INITIALIZER_UNLOCKED;
static const char* names[Count] = {"loop", "webGap", "webService", "statusHandler",
    "displayInit", "displayFull", "displayPartial", "goodwe",
    "azPower", "azStatus", "azDevices"};

const char* name(Metric metric) {
    return names[metric];
}

Stats stats(Metric metric) {
    portENTER_CRITICAL(&valuesMux);
    const Stats snapshot = values[metric];
    portEXIT_CRITICAL(&valuesMux);
    return snapshot;
}

void record(Metric metric, uint32_t durationMs) {
    const uint32_t endedMs = millis();
    portENTER_CRITICAL(&valuesMux);
    auto& value = values[metric];
    ++value.count;
    value.totalMs += durationMs;
    value.lastMs = durationMs;
    value.lastEndMs = endedMs;
    if (durationMs > value.maxMs) {
        value.maxMs = durationMs;
    }
    portEXIT_CRITICAL(&valuesMux);

    // Avoid duplicate messages for the enclosing loop and web service gap.
    if (durationMs >= 500 && metric != Loop && metric != WebGap) {
        Serial.printf("[PERF][%lu ms] slow %s: %lu ms\n",
            (unsigned long)endedMs, name(metric), (unsigned long)durationMs);
    }
}

void webTick() {
    static bool started = false;
    static uint32_t previous = 0;
    const uint32_t now = millis();
    if (started) {
        record(WebGap, now - previous);
    }
    previous = now;
    started = true;
}

void report() {
    static uint32_t previous = 0;
    if (millis() - previous < 30000) {
        return;
    }
    previous = millis();
    Serial.printf("[PERF][%lu ms] cumulative count/avgMs/maxMs\n", (unsigned long)previous);
    for (int i = 0; i < Count; ++i) {
        const Stats value = stats(static_cast<Metric>(i));
        if (!value.count) {
            continue;
        }
        Serial.printf("[PERF] %s: %lu/%.1f/%lu\n", names[i],
            (unsigned long)value.count, (double)value.totalMs / value.count,
            (unsigned long)value.maxMs);
    }
}
}