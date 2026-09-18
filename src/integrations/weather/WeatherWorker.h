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
    static constexpr uint32_t TaskStackBytes = 12288;
    static constexpr UBaseType_t TaskPriority = 1;
    static constexpr uint32_t BackgroundFetchGapMs = 3000;

    struct CacheEntry {
        bool used = false;
        String locationId = "";
        double latitude = 0.0;
        double longitude = 0.0;

        // Plná předpověď se alokuje až po úspěšném načtení daného místa.
        // Neplatíme tak RAM za všech 8 možných slotů před prvním TLS requestem.
        WeatherData* data = nullptr;

        uint32_t fetchedAtMs = 0;
        uint32_t validForSeconds = 0;
        uint32_t nextAttemptMs = 0;
        uint8_t failureStreak = 0;
    };

    WeatherConfig _config;
    OpenMeteoClient _openMeteoClient;
    MetNorwayClient _metNorwayClient;
    IWeatherProvider* _provider = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;

    WeatherData _latest;
    bool _hasLatest = false;

    // Metadata má pevně max. 8 malých slotů; velká WeatherData jsou dynamická.
    CacheEntry _cache[MaxWeatherLocations];
    String _cacheProvider = "";
    uint32_t _configGeneration = 0;
    uint32_t _providerGeneration = 0;

    static void taskEntry(void* parameter);
    void taskLoop();

    IWeatherProvider* providerFor(const String& providerName);

    void releaseEntryData(CacheEntry& entry);
    void resetEntry(CacheEntry& entry);
    void clearCacheLocked();
    void reconcileCacheLocked(const WeatherConfig& config, bool providerChanged);
    int findCacheEntryLocked(const String& locationId, double latitude, double longitude) const;
    int findUnusedCacheEntryLocked() const;
    bool locationStillConfiguredLocked(const String& locationId, double latitude, double longitude) const;
    void publishCachedActiveLocked(const WeatherConfig& config);
    void storeFetchResult(
        const String& providerName,
        uint32_t providerGeneration,
        const WeatherLocation& location,
        const WeatherData& weatherData,
        bool success,
        uint32_t validForSeconds);
};
