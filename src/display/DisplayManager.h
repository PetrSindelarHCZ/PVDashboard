#pragma once
#include <Arduino.h>
#include "IDisplay.h"
#include "../screens/IScreen.h"
#include "../data/DataModel.h"

class DisplayManager {
public:
    explicit DisplayManager(IDisplay& display);

    void init();
    void renderScreen(IScreen* screen, const DataModel& dataModel, bool forceFullRefresh = false);
    void requestRefresh(bool full = false);

private:
    IDisplay& _display;
    bool _forceFullRefresh = true;
};
