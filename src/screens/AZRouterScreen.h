#pragma once
#include "IScreen.h"

class AZRouterScreen : public IScreen {
public:
    String getId() const override { return "azrouter"; }
    String getTitle() const override { return "AZRouter"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
};
