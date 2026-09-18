#pragma once
#include "IScreen.h"
#include "../layout/ScreenLayout.h"
#include "../config/ConfigSchema.h"

class HomeScreen : public IScreen {
public:
    String getId() const override { return "home"; }
    String getTitle() const override { return "Hlavní souhrn"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
    void setLayoutConfig(const HomeLayoutConfig* config) { _layoutConfig = config; }

private:
    const HomeLayoutConfig* _layoutConfig = nullptr;
    void buildLayout(const DataModel& dataModel, ScreenLayout& layout) const;
};
