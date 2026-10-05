#include "RfSensorManager.h"

#include <ArduinoJson.h>
#include <new>
#include <math.h>
#include <time.h>

namespace {
constexpr uint32_t SensorOfflineAfterMs = 5UL * 60UL * 1000UL;

bool elapsedAtLeast(uint32_t now, uint32_t started, uint32_t duration) {
    return static_cast<uint32_t>(now - started) >= duration;
}
}

RfSensorManager::RfSensorManager(DataModel& dataModel)
    : _dataModel(dataModel) {
}

int RfSensorManager::slotIndex(const String& slotId) {
    if (!slotId.startsWith("sensor")) return -1;
    const int value = slotId.substring(6).toInt();
    if (value < 1 || value > MaxRfSensors) return -1;
    if (slotId != "sensor" + String(value)) return -1;
    return value - 1;
}

bool RfSensorManager::ensureHistoryTable() {
    if (_history != nullptr) return true;

    _history = new (std::nothrow) RfSensorHistory*[MaxRfSensors]();
    if (_history == nullptr) {
        Serial.println(
            "[RF-SENSORS] Tabulku historii nelze alokovat: "
            "nedostatek heap pameti.");
        return false;
    }

    _dataModel.rfSensors.history = _history;
    return true;
}

RfSensorHistory* RfSensorManager::ensureHistory(uint8_t stableIndex) {
    if (stableIndex >= MaxRfSensors || !ensureHistoryTable()) return nullptr;

    if (_history[stableIndex] == nullptr) {
        _history[stableIndex] = new (std::nothrow) RfSensorHistory();
        if (_history[stableIndex] == nullptr) {
            Serial.printf(
                "[RF-SENSORS] Historie slotu %u nelze alokovat: "
                "nedostatek heap pameti.\n",
                static_cast<unsigned>(stableIndex + 1));
            return nullptr;
        }
        Serial.printf(
            "[RF-SENSORS] Historie slotu %u alokovana (%u B).\n",
            static_cast<unsigned>(stableIndex + 1),
            static_cast<unsigned>(sizeof(RfSensorHistory)));
    }

    _dataModel.rfSensors.history = _history;
    return _history[stableIndex];
}

void RfSensorManager::sampleHistory(
    uint8_t configuredIndex,
    const RfSensorObservation& observation,
    uint32_t now) {

    if (_config == nullptr ||
        configuredIndex >= _config->sensorCount ||
        configuredIndex >= MaxRfSensors) {
        return;
    }

    const int stableIndex = slotIndex(_config->sensors[configuredIndex].slotId);
    if (stableIndex < 0) return;

    RfSensorHistory* historyPtr =
        ensureHistory(static_cast<uint8_t>(stableIndex));
    if (historyPtr == nullptr) return;

    RfHistorySample sample;
    if (observation.hasTemperature) {
        sample.temperatureCenti =
            static_cast<int16_t>(lroundf(observation.temperatureC * 100.0f));
        sample.flags |= 0x01;
    }
    if (observation.hasHumidity) {
        int humidity = observation.humidityPercent;
        if (humidity < 0) humidity = 0;
        if (humidity > 100) humidity = 100;
        sample.humidityPercent = static_cast<uint8_t>(humidity);
        sample.flags |= 0x02;
    }
    if (sample.flags == 0) return;

    const time_t epoch = time(nullptr);
    const bool wallClock = epoch > 1700000000;
    const uint32_t timeSeconds =
        wallClock ? static_cast<uint32_t>(epoch) : now / 1000UL;

    RfSensorHistory& history = *historyPtr;
    for (uint8_t periodIndex = 0;
         periodIndex < SensorGraphPeriodCount;
         ++periodIndex) {

        const uint8_t hours = SensorGraphPeriodHours[periodIndex];
        const uint32_t bucketSeconds = sensorGraphBucketSeconds(hours);
        if (bucketSeconds == 0) continue;

        const uint32_t bucket = timeSeconds / bucketSeconds;
        RfHistorySeries& series = history.series[periodIndex];

        auto appendSample = [&series](const RfHistorySample& value) {
            series.samples[series.next] = value;
            series.next = static_cast<uint8_t>(
                (series.next + 1) % SensorGraphSampleCount);
            if (series.count < SensorGraphSampleCount) ++series.count;
        };

        if (series.count == 0 || series.wallClock != wallClock) {
            series = RfHistorySeries{};
            series.wallClock = wallClock;
            series.lastBucket = bucket;
            appendSample(sample);
            continue;
        }

        if (bucket == series.lastBucket) {
            const uint8_t latest =
                static_cast<uint8_t>(
                    (series.next + SensorGraphSampleCount - 1) %
                    SensorGraphSampleCount);
            series.samples[latest] = sample;
            continue;
        }

        if (bucket < series.lastBucket ||
            bucket - series.lastBucket >= SensorGraphSampleCount) {
            series = RfHistorySeries{};
            series.wallClock = wallClock;
            series.lastBucket = bucket;
            appendSample(sample);
            continue;
        }

        const uint32_t gap = bucket - series.lastBucket;
        for (uint32_t step = 1; step < gap; ++step) {
            appendSample(RfHistorySample{});
        }
        appendSample(sample);
        series.lastBucket = bucket;
    }
}

