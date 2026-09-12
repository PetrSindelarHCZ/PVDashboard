#pragma once
#include "IScreen.h"

class SolarScreen : public IScreen {
public:
    String getId() const override { return "solar"; }
    String getTitle() const override { return "Fotovoltaika (FVE)"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
};
