#pragma once
#include "IScreen.h"

class FontTestScreen : public IScreen {
public:
    String getId() const override { return "font-test"; }
    String getTitle() const override { return "Test fontů"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
};
