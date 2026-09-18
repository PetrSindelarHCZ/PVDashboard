#include "WeatherWorker.h"
#include <WiFi.h>
#include <math.h>
#include <new>
#include <time.h>

namespace {
constexpr uint32_t MinimumRetrySeconds = 60;
constexpr uint32_t MaximumRetrySeconds = 3600;
constexpr uint8_t MaximumFailureShift = 3;
constexpr uint32_t OfflineRetryMs = 10000;
constexpr time_t MinimumTlsEpoch = 1704067200; // 2024-01-01 UTC

bool tlsClockReady() {
    return time(nullptr) >= MinimumTlsEpoch;
}

uint32_t nextDelaySeconds(
    uint32_t configuredSeconds,
    uint8_t failureStreak,
    bool success) {
    if (success) return configuredSeconds;

    uint64_t delaySeconds = min(configuredSeconds, MinimumRetrySeconds);
    delaySeconds = max(
        delaySeconds,
        static_cast<uint64_t>(MinimumRetrySeconds));
    delaySeconds <<= min(failureStreak, MaximumFailureShift);

    return static_cast<uint32_t>(
        min(delaySeconds, static_cast<uint64_t>(MaximumRetrySeconds)));
}

bool dueNow(uint32_t now, uint32_t target) {
    return target == 0 || static_cast<int32_t>(now - target) >= 0;
}

uint32_t msUntil(uint32_t now, uint32_t target) {
    if (target == 0 || dueNow(now, target)) return 0;
    return target - now;
}

const char* providerLabel(const String& provider) {
    if (provider == "met-no") return "MET Norway";
    if (provider == "open-meteo") return "Open-Meteo";
    return provider.c_str();
}
}

IWeatherProvider* WeatherWorker::providerFor(const String& providerName) {
    if (providerName == "open-meteo") return &_openMeteoClient;
    if (providerName == "met-no") return &_metNorwayClient;
    return nullptr;
}

void WeatherWorker::setMemoryHeavyGate(SemaphoreHandle_t gate) {
    _memoryHeavyGate = gate;
}

bool WeatherWorker::takeMemoryHeavyGate() {
    if (_memoryHeavyGate == nullptr) return true;

    bool waitingLogged = false;
    while (!_stopRequested) {
        if (xSemaphoreTake(
                _memoryHeavyGate,
                pdMS_TO_TICKS(250)) == pdTRUE) {
            if (waitingLogged) {
                Serial.println("[WEATHER] Memory gate ziskan, TLS muze zacit.");
            }
            return true;
        }

        if (!waitingLogged) {
            Serial.println("[WEATHER] Cekam na memory gate pred TLS...");
            waitingLogged = true;
        }
    }

    return false;
}

bool WeatherWorker::begin(const WeatherConfig& config) {
    if (_task != nullptr) return reconfigure(config);

    _config = config;
    _provider = providerFor(_config.provider);
    _configGeneration = 0;
    _providerGeneration = 0;
    _cacheProvider = "";
    _priorityLocationId = "";

    if (!_config.enabled) {
        _provider = nullptr;
        _latest = WeatherData();
        _hasLatest = false;
        Serial.printf(
            "[WEATHER] Modul je vypnuty; worker se nevytvari. free=%u, maxBlock=%u\n",
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
        return true;
    }

    _stopRequested = false;
    _configGeneration = 1;
    _providerGeneration = 1;
    _cacheProvider = _config.provider;

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[WEATHER] Nelze vytvorit mutex.");
        return false;
    }

    clearCacheLocked();
    reconcileCacheLocked(_config, false);

    const BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "weatherTask",
        TaskStackBytes,
        this,
        TaskPriority,
        &_task,
        ARDUINO_RUNNING_CORE);
    if (result != pdPASS) {
        Serial.println("[WEATHER] Nelze vytvorit task.");
        clearCacheLocked();
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
        _task = nullptr;
        return false;
    }

    Serial.printf(
        "[WEATHER] Cache pripravena: provider=%s, lokalit=%u | "
        "free=%u, maxBlock=%u\n",
        _config.provider.c_str(),
        static_cast<unsigned>(_config.locationCount),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap());
    return true;
}

