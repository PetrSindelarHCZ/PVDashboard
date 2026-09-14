#pragma once
#include <Arduino.h>
#include "IDisplay.h"
#include "../screens/IScreen.h"
#include "../data/DataModel.h"

class DisplayManager {
public:
    static constexpr uint8_t MaxConsecutivePartialRefreshes = 5;

    explicit DisplayManager(IDisplay& display);

    void init();
    void renderScreen(IScreen* screen, const DataModel& dataModel, bool forceFullRefresh = false);
    void requestRefresh(bool full = false);

private:
    IDisplay& _display;
    unsigned long _lastFullRefreshMs = 0;
    bool _forceFullRefresh = true;
    uint8_t _consecutivePartialRefreshes = 0;
};