String RfSensorManager::normalizedName(String name) {
    name.trim();
    if (name.length() > 40) name.remove(40);
    for (size_t i = 0; i < name.length(); ++i) {
        if (static_cast<uint8_t>(name[i]) < 0x20) name.setCharAt(i, ' ');
    }
    name.trim();
    return name;
}

String RfSensorManager::protocolLabel(const String& protocol) {
    if (protocol == "tfa-twin") return "TFA Twin Plus / KW9010";
    if (protocol == "auriol") return "Auriol";
    if (protocol == "nexus-th") return "Nexus-TH";
    if (protocol == "ft017th") return "FT017TH";
    if (protocol == "hyundai-r50") return "Hyundai R50";
    return protocol;
}

String RfSensorManager::defaultSensorLabel(const RfSensorConfig& sensor) {
    String label = protocolLabel(sensor.protocol);
    label += " 0x";
    label += String(sensor.sensorId, HEX);
    if (sensor.channel > 0) {
        label += " CH";
        label += String(sensor.channel);
    }
    return label;
}

bool RfSensorManager::sameBinding(
    const RfSensorConfig& sensor,
    const RfSensorObservation& observation) {
    return sensor.protocol == observation.protocol &&
           sensor.sensorId == observation.sensorId &&
           sensor.channel == observation.channel;
}

void RfSensorManager::applyConfig(const RfSensorsConfig& config) {
    ensureHistoryTable();
    _dataModel.rfSensors.history = _history;

    RfSensorData previous[MaxRfSensors];
    const uint8_t previousCount = _dataModel.rfSensors.sensorCount;
    for (uint8_t i = 0; i < previousCount && i < MaxRfSensors; ++i) {
        previous[i] = _dataModel.rfSensors.sensors[i];
    }

    _config = &config;
    const uint8_t configuredCount =
        config.sensorCount > MaxRfSensors ? MaxRfSensors : config.sensorCount;

    _dataModel.rfSensors.sensorCount = configuredCount;
    for (uint8_t i = 0; i < MaxRfSensors; ++i) {
        RfSensorData next;
        if (i < configuredCount) {
            const RfSensorConfig& sensor = _config->sensors[i];
            next.configured = true;
            next.slotId = sensor.slotId;
            next.hasTemperature = sensor.hasTemperature;
            next.hasHumidity = sensor.hasHumidity;
            next.hasBattery = sensor.hasBattery;

            for (uint8_t p = 0; p < previousCount; ++p) {
                if (previous[p].slotId != sensor.slotId) continue;
                next.available = previous[p].available;
                next.temperatureC = previous[p].temperatureC;
                next.humidityPercent = previous[p].humidityPercent;
                next.batteryOk = previous[p].batteryOk;
                next.lastUpdateMs = previous[p].lastUpdateMs;
                break;
            }
        }
        _dataModel.rfSensors.sensors[i] = next;
    }
}

