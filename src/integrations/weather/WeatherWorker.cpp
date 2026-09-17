#include "WeatherWorker.h"
#include <WiFi.h>

namespace {
constexpr uint32_t MinimumRetrySeconds = 60;
constexpr uint32_t MaximumRetrySeconds = 3600;
constexpr uint8_t MaximumFailureShift = 3;

uint32_t nextDelaySeconds(uint32_t configuredSeconds, uint8_t failureStreak, bool success) {
    if (success) {
        return configuredSeconds;
    }
    uint64_t delaySeconds = min(configuredSeconds, MinimumRetrySeconds);
    delaySeconds = max(delaySeconds, static_cast<uint64_t>(MinimumRetrySeconds));
    delaySeconds <<= min(failureStreak, MaximumFailureShift);
    return static_cast<uint32_t>(min(delaySeconds, static_cast<uint64_t>(MaximumRetrySeconds)));
}
}

IWeatherProvider* WeatherWorker::providerFor(const String& providerName) {
    if (providerName == "open-meteo") return &_openMeteoClient;
    if (providerName == "met-no") return &_metNorwayClient;
    return nullptr;
}

bool WeatherWorker::begin(const WeatherConfig& config) {
    if (_task != nullptr) {
        return reconfigure(config);
    }

    _config = config;
    _provider = providerFor(_config.provider);
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[WEATHER] Nelze vytvorit mutex.");
        return false;
    }

    const BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "weatherTask",
        TaskStackWords,
        this,
        TaskPriority,
        &_task,
        ARDUINO_RUNNING_CORE);
    if (result != pdPASS) {
        Serial.println("[WEATHER] Nelze vytvorit task.");
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
        _task = nullptr;
        return false;
    }
    return true;
}

bool WeatherWorker::reconfigure(const WeatherConfig& config) {
    if (_mutex == nullptr || _task == nullptr) {
        return begin(config);
    }

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        Serial.println("[WEATHER] Reconfiguration lock timeout.");
        return false;
    }
    _config = config;
    _provider = providerFor(_config.provider);
    xSemaphoreGive(_mutex);

    Serial.printf("[WEATHER] Konfigurace zmenena za behu: %s, %.5f, %.5f, interval %lu s, enabled=%d\n",
                  _config.provider.c_str(), _config.latitude, _config.longitude,
                  _config.pollIntervalSeconds, _config.enabled);

    // Probudit task z čekání, aby novou konfiguraci použil hned.
    xTaskNotifyGive(_task);
    return true;
}

bool WeatherWorker::takeLatest(WeatherData& weatherData) {
    if (_mutex == nullptr || xSemaphoreTake(_mutex, 0) != pdTRUE) {
        return false;
    }
    const bool hasLatest = _hasLatest;
    if (hasLatest) {
        weatherData = _latest;
        _hasLatest = false;
    }
    xSemaphoreGive(_mutex);
    return hasLatest;
}

void WeatherWorker::taskEntry(void* parameter) {
    static_cast<WeatherWorker*>(parameter)->taskLoop();
}

void WeatherWorker::taskLoop() {
    WeatherData working;
    uint8_t failureStreak = 0;

    for (;;) {
        WeatherConfig config;
        IWeatherProvider* provider = nullptr;
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            config = _config;
            provider = _provider;
            xSemaphoreGive(_mutex);
        }

        bool success = false;
        if (!config.enabled) {
            working.status.recordError("Weather disabled");
        } else if (!WiFi.isConnected()) {
            working.status.recordError("WiFi unavailable");
        } else if (provider == nullptr) {
            working.status.recordError("Unsupported provider");
        } else {
            success = provider->update(config, working);
        }

        publish(working);
        uint32_t delaySeconds =
            nextDelaySeconds(config.pollIntervalSeconds, failureStreak, success);
        if (success && provider != nullptr) {
            delaySeconds = provider->recommendedPollIntervalSeconds(delaySeconds);
        }
        failureStreak = success ? 0 : min<uint8_t>(failureStreak + 1, MaximumFailureShift);

        // Nová konfigurace probudí task okamžitě; jinak čekáme běžný interval.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delaySeconds * 1000UL));
    }
}

void WeatherWorker::publish(const WeatherData& weatherData) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _latest = weatherData;
        _hasLatest = true;
        xSemaphoreGive(_mutex);
    }
}