bool WeatherWorker::reconfigure(const WeatherConfig& config) {
    if (!config.enabled) {
        const bool stopped = stop();
        if (!stopped) return false;

        _config = config;
        _provider = nullptr;
        _latest = WeatherData();
        _hasLatest = false;

        Serial.printf(
            "[WEATHER] Modul vypnut; task, cache a mutex uvolneny. "
            "free=%u, maxBlock=%u\n",
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
        return true;
    }

    if (_mutex == nullptr || _task == nullptr) return begin(config);

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        Serial.println("[WEATHER] Reconfiguration lock timeout.");
        return false;
    }

    const bool providerChanged = _config.provider != config.provider;
    const bool activeChanged =
        _config.activeLocationId != config.activeLocationId;

    _config = config;
    _provider = providerFor(_config.provider);
    ++_configGeneration;
    if (providerChanged) ++_providerGeneration;

    reconcileCacheLocked(_config, providerChanged);
    publishCachedActiveLocked(_config);

    uint8_t validCount = 0;
    for (const auto& entry : _cache) {
        if (entry.used && entry.data != nullptr) ++validCount;
    }

    const uint32_t generation = _configGeneration;
    const uint32_t providerGeneration = _providerGeneration;
    xSemaphoreGive(_mutex);

    Serial.printf(
        "[WEATHER] Konfigurace za behu: gen=%lu providerGen=%lu, %s, "
        "active=%s, interval=%lu s, cache=%u/%u%s%s\n",
        static_cast<unsigned long>(generation),
        static_cast<unsigned long>(providerGeneration),
        _config.provider.c_str(),
        _config.activeLocationId.c_str(),
        static_cast<unsigned long>(_config.pollIntervalSeconds),
        validCount,
        static_cast<unsigned>(_config.locationCount),
        providerChanged ? " provider-change" : "",
        activeChanged ? " active-change" : "");

    xTaskNotifyGive(_task);
    return true;
}

