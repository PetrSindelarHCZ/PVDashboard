#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"
#include "GoodWeClient.h"

class GoodWeWorker {
public:
    bool begin(const GoodWeConfig& config);
    bool reconfigure(const GoodWeConfig& config);
    bool requestPoll();
    bool takeLatest(SolarData& data, bool& success, uint32_t& completedMs);

private:
    static constexpr uint32_t TaskStackBytes = 6144;
    static constexpr UBaseType_t TaskPriority = 1;

    GoodWeConfig _config;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;
    volatile bool _pollRequested = false;
    volatile bool _pollInFlight = false;
    uint32_t _configGeneration = 0;

    SolarData _latest;
    bool _latestSuccess = false;
    uint32_t _latestCompletedMs = 0;
    bool _hasLatest = false;

    static void taskEntry(void* parameter);
    void taskLoop();
};
