#include "WeatherWorker.h"
#include <WiFi.h>
#include <math.h>

namespace {
constexpr uint32_t MinimumRetrySeconds = 60;
constexpr uint32_t MaximumRetrySeconds = 3600;
constexpr uint8_t MaximumFailureShift = 3;
constexpr uint32_t OfflineRetryMs = 10000;

uint32_t nextDelaySeconds(uint32_t configuredSeconds, uint8_t failureStreak, bool success) {
    if (success) return configuredSeconds;

    uint64_t delaySeconds = min(configuredSeconds, MinimumRetrySeconds);
    delaySeconds = max(delaySeconds, static_cast<uint64_t>(MinimumRetrySeconds));
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

bool WeatherWorker::begin(const WeatherConfig& config) {
    if (_task != nullptr) return reconfigure(config);

    _config = config;
    _provider = providerFor(_config.provider);
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

    Serial.printf("[WEATHER] Cache pripravena: provider=%s, lokalit=%u\n",
                  _config.provider.c_str(),
                  static_cast<unsigned>(_config.locationCount));
    return true;
}

bool WeatherWorker::reconfigure(const WeatherConfig& config) {
    if (_mutex == nullptr || _task == nullptr) return begin(config);

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        Serial.println("[WEATHER] Reconfiguration lock timeout.");
        return false;
    }

    const bool providerChanged = _config.provider != config.provider;
    const bool activeChanged = _config.activeLocationId != config.activeLocationId;
    const bool enabledChanged = _config.enabled != config.enabled;

    _config = config;
    _provider = providerFor(_config.provider);
    ++_configGeneration;
    if (providerChanged) ++_providerGeneration;

    reconcileCacheLocked(_config, providerChanged);

    if (_config.enabled) {
        publishCachedActiveLocked(_config);
    } else {
        _latest = WeatherData();
        _hasLatest = false;
    }

    uint8_t validCount = 0;
    for (const auto& entry : _cache) {
        if (entry.used && entry.valid) ++validCount;
    }

    const uint32_t generation = _configGeneration;
    const uint32_t providerGeneration = _providerGeneration;
    xSemaphoreGive(_mutex);

    Serial.printf(
        "[WEATHER] Konfigurace za behu: gen=%lu providerGen=%lu, %s, active=%s, "
        "interval=%lu s, enabled=%d, cache=%u/%u%s%s%s\n",
        static_cast<unsigned long>(generation),
        static_cast<unsigned long>(providerGeneration),
        _config.provider.c_str(),
        _config.activeLocationId.c_str(),
        static_cast<unsigned long>(_config.pollIntervalSeconds),
        _config.enabled,
        validCount,
        static_cast<unsigned>(_config.locationCount),
        providerChanged ? " provider-change" : "",
        activeChanged ? " active-change" : "",
        enabledChanged ? " enabled-change" : "");

    // Přerušit případné čekání. Probíhající HTTPS request doběhne, ale jeho
    // výsledek se uloží jen pokud stále patří aktuálnímu provideru a lokalitě.
    xTaskNotifyGive(_task);
    return true;
}

bool WeatherWorker::takeLatest(WeatherData& weatherData) {
    if (_mutex == nullptr || xSemaphoreTake(_mutex, 0) != pdTRUE) return false;

    const bool hasLatest = _hasLatest;
    if (hasLatest) {
        weatherData = _latest;
        _hasLatest = false;
    }
    xSemaphoreGive(_mutex);
    return hasLatest;
}

void WeatherWorker::clearCacheLocked() {
    for (auto& entry : _cache) entry = CacheEntry();
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

    // Odstranit cache lokalit, které už v konfiguraci nejsou nebo změnily
    // souřadnice.
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

        if (!found) entry = CacheEntry();
    }

    // Každé uložené místo musí mít svůj cache slot. Pořadí slotů není důležité,
    // takže existující WeatherData nemusíme při změně pořadí kopírovat.
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
        entry = CacheEntry();
        entry.used = true;
        entry.locationId = location.id;
        entry.latitude = location.latitude;
        entry.longitude = location.longitude;
        entry.nextAttemptMs = 0; // nová lokalita má prioritu k načtení
    }

    // Delší nově nastavený interval respektujeme ihned. Zkrácení intervalu se
    // projeví po nejbližším refreshi; u MET tím zároveň neporušíme serverové
    // cache minimum získané z Expires.
    const uint32_t now = millis();
    for (auto& entry : _cache) {
        if (!entry.used || !entry.valid) continue;
        if (config.pollIntervalSeconds > entry.validForSeconds) {
            entry.validForSeconds = config.pollIntervalSeconds;
            entry.nextAttemptMs =
                entry.fetchedAtMs + entry.validForSeconds * 1000UL;
            if (dueNow(now, entry.nextAttemptMs)) entry.nextAttemptMs = now;
        }
    }
}

