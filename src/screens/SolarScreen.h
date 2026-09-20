#pragma once
#include "IScreen.h"

class SolarScreen : public IScreen {
public:
    String getId() const override { return "solar"; }
    String getTitle() const override { return "Fotovoltaika (FVE)"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
    uint8_t getNavigationSubpageCount(const DataModel& dataModel) const override;
    uint8_t getInitialNavigationSubpage(const DataModel&) const override { return 0; }
};
