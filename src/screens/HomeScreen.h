#pragma once
#include "IScreen.h"

class HomeScreen : public IScreen {
public:
    String getId() const override { return "home"; }
    String getTitle() const override { return "Hlavní souhrn"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
};
