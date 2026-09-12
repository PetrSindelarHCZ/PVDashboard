#pragma once
#include <Arduino.h>
#include "../display/IDisplay.h"
#include "../data/DataModel.h"

class IScreen {
public:
    virtual ~IScreen() = default;

    virtual String getId() const = 0;
    virtual String getTitle() const = 0;
    virtual void render(IDisplay& display, const DataModel& dataModel) = 0;
};
