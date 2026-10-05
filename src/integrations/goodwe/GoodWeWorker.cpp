#include "GoodWeWorker.h"

bool GoodWeWorker::begin(const GoodWeConfig& config) {
    if (_task != nullptr) return reconfigure(config);

    _config = config;
    _configGeneration = 1;
    _pollRequested = false;
    _pollInFlight = false;
    _hasLatest = false;

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[GOODWE-WORKER] Nelze vytvorit mutex.");
        return false;
    }

    const BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "goodweTask",
        TaskStackBytes,
        this,
        TaskPriority,
        &_task,
        ARDUINO_RUNNING_CORE);

    if (result != pdPASS) {
        Serial.println("[GOODWE-WORKER] Nelze vytvorit task.");
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
        _task = nullptr;
        return false;
    }

    Serial.printf(
        "[GOODWE-WORKER] Spusten. enabled=%d host=%s:%u\n",
        _config.enabled ? 1 : 0,
        _config.host.c_str(),
        _config.port);
    return true;
}

bool GoodWeWorker::reconfigure(const GoodWeConfig& config) {
    if (_task == nullptr || _mutex == nullptr) return begin(config);

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        Serial.println("[GOODWE-WORKER] Reconfiguration lock timeout.");
        return false;
    }

    _config = config;
    ++_configGeneration;
    _pollRequested = false;
    _hasLatest = false;
    xSemaphoreGive(_mutex);

    xTaskNotifyGive(_task);
    Serial.printf(
        "[GOODWE-WORKER] Konfigurace gen=%lu enabled=%d host=%s:%u\n",
        static_cast<unsigned long>(_configGeneration),
        config.enabled ? 1 : 0,
        config.host.c_str(),
        config.port);
    return true;
}

bool GoodWeWorker::requestPoll() {
    if (_task == nullptr || _mutex == nullptr) return false;
    if (xSemaphoreTake(_mutex, 0) != pdTRUE) return false;

    const bool canQueue =
        _config.enabled && !_pollRequested && !_pollInFlight;
    if (canQueue) {
        _pollRequested = true;
    }
    xSemaphoreGive(_mutex);

    if (canQueue) xTaskNotifyGive(_task);
    return canQueue;
}

bool GoodWeWorker::takeLatest(
    SolarData& data,
    bool& success,
    uint32_t& completedMs) {

    if (_mutex == nullptr || xSemaphoreTake(_mutex, 0) != pdTRUE) {
        return false;
    }

    const bool available = _hasLatest;
    if (available) {
        data = _latest;
        success = _latestSuccess;
        completedMs = _latestCompletedMs;
        _hasLatest = false;
    }
    xSemaphoreGive(_mutex);
    return available;
}

void GoodWeWorker::taskEntry(void* parameter) {
    static_cast<GoodWeWorker*>(parameter)->taskLoop();
}

void GoodWeWorker::taskLoop() {
    GoodWeClient client;
    uint32_t appliedGeneration = 0;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        GoodWeConfig config;
        uint32_t generation = 0;
        bool shouldPoll = false;

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            config = _config;
            generation = _configGeneration;
            shouldPoll = _pollRequested && config.enabled;
            _pollRequested = false;
            if (shouldPoll) _pollInFlight = true;
            xSemaphoreGive(_mutex);
        }

        if (!shouldPoll) continue;

        if (appliedGeneration != generation) {
            client.begin(config.host, config.port);
            appliedGeneration = generation;
        }

        SolarData result;
        result.enabled = config.enabled;

        // Weather HTTPS can consume nearly all internal Wi-Fi/TLS buffers on
        // the classic ESP32. Serialize GoodWe UDP with the shared network gate
        // so socket allocation is postponed instead of failing under TLS load.
        bool success = false;
        const bool gateTaken =
            _networkClientGate == nullptr ||
            xSemaphoreTake(_networkClientGate, portMAX_DELAY) == pdTRUE;
        if (gateTaken) {
            success = client.update(result);
            if (_networkClientGate != nullptr) {
                xSemaphoreGive(_networkClientGate);
            }
        }
        const uint32_t completedMs = millis();

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            if (generation == _configGeneration) {
                _latest = result;
                _latestSuccess = success;
                _latestCompletedMs = completedMs;
                _hasLatest = true;
            }
            _pollInFlight = false;
            xSemaphoreGive(_mutex);
        }
    }
}