int RfSensorManager::configuredIndexFor(
    const RfSensorObservation& observation) const {
    if (_config == nullptr) return -1;
    for (uint8_t i = 0; i < _config->sensorCount && i < MaxRfSensors; ++i) {
        if (sameBinding(_config->sensors[i], observation)) return i;
    }
    return -1;
}

int RfSensorManager::discoveredIndexFor(const String& bindingKey) const {
    if (_discovered == nullptr) return -1;
    for (uint8_t i = 0; i < MaxRfDiscoveredSensors; ++i) {
        if (!_discovered[i].used) continue;
        if (_discovered[i].observation.bindingKey() == bindingKey) return i;
    }
    return -1;
}

int RfSensorManager::freeDiscoveredIndex() const {
    if (_discovered == nullptr) return -1;
    int oldestIndex = -1;
    uint32_t oldestSeen = 0;
    for (uint8_t i = 0; i < MaxRfDiscoveredSensors; ++i) {
        if (!_discovered[i].used) return i;
        if (oldestIndex < 0 ||
            static_cast<int32_t>(_discovered[i].lastSeenMs - oldestSeen) < 0) {
            oldestIndex = i;
            oldestSeen = _discovered[i].lastSeenMs;
        }
    }
    return oldestIndex;
}

bool RfSensorManager::observe(const RfSensorObservation& observation) {
    if (observation.protocol.isEmpty()) return false;

    const uint32_t now = millis();

    if (isScanning()) {
        const String key = observation.bindingKey();
        int index = discoveredIndexFor(key);
        if (index < 0) index = freeDiscoveredIndex();
        if (index >= 0) {
            DiscoveredSensor& item = _discovered[index];
            if (!item.used || item.observation.bindingKey() != key) {
                item = DiscoveredSensor{};
                item.used = true;
                item.firstSeenMs = now;
            }
            item.observation = observation;
            item.lastSeenMs = now;
            ++item.packetCount;
        }
    }

    const int configuredIndex = configuredIndexFor(observation);
    if (configuredIndex < 0) return false;

    RfSensorData& target = _dataModel.rfSensors.sensors[configuredIndex];
    target.available = true;
    target.lastUpdateMs = now;

    if (observation.hasTemperature) {
        target.hasTemperature = true;
        target.temperatureC = observation.temperatureC;
    }
    if (observation.hasHumidity) {
        target.hasHumidity = true;
        target.humidityPercent = observation.humidityPercent;
    }
    if (observation.hasBattery) {
        target.hasBattery = true;
        target.batteryOk = observation.batteryOk;
    }

    sampleHistory(static_cast<uint8_t>(configuredIndex), observation, now);
    return true;
}

void RfSensorManager::loop() {
    const uint32_t now = millis();
    for (uint8_t i = 0;
         i < _dataModel.rfSensors.sensorCount && i < MaxRfSensors;
         ++i) {
        RfSensorData& sensor = _dataModel.rfSensors.sensors[i];
        if (!sensor.available || sensor.lastUpdateMs == 0) continue;
        if (elapsedAtLeast(now, sensor.lastUpdateMs, SensorOfflineAfterMs)) {
            sensor.available = false;
        }
    }
}

void RfSensorManager::startScan(uint32_t durationMs) {
    if (durationMs < 30000) durationMs = 30000;
    if (durationMs > 180000) durationMs = 180000;

    if (_discovered == nullptr) {
        _discovered =
            new (std::nothrow) DiscoveredSensor[MaxRfDiscoveredSensors];
        if (_discovered == nullptr) {
            _scanStartedMs = 0;
            _scanDurationMs = 0;
            Serial.println(
                "[RF-SENSORS] Scan nelze spustit: nedostatek heap pameti.");
            return;
        }
    }

    for (uint8_t i = 0; i < MaxRfDiscoveredSensors; ++i) {
        _discovered[i] = DiscoveredSensor{};
    }
    _scanStartedMs = millis();
    _scanDurationMs = durationMs;
}