bool WeatherWorker::stop(uint32_t timeoutMs) {
    TaskHandle_t task = _task;
    if (task == nullptr) {
        return true;
    }

    Serial.println("[WEATHER] Pozadavek na korektni zastaveni workeru...");
    _stopRequested = true;
    xTaskNotifyGive(task);

    const uint32_t started = millis();
    while (_task != nullptr &&
           millis() - started < timeoutMs) {
        delay(10);
    }

    if (_task != nullptr) {
        Serial.printf(
            "[WEATHER] Worker se nepodarilo zastavit do %lu ms; "
            "task nebyl nasilne ukoncen.\n",
            static_cast<unsigned long>(timeoutMs));
        return false;
    }

    Serial.printf(
        "[WEATHER] Worker zastaven korektne za %lu ms.\n",
        static_cast<unsigned long>(millis() - started));
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

bool WeatherWorker::copyCached(
    const String& locationId,
    WeatherData& weatherData) {
    if (_mutex == nullptr ||
        xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return false;
    }

    bool found = false;
    for (uint8_t i = 0;
         i < _config.locationCount && i < MaxWeatherLocations;
         ++i) {
        const WeatherLocation& location = _config.locations[i];
        if (location.id != locationId) continue;

        const int cacheIndex = findCacheEntryLocked(
            location.id,
            location.latitude,
            location.longitude);
        if (cacheIndex >= 0 && _cache[cacheIndex].data != nullptr) {
            weatherData = *_cache[cacheIndex].data;
            weatherData.enabled = _config.enabled;
            weatherData.locationId = location.id;
            weatherData.locationName = location.name;
            found = true;
        }
        break;
    }

    xSemaphoreGive(_mutex);
    return found;
}

bool WeatherWorker::requestLocation(const String& locationId) {
    if (_mutex == nullptr ||
        xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool configured = false;
    for (uint8_t i = 0;
         i < _config.locationCount && i < MaxWeatherLocations;
         ++i) {
        if (_config.locations[i].id == locationId) {
            configured = true;
            _priorityLocationId = locationId;

            const WeatherLocation& location = _config.locations[i];
            const int index = findCacheEntryLocked(
                location.id,
                location.latitude,
                location.longitude);
            if (index >= 0 && _cache[index].data == nullptr) {
                _cache[index].nextAttemptMs = 0;
            }
            break;
        }
    }

    xSemaphoreGive(_mutex);
    if (configured && _task != nullptr) xTaskNotifyGive(_task);
    return configured;
}

void WeatherWorker::releaseEntryData(CacheEntry& entry) {
    delete entry.data;
    entry.data = nullptr;
}

void WeatherWorker::resetEntry(CacheEntry& entry) {
    releaseEntryData(entry);
    entry.used = false;
    entry.locationId = "";
    entry.latitude = 0.0;
    entry.longitude = 0.0;
    entry.fetchedAtMs = 0;
    entry.validForSeconds = 0;
    entry.nextAttemptMs = 0;
    entry.failureStreak = 0;
}

void WeatherWorker::clearCacheLocked() {
    for (auto& entry : _cache) resetEntry(entry);
}

int WeatherWorker::findCacheEntryLocked(
    const String& locationId,
    double latitude,
    double longitude) const {
    for (uint8_t i = 0; i < MaxWeatherLocations; ++i) {
        const auto& entry = _cache[i];
        if (!entry.used) continue;

        if (entry.locationId == locationId &&
            fabs(entry.latitude - latitude) <= 0.00001 &&
            fabs(entry.longitude - longitude) <= 0.00001) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int WeatherWorker::findUnusedCacheEntryLocked() const {
    for (uint8_t i = 0; i < MaxWeatherLocations; ++i) {
        if (!_cache[i].used) return static_cast<int>(i);
    }

    return -1;
}

bool WeatherWorker::locationStillConfiguredLocked(
    const String& locationId,
    double latitude,
    double longitude) const {
    for (uint8_t i = 0;
         i < _config.locationCount && i < MaxWeatherLocations;
         ++i) {
        const auto& location = _config.locations[i];
        if (location.id == locationId &&
            fabs(location.latitude - latitude) <= 0.00001 &&
            fabs(location.longitude - longitude) <= 0.00001) {
            return true;
        }
    }

    return false;
}

void WeatherWorker::reconcileCacheLocked(
    const WeatherConfig& config,
    bool providerChanged) {
    if (providerChanged || _cacheProvider != config.provider) {
        clearCacheLocked();
        _cacheProvider = config.provider;
    }

    for (auto& entry : _cache) {
        if (!entry.used) continue;

        bool found = false;
        for (uint8_t i = 0;
             i < config.locationCount && i < MaxWeatherLocations;
             ++i) {
            const auto& location = config.locations[i];
            if (entry.locationId == location.id &&
                fabs(entry.latitude - location.latitude) <= 0.00001 &&
                fabs(entry.longitude - location.longitude) <= 0.00001) {
                found = true;
                break;
            }
        }

        if (!found) resetEntry(entry);
    }

    for (uint8_t i = 0;
         i < config.locationCount && i < MaxWeatherLocations;
         ++i) {
        const auto& location = config.locations[i];

        if (findCacheEntryLocked(
                location.id,
                location.latitude,
                location.longitude) >= 0) {
            continue;
        }

        const int slot = findUnusedCacheEntryLocked();
        if (slot < 0) break;

        auto& entry = _cache[slot];
        resetEntry(entry);
        entry.used = true;
        entry.locationId = location.id;
        entry.latitude = location.latitude;
        entry.longitude = location.longitude;
        entry.nextAttemptMs = 0;
    }

    const uint32_t now = millis();
    for (auto& entry : _cache) {
        if (!entry.used || entry.data == nullptr) continue;

        if (config.pollIntervalSeconds > entry.validForSeconds) {
            entry.validForSeconds = config.pollIntervalSeconds;
            entry.nextAttemptMs =
                entry.fetchedAtMs + entry.validForSeconds * 1000UL;

            if (dueNow(now, entry.nextAttemptMs)) {
                entry.nextAttemptMs = now;
            }
        }
    }
}

void WeatherWorker::publishCachedActiveLocked(
    const WeatherConfig& config) {
    if (!config.enabled) return;

    const WeatherLocation* active = config.activeLocation();
    if (active == nullptr) return;

    const int index = findCacheEntryLocked(
        active->id,
        active->latitude,
        active->longitude);

    if (index < 0 || _cache[index].data == nullptr) {
        _latest = WeatherData();
        _hasLatest = false;
        Serial.printf(
            "[WEATHER] Cache MISS aktivni lokalita: %s\n",
            active->name.c_str());
        return;
    }

    WeatherData cached = *_cache[index].data;
    cached.enabled = true;
    cached.locationId = active->id;
    cached.locationName = active->name;

    _latest = cached;
    _hasLatest = true;

    const uint32_t ageSeconds =
        (millis() - _cache[index].fetchedAtMs) / 1000UL;

    Serial.printf(
        "[WEATHER] Cache HIT aktivni lokalita: %s, stari %lu s\n",
        active->name.c_str(),
        static_cast<unsigned long>(ageSeconds));
}

void WeatherWorker::storeFetchResult(
    const String& providerName,
    uint32_t providerGeneration,
    const WeatherLocation& location,
    const WeatherData& weatherData,
    bool success,
    uint32_t validForSeconds) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return;

    if (providerName != _config.provider ||
        providerGeneration != _providerGeneration ||
        !locationStillConfiguredLocked(
            location.id,
            location.latitude,
            location.longitude)) {
        Serial.printf(
            "[WEATHER] Zahazuji vysledek mimo aktualni cache: %s / %s\n",
            providerName.c_str(),
            location.name.c_str());
        xSemaphoreGive(_mutex);
        return;
    }

    const int index = findCacheEntryLocked(
        location.id,
        location.latitude,
        location.longitude);
    if (index < 0) {
        xSemaphoreGive(_mutex);
        return;
    }

    auto& entry = _cache[index];
    const uint32_t now = millis();
    const bool isActive =
        _config.activeLocationId == location.id;

    if (success) {
        if (entry.data == nullptr) {
            entry.data = new (std::nothrow) WeatherData();
        }

        if (entry.data != nullptr) {
            *entry.data = weatherData;
            entry.data->enabled = _config.enabled;
            entry.data->locationId = location.id;
            entry.data->locationName = location.name;
        } else {
            Serial.printf(
                "[WEATHER] Cache alokace selhala: %s | free=%u, maxBlock=%u\n",
                location.name.c_str(),
                ESP.getFreeHeap(),
                ESP.getMaxAllocHeap());
        }

        entry.fetchedAtMs = now;
        entry.validForSeconds = max<uint32_t>(
            max<uint32_t>(
                validForSeconds,
                _config.pollIntervalSeconds),
            MinimumRetrySeconds);
        entry.nextAttemptMs =
            entry.data != nullptr
                ? now + entry.validForSeconds * 1000UL
                : now + MinimumRetrySeconds * 1000UL;
        entry.failureStreak = 0;

        // Aktivní místo publikujeme i tehdy, kdyby jeho dlouhodobá cache
        // kvůli nízké paměti nešla alokovat.
        if (isActive) {
            _latest = weatherData;
            _latest.enabled = _config.enabled;
            _latest.locationId = location.id;
            _latest.locationName = location.name;
            _hasLatest = true;
        }

        Serial.printf(
            "[WEATHER] Cache STORE: %s | %s | TTL %lu s%s | "
            "free=%u, maxBlock=%u\n",
            providerName.c_str(),
            location.name.c_str(),
            static_cast<unsigned long>(entry.validForSeconds),
            isActive ? " | ACTIVE" : "",
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
    } else {
        const uint8_t shift =
            min(entry.failureStreak, MaximumFailureShift);

        const uint32_t retrySeconds =
            nextDelaySeconds(
                _config.pollIntervalSeconds,
                shift,
                false);

        entry.failureStreak =
            min<uint8_t>(
                entry.failureStreak + 1,
                MaximumFailureShift);
        entry.nextAttemptMs =
            now + retrySeconds * 1000UL;

        if (isActive && entry.data == nullptr) {
            WeatherData failed = weatherData;
            failed.enabled = _config.enabled;
            failed.locationId = location.id;
            failed.locationName = location.name;
            _latest = failed;
            _hasLatest = true;
        } else if (isActive && entry.data != nullptr) {
            _latest = *entry.data;
            _latest.enabled = _config.enabled;
            _latest.locationId = location.id;
            _latest.locationName = location.name;
            _hasLatest = true;
        }

        Serial.printf(
            "[WEATHER] Cache RETRY: %s | %s | za %lu s | %s\n",
            providerName.c_str(),
            location.name.c_str(),
            static_cast<unsigned long>(retrySeconds),
            weatherData.status.lastError.c_str());
    }

    xSemaphoreGive(_mutex);
}

void WeatherWorker::taskEntry(void* parameter) {
    static_cast<WeatherWorker*>(parameter)->taskLoop();
}

void WeatherWorker::taskLoop() {
    bool waitingForClockLogged = false;

    for (;;) {
        if (_stopRequested) break;

        WeatherConfig config;
        IWeatherProvider* provider = nullptr;
        uint32_t providerGeneration = 0;
        WeatherLocation target;
        bool haveTarget = false;
        bool targetIsActive = false;
        uint32_t waitMs = 60000;

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            config = _config;
            provider = _provider;
            providerGeneration = _providerGeneration;

            if (config.enabled) {
                const uint32_t now = millis();
                const WeatherLocation* active =
                    config.activeLocation();

                // A location currently selected by the display pager gets one
                // immediate fetch opportunity. This does not change the
                // persisted active location used by Home.
                if (!_priorityLocationId.isEmpty()) {
                    for (uint8_t i = 0;
                         i < config.locationCount &&
                         i < MaxWeatherLocations;
                         ++i) {
                        const auto& location = config.locations[i];
                        if (location.id != _priorityLocationId) continue;

                        const int index = findCacheEntryLocked(
                            location.id,
                            location.latitude,
                            location.longitude);
                        if (index >= 0 &&
                            dueNow(now, _cache[index].nextAttemptMs)) {
                            target = location;
                            haveTarget = true;
                            targetIsActive =
                                location.id == config.activeLocationId;
                        }

                        _priorityLocationId = "";
                        break;
                    }
                }

                // Aktivní lokalita má prioritu, ale respektuje retry/backoff.
                if (!haveTarget && active != nullptr) {
                    const int activeIndex =
                        findCacheEntryLocked(
                            active->id,
                            active->latitude,
                            active->longitude);

                    if (activeIndex >= 0) {
                        const auto& entry = _cache[activeIndex];
                        if (dueNow(now, entry.nextAttemptMs)) {
                            target = *active;
                            haveTarget = true;
                            targetIsActive = true;
                        }
                    }
                }

                // Potom doplňovat ostatní lokality po jedné.
                if (!haveTarget) {
                    for (uint8_t i = 0;
                         i < config.locationCount &&
                         i < MaxWeatherLocations;
                         ++i) {
                        const auto& location =
                            config.locations[i];

                        if (location.id ==
                            config.activeLocationId) {
                            continue;
                        }

                        const int index =
                            findCacheEntryLocked(
                                location.id,
                                location.latitude,
                                location.longitude);
                        if (index < 0) continue;

                        const auto& entry = _cache[index];
                        if (dueNow(now, entry.nextAttemptMs)) {
                            target = location;
                            haveTarget = true;
                            break;
                        }
                    }
                }

                if (!haveTarget) {
                    bool haveDeadline = false;
                    uint32_t shortest = 0xFFFFFFFFUL;

                    for (const auto& entry : _cache) {
                        if (!entry.used) continue;

                        const uint32_t remaining =
                            msUntil(
                                now,
                                entry.nextAttemptMs);
                        if (!haveDeadline ||
                            remaining < shortest) {
                            shortest = remaining;
                            haveDeadline = true;
                        }
                    }

                    if (haveDeadline) {
                        waitMs =
                            max<uint32_t>(
                                shortest,
                                100UL);
                    }
                }
            }

            xSemaphoreGive(_mutex);
        }

        if (_stopRequested) break;

        if (!haveTarget) {
            ulTaskNotifyTake(
                pdTRUE,
                pdMS_TO_TICKS(waitMs));
            continue;
        }

        if (!WiFi.isConnected()) {
            ulTaskNotifyTake(
                pdTRUE,
                pdMS_TO_TICKS(OfflineRetryMs));
            continue;
        }

        // Ověření TLS certifikátu vyžaduje validní systémový čas.
        // Po startu proto neposíláme HTTPS request dřív, než doběhne NTP.
        if (!tlsClockReady()) {
            if (!waitingForClockLogged) {
                Serial.println("[WEATHER] Cekam na validni NTP cas pred HTTPS...");
                waitingForClockLogged = true;
            }
            ulTaskNotifyTake(
                pdTRUE,
                pdMS_TO_TICKS(1000));
            continue;
        }

        if (waitingForClockLogged) {
            Serial.println("[WEATHER] NTP cas je validni, HTTPS muze pokracovat.");
            waitingForClockLogged = false;
        }

        WeatherConfig targetConfig = config;
        targetConfig.activeLocationId = target.id;
        targetConfig.latitude = target.latitude;
        targetConfig.longitude = target.longitude;

        WeatherData working;
        working.enabled = true;
        working.locationName = target.name;
        working.provider =
            providerLabel(config.provider);

        bool success = false;
        if (provider == nullptr) {
            working.status.recordError(
                "Unsupported provider");
        } else {
            if (!takeMemoryHeavyGate()) break;

            Serial.printf(
                "[WEATHER] Fetch %s: %s (%.5f, %.5f)%s | "
                "free=%u, maxBlock=%u\n",
                config.provider.c_str(),
                target.name.c_str(),
                target.latitude,
                target.longitude,
                targetIsActive
                    ? " [ACTIVE]"
                    : " [BACKGROUND]",
                ESP.getFreeHeap(),
                ESP.getMaxAllocHeap());

            success =
                provider->update(
                    targetConfig,
                    working);

            if (_memoryHeavyGate != nullptr) {
                xSemaphoreGive(_memoryHeavyGate);
            }
        }

        // Pokud prisel stop behem HTTP/TLS operace, nedotykame se uz cache.
        // Task se uklidi a ukonci sam, aby nebyl zrusen uprostred knihovny.
        if (_stopRequested) break;

        uint32_t validForSeconds =
            config.pollIntervalSeconds;
        if (success && provider != nullptr) {
            validForSeconds =
                provider->recommendedPollIntervalSeconds(
                    config.pollIntervalSeconds);
        }

        storeFetchResult(
            config.provider,
            providerGeneration,
            target,
            working,
            success,
            validForSeconds);

        Serial.printf(
            "[WEATHER] Worker stack watermark=%u | free=%u, maxBlock=%u\n",
            static_cast<unsigned>(
                uxTaskGetStackHighWaterMark(nullptr)),
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());

        ulTaskNotifyTake(
            pdTRUE,
            pdMS_TO_TICKS(BackgroundFetchGapMs));
    }

    cleanupTaskResources();
    vTaskDelete(nullptr);
}

void WeatherWorker::cleanupTaskResources() {
    SemaphoreHandle_t mutex = _mutex;

    if (mutex != nullptr &&
        xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        clearCacheLocked();
        _latest = WeatherData();
        _hasLatest = false;
        _provider = nullptr;
        _cacheProvider = "";
        _configGeneration = 0;
        _providerGeneration = 0;
        _priorityLocationId = "";
        xSemaphoreGive(mutex);
    }

    _mutex = nullptr;
    if (mutex != nullptr) {
        vSemaphoreDelete(mutex);
    }

    _stopRequested = false;

    // _task nulujeme az po kompletnim uklidu. stop() se tak vrati teprve
    // ve chvili, kdy uz hlavni task muze bezpecne pokracovat.
    _task = nullptr;

    Serial.printf(
        "[WEATHER] Task ukoncen a runtime prostredky uvolneny. "
        "free=%u, maxBlock=%u\n",
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap());
}
