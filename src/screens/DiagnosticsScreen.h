#pragma once
#include "IScreen.h"

class DiagnosticsScreen : public IScreen {
public:
    String getId() const override { return "diagnostics"; }
    String getTitle() const override { return "Diagnostika"; }
    void render(IDisplay& display, const DataModel& dataModel) override;
};
