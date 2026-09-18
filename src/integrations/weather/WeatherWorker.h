#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"
#include "IWeatherProvider.h"
#include "OpenMeteoClient.h"
#include "MetNorwayClient.h"

class WeatherWorker {
public:
    bool begin(const WeatherConfig& config);
    bool reconfigure(const WeatherConfig& config);
    bool takeLatest(WeatherData& weatherData);

private:
    static constexpr uint32_t TaskStackWords = 12288;
    static constexpr UBaseType_t TaskPriority = 1;

    WeatherConfig _config;
    OpenMeteoClient _openMeteoClient;
    MetNorwayClient _metNorwayClient;
    IWeatherProvider* _provider = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;
    WeatherData _latest;
    bool _hasLatest = false;
    uint32_t _configGeneration = 0;

    static void taskEntry(void* parameter);
    void taskLoop();
    void publish(const WeatherData& weatherData);
    IWeatherProvider* providerFor(const String& providerName);
};