bool RfSensorManager::isScanning() const {
    if (_scanDurationMs == 0) return false;
    return !elapsedAtLeast(millis(), _scanStartedMs, _scanDurationMs);
}

uint32_t RfSensorManager::scanRemainingMs() const {
    if (!isScanning()) return 0;
    const uint32_t elapsed = static_cast<uint32_t>(millis() - _scanStartedMs);
    return elapsed >= _scanDurationMs ? 0 : _scanDurationMs - elapsed;
}

String RfSensorManager::nextSlotId(const RfSensorsConfig& config) const {
    for (uint8_t sequence = 1; sequence <= MaxRfSensors; ++sequence) {
        const String candidate = "sensor" + String(sequence);
        bool used = false;
        for (uint8_t i = 0; i < config.sensorCount && i < MaxRfSensors; ++i) {
            if (config.sensors[i].slotId == candidate) {
                used = true;
                break;
            }
        }
        if (!used) return candidate;
    }
    return "";
}

bool RfSensorManager::addDiscoveredSensor(
    const String& bindingKey,
    const String& requestedName,
    RfSensorsConfig& updated,
    String& error) const {

    const int discoveredIndex = discoveredIndexFor(bindingKey);
    if (discoveredIndex < 0) {
        error = "Čidlo už není v aktuálním výsledku scanu.";
        return false;
    }

    const RfSensorObservation& observation =
        _discovered[discoveredIndex].observation;

    if (_config == nullptr) {
        error = "Správa RF čidel ještě není inicializovaná.";
        return false;
    }

    for (uint8_t i = 0; i < _config->sensorCount && i < MaxRfSensors; ++i) {
        if (sameBinding(_config->sensors[i], observation)) {
            error = "Toto čidlo už je uložené.";
            return false;
        }
    }

    if (_config->sensorCount >= MaxRfSensors) {
        error = "Je dosažen maximální počet uložených čidel.";
        return false;
    }

    updated = *_config;
    const String slotId = nextSlotId(updated);
    if (slotId.isEmpty()) {
        error = "Nelze vytvořit stabilní identifikátor čidla.";
        return false;
    }

    RfSensorConfig& sensor = updated.sensors[updated.sensorCount++];
    sensor.slotId = slotId;
    sensor.protocol = observation.protocol;
    sensor.sensorId = observation.sensorId;
    sensor.channel = observation.channel;
    sensor.name = normalizedName(requestedName);
    sensor.hasTemperature = observation.hasTemperature;
    sensor.hasHumidity = observation.hasHumidity;
    sensor.hasBattery = observation.hasBattery;
    return true;
}

bool RfSensorManager::renameSensor(
    const String& slotId,
    const String& requestedName,
    RfSensorsConfig& updated,
    String& error) const {

    if (_config == nullptr) {
        error = "Správa RF čidel ještě není inicializovaná.";
        return false;
    }
    updated = *_config;

    if (slotId == "bme280") {
        updated.bme280Name = normalizedName(requestedName);
        if (updated.bme280Name.isEmpty()) updated.bme280Name = "BME280";
        return true;
    }

    for (uint8_t i = 0; i < updated.sensorCount && i < MaxRfSensors; ++i) {
        if (updated.sensors[i].slotId != slotId) continue;
        updated.sensors[i].name = normalizedName(requestedName);
        return true;
    }

    error = "Uložené čidlo nebylo nalezeno.";
    return false;
}

