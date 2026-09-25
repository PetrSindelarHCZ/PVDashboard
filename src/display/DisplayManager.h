#pragma once
#include <Arduino.h>
#include "IDisplay.h"
#include "DisplayPreview.h"
#include "../screens/IScreen.h"
#include "../data/DataModel.h"

class DisplayManager {
public:
    explicit DisplayManager(IDisplay& display, DisplayPreview* preview = nullptr);

    void init();
    void renderScreen(IScreen* screen, const DataModel& dataModel,
                      bool forceFullRefresh = false,
                      const DisplayRegion* partialRegion = nullptr,
                      bool capturePreview = true);
    void requestRefresh(bool full = false);
    void powerOff();

private:
    IDisplay& _display;
    DisplayPreview* _preview = nullptr;
    bool _forceFullRefresh = true;
};
