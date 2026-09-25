#include "DisplayWorker.h"
#include <esp_heap_caps.h>

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

void DisplayWorker::setMemoryHeavyGate(SemaphoreHandle_t gate) {
    _memoryHeavyGate = gate;
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

bool DisplayWorker::enqueue(IScreen* screen, const DataModel& dataModel, bool full,
                            const DisplayRegion* partialRegion,
                            bool capturePreview) {
    if (_mutex == nullptr || _task == nullptr || screen == nullptr) {
        return false;
    }

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[DISPLAY-WORKER] Timeout pri frontovani pozadavku.");
        return false;
    }

    _pendingData = dataModel;
    _pendingScreen = screen;

    const bool hadPending = _hasPending;
    _pendingFull = hadPending ? (_pendingFull || full) : full;

    if (_pendingFull || partialRegion == nullptr || !partialRegion->valid()) {
        _pendingHasRegion = false;
    } else if (!hadPending) {
        _pendingRegion = *partialRegion;
        _pendingHasRegion = true;
    } else if (_pendingHasRegion) {
        const int16_t x1 = min(_pendingRegion.x, partialRegion->x);
        const int16_t y1 = min(_pendingRegion.y, partialRegion->y);
        const int16_t x2 = max(
            static_cast<int16_t>(_pendingRegion.x + _pendingRegion.width),
            static_cast<int16_t>(partialRegion->x + partialRegion->width));
        const int16_t y2 = max(
            static_cast<int16_t>(_pendingRegion.y + _pendingRegion.height),
            static_cast<int16_t>(partialRegion->y + partialRegion->height));
        _pendingRegion.x = x1;
        _pendingRegion.y = y1;
        _pendingRegion.width = x2 - x1;
        _pendingRegion.height = y2 - y1;
    }
    // If an older pending request already covers the whole screen,
    // keep it whole-screen; a later cursor region must not narrow it.

    _pendingCapturePreview =
        hadPending ? (_pendingCapturePreview || capturePreview) : capturePreview;

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
    bool waitedForGate = false;
    if (_memoryHeavyGate != nullptr &&
        xSemaphoreTake(_memoryHeavyGate, 0) != pdTRUE) {
        waitedForGate = true;
        Serial.println("[DISPLAY-WORKER] Cekam na memory gate pred inicializaci displeje...");
        xSemaphoreTake(_memoryHeavyGate, portMAX_DELAY);
    }

    Serial.printf(
        "[MEM][DISPLAY] sizeof(DataModel)=%u bytes taskStack=%u words free=%lu min=%lu largest=%lu\n",
        static_cast<unsigned>(sizeof(DataModel)),
        static_cast<unsigned>(TaskStackWords),
        static_cast<unsigned long>(ESP.getFreeHeap()),
        static_cast<unsigned long>(ESP.getMinFreeHeap()),
        static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));

    _displayManager.init();

    if (_memoryHeavyGate != nullptr) {
        xSemaphoreGive(_memoryHeavyGate);
        if (waitedForGate) {
            Serial.println("[DISPLAY-WORKER] Memory gate po inicializaci uvolnen.");
        }
    }

    xSemaphoreTake(_mutex, portMAX_DELAY);
    _status.ready = true;
    _status.state = _hasPending ? DisplayTaskState::Queued : DisplayTaskState::Idle;
    xSemaphoreGive(_mutex);

    DataModel dataSnapshot;
    for (;;) {
        const uint32_t notifications =
            ulTaskNotifyTake(
                pdTRUE,
                pdMS_TO_TICKS(PartialIdlePowerOffMs));

        if (notifications == 0) {
            // No render request arrived for a while. End the current partial
            // update burst and remove panel drive voltages. The controller RAM
            // remains intact, so the next partial update can continue from the
            // last displayed image.
            _displayManager.powerOff();
            continue;
        }

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
            const bool hasRegion = _pendingHasRegion && !full;
            const DisplayRegion region = _pendingRegion;
            const bool capturePreview = _pendingCapturePreview;

            _hasPending = false;
            _pendingFull = false;
            _pendingHasRegion = false;
            _pendingCapturePreview = true;
            _status.pending = false;
            _status.pendingFull = false;
            _status.state = full
                ? DisplayTaskState::RenderingFull
                : DisplayTaskState::RenderingPartial;
            xSemaphoreGive(_mutex);

            bool waitedForGate = false;
            if (_memoryHeavyGate != nullptr &&
                xSemaphoreTake(_memoryHeavyGate, 0) != pdTRUE) {
                waitedForGate = true;
                Serial.println("[DISPLAY-WORKER] Cekam na memory gate pred renderem...");
                xSemaphoreTake(_memoryHeavyGate, portMAX_DELAY);
            }

            if (waitedForGate) {
                Serial.println("[DISPLAY-WORKER] Memory gate ziskan, render muze zacit.");
            }

            const uint32_t freeBefore = ESP.getFreeHeap();
            const uint32_t minFreeBefore = ESP.getMinFreeHeap();
            const uint32_t largestBefore =
                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
            const UBaseType_t stackBefore =
                uxTaskGetStackHighWaterMark(nullptr);

            Serial.printf(
                "[MEM][DISPLAY] before screen=%s mode=%s free=%lu min=%lu largest=%lu stackHWM=%lu words\n",
                screen->getId().c_str(),
                full ? "FULL" : "PARTIAL",
                static_cast<unsigned long>(freeBefore),
                static_cast<unsigned long>(minFreeBefore),
                static_cast<unsigned long>(largestBefore),
                static_cast<unsigned long>(stackBefore));

            const uint32_t startedMs = millis();
            _displayManager.renderScreen(
                screen,
                dataSnapshot,
                full,
                hasRegion ? &region : nullptr,
                capturePreview);
            const uint32_t completedMs = millis();

            const uint32_t freeAfter = ESP.getFreeHeap();
            const uint32_t minFreeAfter = ESP.getMinFreeHeap();
            const uint32_t largestAfter =
                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
            const UBaseType_t stackAfter =
                uxTaskGetStackHighWaterMark(nullptr);

            Serial.printf(
                "[MEM][DISPLAY] after  screen=%s free=%lu min=%lu largest=%lu stackHWM=%lu words delta=%ld\n",
                screen->getId().c_str(),
                static_cast<unsigned long>(freeAfter),
                static_cast<unsigned long>(minFreeAfter),
                static_cast<unsigned long>(largestAfter),
                static_cast<unsigned long>(stackAfter),
                static_cast<long>(freeAfter) - static_cast<long>(freeBefore));

            if (_memoryHeavyGate != nullptr) {
                xSemaphoreGive(_memoryHeavyGate);
            }

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