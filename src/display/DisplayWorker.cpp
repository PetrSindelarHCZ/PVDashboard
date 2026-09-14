#include "DisplayWorker.h"

const char* displayTaskStateName(DisplayTaskState state) {
    switch (state) {
        case DisplayTaskState::Stopped: return "stopped";
        case DisplayTaskState::Initializing: return "initializing";
        case DisplayTaskState::Idle: return "idle";
        case DisplayTaskState::Queued: return "queued";
        case DisplayTaskState::RenderingPartial: return "rendering_partial";
        case DisplayTaskState::RenderingFull: return "rendering_full";
        case DisplayTaskState::Error: return "error";
    }
    return "unknown";
}

DisplayWorker::DisplayWorker(DisplayManager& displayManager)
    : _displayManager(displayManager) {
}

bool DisplayWorker::begin() {
    if (_task != nullptr) {
        return true;
    }

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[DISPLAY-WORKER] Nelze vytvorit mutex.");
        setError();
        return false;
    }

    _status.state = DisplayTaskState::Initializing;
    const BaseType_t result = xTaskCreatePinnedToCore(
        taskEntry,
        "displayTask",
        TaskStackWords,
        this,
        TaskPriority,
        &_task,
        ARDUINO_RUNNING_CORE);
    if (result != pdPASS) {
        Serial.println("[DISPLAY-WORKER] Nelze vytvorit task.");
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
        _task = nullptr;
        setError();
        return false;
    }

    return true;
}

bool DisplayWorker::enqueue(IScreen* screen, const DataModel& dataModel, bool full) {
    if (_mutex == nullptr || _task == nullptr || screen == nullptr) {
        return false;
    }

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[DISPLAY-WORKER] Timeout pri frontovani pozadavku.");
        return false;
    }

    _pendingData = dataModel;
    _pendingScreen = screen;
    _pendingFull = _hasPending ? (_pendingFull || full) : full;
    _hasPending = true;
    _status.pending = true;
    _status.pendingFull = _pendingFull;
    if (_status.ready &&
        _status.state != DisplayTaskState::RenderingPartial &&
        _status.state != DisplayTaskState::RenderingFull) {
        _status.state = DisplayTaskState::Queued;
    }
    xSemaphoreGive(_mutex);

    xTaskNotifyGive(_task);
    return true;
}

DisplayTaskStatus DisplayWorker::getStatus() {
    if (_mutex == nullptr) {
        return _status;
    }

    xSemaphoreTake(_mutex, portMAX_DELAY);
    const DisplayTaskStatus snapshot = _status;
    xSemaphoreGive(_mutex);
    return snapshot;
}

void DisplayWorker::taskEntry(void* parameter) {
    static_cast<DisplayWorker*>(parameter)->taskLoop();
}

void DisplayWorker::taskLoop() {
    _displayManager.init();

    xSemaphoreTake(_mutex, portMAX_DELAY);
    _status.ready = true;
    _status.state = _hasPending ? DisplayTaskState::Queued : DisplayTaskState::Idle;
    xSemaphoreGive(_mutex);

    DataModel dataSnapshot;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (;;) {
            xSemaphoreTake(_mutex, portMAX_DELAY);
            if (!_hasPending) {
                _status.state = DisplayTaskState::Idle;
                _status.pending = false;
                _status.pendingFull = false;
                xSemaphoreGive(_mutex);
                break;
            }

            dataSnapshot = _pendingData;
            IScreen* screen = _pendingScreen;
            const bool full = _pendingFull;
            _hasPending = false;
            _pendingFull = false;
            _status.pending = false;
            _status.pendingFull = false;
            _status.state = full
                ? DisplayTaskState::RenderingFull
                : DisplayTaskState::RenderingPartial;
            xSemaphoreGive(_mutex);

            const uint32_t startedMs = millis();
            _displayManager.renderScreen(screen, dataSnapshot, full);
            const uint32_t completedMs = millis();

            xSemaphoreTake(_mutex, portMAX_DELAY);
            _status.completedCount++;
            _status.lastDurationMs = completedMs - startedMs;
            _status.lastCompletedMs = completedMs;
            _status.stackHighWaterWords = uxTaskGetStackHighWaterMark(nullptr);
            _status.pending = _hasPending;
            _status.pendingFull = _pendingFull;
            _status.state = _hasPending ? DisplayTaskState::Queued : DisplayTaskState::Idle;
            xSemaphoreGive(_mutex);
        }
    }
}

void DisplayWorker::setError() {
    _status.ready = false;
    _status.state = DisplayTaskState::Error;
}