bool RfSensorManager::rebindSensor(
    const String& slotId,
    const String& bindingKey,
    RfSensorsConfig& updated,
    String& error) const {

    if (_config == nullptr) {
        error = "Správa RF čidel ještě není inicializovaná.";
        return false;
    }

    const int discoveredIndex = discoveredIndexFor(bindingKey);
    if (discoveredIndex < 0) {
        error = "Čidlo už není v aktuálním výsledku scanu.";
        return false;
    }

    const RfSensorObservation& observation =
        _discovered[discoveredIndex].observation;

    updated = *_config;
    int targetIndex = -1;
    for (uint8_t i = 0; i < updated.sensorCount && i < MaxRfSensors; ++i) {
        if (updated.sensors[i].slotId == slotId) {
            targetIndex = i;
            continue;
        }
        if (sameBinding(updated.sensors[i], observation)) {
            error = "Toto RF čidlo už je přiřazené k jinému uloženému čidlu.";
            return false;
        }
    }
    if (targetIndex < 0) {
        error = "Uložené čidlo nebylo nalezeno.";
        return false;
    }

    RfSensorConfig& target = updated.sensors[targetIndex];
    target.protocol = observation.protocol;
    target.sensorId = observation.sensorId;
    target.channel = observation.channel;
    target.hasTemperature = observation.hasTemperature;
    target.hasHumidity = observation.hasHumidity;
    target.hasBattery = observation.hasBattery;
    // slotId and user-visible name intentionally stay unchanged so all
    // rf.<slotId> KPI/layout references remain stable.
    return true;
}

bool RfSensorManager::removeSensor(
    const String& slotId,
    RfSensorsConfig& updated,
    String& error) const {

    if (_config == nullptr) {
        error = "Správa RF čidel ještě není inicializovaná.";
        return false;
    }
    updated = *_config;
    for (uint8_t i = 0; i < updated.sensorCount && i < MaxRfSensors; ++i) {
        if (updated.sensors[i].slotId != slotId) continue;
        for (uint8_t j = i + 1; j < updated.sensorCount; ++j) {
            updated.sensors[j - 1] = updated.sensors[j];
        }
        if (updated.sensorCount > 0) {
            --updated.sensorCount;
            updated.sensors[updated.sensorCount] = RfSensorConfig{};
        }
        return true;
    }

    error = "Uložené čidlo nebylo nalezeno.";
    return false;
}