void WeatherWorker::publishCachedActiveLocked(const WeatherConfig& config) {
    if (!config.enabled) return;

    const WeatherLocation* active = config.activeLocation();
    if (active == nullptr) return;

    const int index = findCacheEntryLocked(
        active->id,
        active->latitude,
        active->longitude);
    if (index < 0 || !_cache[index].valid) {
        _latest = WeatherData();
        _hasLatest = false;
        Serial.printf("[WEATHER] Cache MISS aktivni lokalita: %s\n",
                      active->name.c_str());
        return;
    }

    WeatherData cached = _cache[index].data;
    cached.enabled = true;
    cached.locationName = active->name;
    _latest = cached;
    _hasLatest = true;

    const uint32_t ageSeconds =
        (millis() - _cache[index].fetchedAtMs) / 1000UL;
    Serial.printf("[WEATHER] Cache HIT aktivni lokalita: %s, stari %lu s\n",
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
    const bool isActive = _config.activeLocationId == location.id;

    if (success) {
        entry.data = weatherData;
        entry.data.enabled = _config.enabled;
        entry.data.locationName = location.name;
        entry.valid = true;
        entry.fetchedAtMs = now;
        entry.validForSeconds = max<uint32_t>(
            max<uint32_t>(validForSeconds, _config.pollIntervalSeconds),
            MinimumRetrySeconds);
        entry.nextAttemptMs =
            now + entry.validForSeconds * 1000UL;
        entry.failureStreak = 0;

        if (isActive) {
            _latest = entry.data;
            _hasLatest = true;
        }

        Serial.printf(
            "[WEATHER] Cache STORE: %s | %s | TTL %lu s%s\n",
            providerName.c_str(),
            location.name.c_str(),
            static_cast<unsigned long>(entry.validForSeconds),
            isActive ? " | ACTIVE" : "");
    } else {
        const uint8_t shift = min(entry.failureStreak, MaximumFailureShift);
        const uint32_t retrySeconds =
            nextDelaySeconds(_config.pollIntervalSeconds, shift, false);
        entry.failureStreak =
            min<uint8_t>(entry.failureStreak + 1, MaximumFailureShift);
        entry.nextAttemptMs = now + retrySeconds * 1000UL;

        if (isActive && !entry.valid) {
            WeatherData failed = weatherData;
            failed.enabled = _config.enabled;
            failed.locationName = location.name;
            _latest = failed;
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
    for (;;) {
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
                const WeatherLocation* active = config.activeLocation();

                // 1) Aktivní lokalita má absolutní prioritu, pokud ještě nemá
                // data nebo už je její cache po termínu obnovy.
                if (active != nullptr) {
                    const int activeIndex = findCacheEntryLocked(
                        active->id,
                        active->latitude,
                        active->longitude);
                    if (activeIndex >= 0) {
                        const auto& entry = _cache[activeIndex];
                        if (!entry.valid || dueNow(now, entry.nextAttemptMs)) {
                            target = *active;
                            haveTarget = true;
                            targetIsActive = true;
                        }
                    }
                }

                // 2) Potom postupně doplnit ostatní chybějící / prošlé cache.
                if (!haveTarget) {
                    for (uint8_t i = 0;
                         i < config.locationCount && i < MaxWeatherLocations;
                         ++i) {
                        const auto& location = config.locations[i];
                        const int index = findCacheEntryLocked(
                            location.id,
                            location.latitude,
                            location.longitude);
                        if (index < 0) continue;

                        const auto& entry = _cache[index];
                        if ((!entry.valid || dueNow(now, entry.nextAttemptMs)) &&
                            location.id != config.activeLocationId) {
                            target = location;
                            haveTarget = true;
                            break;
                        }
                    }
                }

                // 3) Není-li co načítat, spát přesně do nejbližší expirace.
                if (!haveTarget) {
                    bool haveDeadline = false;
                    uint32_t shortest = 0xFFFFFFFFUL;
                    for (const auto& entry : _cache) {
                        if (!entry.used) continue;
                        const uint32_t remaining =
                            msUntil(now, entry.nextAttemptMs);
                        if (!haveDeadline || remaining < shortest) {
                            shortest = remaining;
                            haveDeadline = true;
                        }
                    }
                    if (haveDeadline) waitMs = max<uint32_t>(shortest, 100UL);
                }
            }

            xSemaphoreGive(_mutex);
        }

        if (!config.enabled) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        if (!haveTarget) {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitMs));
            continue;
        }

        if (!WiFi.isConnected()) {
            // Stav ztráty Wi-Fi publikuje hlavní app loop; worker pouze čeká
            // na návrat sítě nebo změnu konfigurace.
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(OfflineRetryMs));
            continue;
        }

        WeatherConfig targetConfig = config;
        targetConfig.activeLocationId = target.id;
        targetConfig.latitude = target.latitude;
        targetConfig.longitude = target.longitude;

        WeatherData working;
        working.enabled = true;
        working.locationName = target.name;
        working.provider = providerLabel(config.provider);

        bool success = false;
        if (provider == nullptr) {
            working.status.recordError("Unsupported provider");
        } else {
            Serial.printf(
                "[WEATHER] Fetch %s: %s (%.5f, %.5f)%s\n",
                config.provider.c_str(),
                target.name.c_str(),
                target.latitude,
                target.longitude,
                targetIsActive ? " [ACTIVE]" : " [BACKGROUND]");
            success = provider->update(targetConfig, working);
        }

        uint32_t validForSeconds = config.pollIntervalSeconds;
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

        // Neposílat dávku osmi HTTP požadavků bez rozestupu. Změna konfigurace
        // toto čekání okamžitě přeruší notifikací.
        ulTaskNotifyTake(
            pdTRUE,
            pdMS_TO_TICKS(BackgroundFetchGapMs));
    }
}
