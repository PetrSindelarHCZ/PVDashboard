#pragma once
#include <Arduino.h>

// Access only from the application loop (including synchronous web handlers).
namespace Performance {
enum Metric { Loop, WebGap, WebService, StatusHandler, DisplayInit, DisplayFull,
              DisplayPartial, GoodWe, AzPower, AzStatus, AzDevices, Count };
struct Stats {
    uint32_t count = 0;
    uint64_t totalMs = 0;
    uint32_t maxMs = 0;
    uint32_t lastMs = 0;
    uint32_t lastEndMs = 0;
};
const char* name(Metric metric);
const Stats& stats(Metric metric);
void record(Metric metric, uint32_t durationMs);
void webTick();
void report();
class Scope {
public:
    explicit Scope(Metric metric) : _metric(metric), _start(millis()) {}
    ~Scope() { record(_metric, millis() - _start); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
private:
    Metric _metric;
    uint32_t _start;
};
}
