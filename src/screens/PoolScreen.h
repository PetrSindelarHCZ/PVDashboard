#pragma once
#include "IScreen.h"

class PoolScreen : public IScreen {
public:
    String getId() const override { return "pool"; }
    String getTitle() const override { return "Bazén"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
};