String RfSensorManager::statusJson() const {
    JsonDocument doc;
    doc["scanning"] = isScanning();
    doc["remainingMs"] = scanRemainingMs();
    doc["maxSensors"] = MaxRfSensors;

    JsonArray configured = doc["configured"].to<JsonArray>();
    const uint32_t now = millis();

    // Fixed local BME280 is presented in the same saved-sensors list as
    // user-paired 433 MHz sensors. It is not an RF binding and therefore
    // cannot be removed/rebound through RF management actions.
    {
        const InsideData& inside = _dataModel.inside;
        JsonObject item = configured.add<JsonObject>();
        item["kind"] = "local";
        item["slotId"] = "bme280";
        const String bmeName =
            (_config != nullptr && !_config->bme280Name.isEmpty())
                ? _config->bme280Name
                : String("BME280");
        item["name"] = bmeName;
        item["displayName"] = bmeName;
        item["protocol"] = "bme280";
        item["protocolLabel"] = "Lokální I2C";
        item["available"] = inside.status.available;
        item["removable"] = false;
        item["renameable"] = true;
        item["lastSeenAgeSeconds"] =
            inside.lastUpdateMs == 0
                ? -1
                : static_cast<long>(
                      static_cast<uint32_t>(now - inside.lastUpdateMs) / 1000UL);

        JsonObject capabilities = item["capabilities"].to<JsonObject>();
        capabilities["temperature"] = true;
        capabilities["humidity"] = true;
        capabilities["pressure"] = true;
        capabilities["battery"] = false;

        if (inside.status.available && inside.lastUpdateMs > 0) {
            item["temperatureC"] = inside.temperatureC;
            item["humidityPercent"] = inside.humidityPercent;
            item["pressureHpa"] = inside.pressureHpa;
        }

        JsonObject sources = item["sources"].to<JsonObject>();
        sources["temperature"] = "inside.temperatureC";
        sources["humidity"] = "inside.humidityPercent";
        sources["pressure"] = "inside.pressureHpa";
    }
    const uint8_t configuredCount =
        _config == nullptr ? 0 :
        (_config->sensorCount > MaxRfSensors ? MaxRfSensors : _config->sensorCount);
    for (uint8_t i = 0; i < configuredCount; ++i) {
        const RfSensorConfig& cfg = _config->sensors[i];
        const RfSensorData& data = _dataModel.rfSensors.sensors[i];

        JsonObject item = configured.add<JsonObject>();
        item["kind"] = "rf";
        item["removable"] = true;
        item["renameable"] = true;
        item["slotId"] = cfg.slotId;
        item["name"] = cfg.name;
        item["displayName"] = cfg.name.isEmpty() ? defaultSensorLabel(cfg) : cfg.name;
        item["protocol"] = cfg.protocol;
        item["protocolLabel"] = protocolLabel(cfg.protocol);
        item["sensorId"] = cfg.sensorId;
        item["sensorIdHex"] = String(cfg.sensorId, HEX);
        item["channel"] = cfg.channel;
        item["bindingKey"] =
            cfg.protocol + ":" + String(cfg.sensorId, HEX) + ":" + String(cfg.channel);
        item["available"] = data.available;
        item["lastSeenAgeSeconds"] =
            data.lastUpdateMs == 0
                ? -1
                : static_cast<long>(
                      static_cast<uint32_t>(now - data.lastUpdateMs) / 1000UL);

        JsonObject capabilities = item["capabilities"].to<JsonObject>();
        capabilities["temperature"] = cfg.hasTemperature;
        capabilities["humidity"] = cfg.hasHumidity;
        capabilities["battery"] = cfg.hasBattery;

        if (data.lastUpdateMs > 0) {
            if (data.hasTemperature) item["temperatureC"] = data.temperatureC;
            if (data.hasHumidity) item["humidityPercent"] = data.humidityPercent;
            if (data.hasBattery) item["batteryOk"] = data.batteryOk;
        }

        JsonObject sources = item["sources"].to<JsonObject>();
        if (cfg.hasTemperature) {
            sources["temperature"] = "rf." + cfg.slotId + ".temperatureC";
        }
        if (cfg.hasHumidity) {
            sources["humidity"] = "rf." + cfg.slotId + ".humidityPercent";
        }
    }

    JsonArray discovered = doc["discovered"].to<JsonArray>();
    for (uint8_t discoveredIndex = 0;
         _discovered != nullptr && discoveredIndex < MaxRfDiscoveredSensors;
         ++discoveredIndex) {
        const DiscoveredSensor& found = _discovered[discoveredIndex];
        if (!found.used) continue;
        const RfSensorObservation& observation = found.observation;
        JsonObject item = discovered.add<JsonObject>();
        item["bindingKey"] = observation.bindingKey();
        item["protocol"] = observation.protocol;
        item["protocolLabel"] = protocolLabel(observation.protocol);
        item["sensorId"] = observation.sensorId;
        item["sensorIdHex"] = String(observation.sensorId, HEX);
        item["channel"] = observation.channel;
        item["packets"] = found.packetCount;
        item["lastSeenAgeSeconds"] =
            static_cast<uint32_t>(now - found.lastSeenMs) / 1000UL;

        bool saved = false;
        String slotId;
        if (_config != nullptr) for (uint8_t i = 0; i < _config->sensorCount && i < MaxRfSensors; ++i) {
            if (!sameBinding(_config->sensors[i], observation)) continue;
            saved = true;
            slotId = _config->sensors[i].slotId;
            break;
        }
        item["saved"] = saved;
        if (saved) item["slotId"] = slotId;

        if (observation.hasTemperature) {
            item["temperatureC"] = observation.temperatureC;
        }
        if (observation.hasHumidity) {
            item["humidityPercent"] = observation.humidityPercent;
        }
        if (observation.hasBattery) {
            item["batteryOk"] = observation.batteryOk;
        }

        JsonObject capabilities = item["capabilities"].to<JsonObject>();
        capabilities["temperature"] = observation.hasTemperature;
        capabilities["humidity"] = observation.hasHumidity;
        capabilities["battery"] = observation.hasBattery;
    }

    String response;
    serializeJson(doc, response);
    return response;
}
