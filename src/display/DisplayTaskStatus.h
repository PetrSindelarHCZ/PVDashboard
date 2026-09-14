#pragma once
#include <Arduino.h>

enum class DisplayTaskState : uint8_t {
    Stopped,
    Initializing,
    Idle,
    Queued,
    RenderingPartial,
    RenderingFull,
    Error
};

struct DisplayTaskStatus {
    DisplayTaskState state = DisplayTaskState::Stopped;
    bool ready = false;
    bool pending = false;
    bool pendingFull = false;
    uint32_t completedCount = 0;
    uint32_t lastDurationMs = 0;
    uint32_t lastCompletedMs = 0;
    uint32_t stackHighWaterWords = 0;
};

const char* displayTaskStateName(DisplayTaskState state);