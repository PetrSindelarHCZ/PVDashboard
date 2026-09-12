#pragma once
#include "IScreen.h"

class WeatherScreen : public IScreen {
public:
    String getId() const override { return "weather"; }
    String getTitle() const override { return "Pocasi"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
};
