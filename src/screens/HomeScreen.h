#pragma once
#include "IScreen.h"

class HomeScreen : public IScreen {
public:
    String getId() const override { return "home"; }
    String getTitle() const override { return "Hlavni souhrn"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
};
