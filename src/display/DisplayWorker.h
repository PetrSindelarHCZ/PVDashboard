#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "DisplayManager.h"
#include "DisplayTaskStatus.h"
#include "../data/DataModel.h"
#include "../screens/IScreen.h"

class DisplayWorker {
public:
    explicit DisplayWorker(DisplayManager& displayManager);

    bool begin();
    bool enqueue(IScreen* screen, const DataModel& dataModel, bool full);
    DisplayTaskStatus getStatus();

private:
    static constexpr uint32_t TaskStackWords = 8192;
    static constexpr UBaseType_t TaskPriority = 1;

    DisplayManager& _displayManager;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;
    DataModel _pendingData;
    IScreen* _pendingScreen = nullptr;
    bool _hasPending = false;
    bool _pendingFull = false;
    DisplayTaskStatus _status;

    static void taskEntry(void* parameter);
    void taskLoop();
    void setError();
};