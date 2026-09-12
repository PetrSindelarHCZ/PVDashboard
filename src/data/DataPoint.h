#pragma once
#include <Arduino.h>

template <typename T>
struct DataPoint {
    T value{};
    bool valid = false;
    uint32_t timestampMs = 0;

    void set(const T& val) {
        value = val;
        valid = true;
        timestampMs = millis();
    }

    void invalidate() {
        valid = false;
    }
};
