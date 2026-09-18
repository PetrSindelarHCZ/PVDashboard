#pragma once
#include "IScreen.h"

class WeatherScreen : public IScreen {
public:
    explicit WeatherScreen(int8_t forecastDay = -1) : _forecastDay(forecastDay) {}
    String getId() const override {
        return _forecastDay < 0 ? String("weather") : String("weather-hourly-") + String(_forecastDay);
    }
    String getTitle() const override {
        return _forecastDay < 0 ? String("Počasí") : String("Počasí - den ") + String(_forecastDay + 1);
    }
    void render(IDisplay& display, const DataModel& dataModel) override;
    bool isSidebarEntry() const override { return _forecastDay < 0; }
    String getSidebarParentId() const override { return "weather"; }
    void buildNavigationLayout(const DataModel& dataModel, NavigationLayout& layout) const override;
    uint8_t getNavigationSubpageCount(const DataModel& dataModel) const override {
        return _forecastDay < 0 && dataModel.weather.locationCount > 0
            ? dataModel.weather.locationCount
            : 1;
    }
    uint8_t getInitialNavigationSubpage(const DataModel& dataModel) const override {
        return _forecastDay < 0 ? dataModel.weather.locationIndex : 0;
    }

private:
    int8_t _forecastDay;
    void renderHourly(IDisplay& display, const DataModel& dataModel);
};
