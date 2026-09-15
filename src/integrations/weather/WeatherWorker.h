#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"
#include "OpenMeteoClient.h"

class WeatherWorker {
public:
    bool begin(const WeatherConfig& config);
    bool takeLatest(WeatherData& weatherData);

private:
    static constexpr uint32_t TaskStackWords = 6144;
    static constexpr UBaseType_t TaskPriority = 1;

    WeatherConfig _config;
    OpenMeteoClient _client;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;
    WeatherData _latest;
    bool _hasLatest = false;

    static void taskEntry(void* parameter);
    void taskLoop();
    void publish(const WeatherData& weatherData);
};