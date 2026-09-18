#include "HomeScreen.h"
#include "ScreenStyle.h"

namespace {

void drawWeatherCard(IDisplay& display, const DataModel& dm, int16_t x, int16_t w) {
    ScreenStyle::drawCard(display, x, 63, w, 402, "VENKU");

    if (dm.weather.status.available) {
        ScreenStyle::drawWeatherSymbol(display, x + w - 52, 130, dm.weather.weatherCode);

        ScreenStyle::useMetric(display);
        display.setCursor(x + 15, 145);
        display.printf("%.1f °C", dm.weather.outdoorTempC);

        ScreenStyle::useBody(display);
        display.setCursor(x + 15, 185);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(x + 15, 235);
        display.print("Dnešní rozsah");
        ScreenStyle::useValue(display);
        display.setCursor(x + 15, 270);
        display.printf("%.0f / %.0f °C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
        ScreenStyle::useBody(display);
        display.setCursor(x + 15, 325);
        display.printf("Stav: %s", dm.weather.conditionText.c_str());
        display.setCursor(x + 15, 375);
        display.print(dm.weather.locationName.isEmpty() ? dm.weather.provider : dm.weather.locationName);
    } else {
        ScreenStyle::useValue(display);
        display.setCursor(x + 15, 145);
        display.print("--.- °C");
        ScreenStyle::useBody(display);
        display.setCursor(x + 15, 195);
        display.print("Počasí nedostupné");
        display.setCursor(x + 15, 235);
        display.print(dm.weather.status.lastError);
    }

    if (!dm.weather.provider.isEmpty()) {
        ScreenStyle::useBody(display);
        const String providerLabel = "Data: " + dm.weather.provider;
        display.setCursor(x + w - 10 - display.textWidth(providerLabel), 445);
        display.print(providerLabel);
    }
}

void drawEnergyCard(IDisplay& display, const DataModel& dm, int16_t x, int16_t w) {
    ScreenStyle::drawCard(display, x, 63, w, 402, "ENERGIE");

    if (w >= 400) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display);
        display.setCursor(leftX, 125);
        display.print("Výroba FVE");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, 150);
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(leftX, 235);
        display.print("Spotřeba domu");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, 260);
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(rightX, 125);
        display.print("Distribuce");
        ScreenStyle::useValue(display);
        display.setCursor(rightX, 150);
        display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(rightX, 235);
        display.print("Baterie");
        ScreenStyle::useValue(display);
        display.setCursor(rightX, 260);
        display.printf("%.0f %%", dm.solar.batterySocPercent);
        ScreenStyle::useBody(display);
        display.setCursor(rightX, 300);
        display.printf("%+.0f W", dm.solar.batteryPowerW);
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 125);
    display.print("Výroba FVE");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 150);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 195);
    display.print("Spotřeba domu");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 220);
    display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 265);
    display.print("Distribuce");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 290);
    display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 335);
    display.print("Baterie");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 360);
    display.printf("%.0f %% (%+.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);
}

void drawIndoorCard(IDisplay& display, const DataModel& dm, int16_t x, int16_t w) {
    ScreenStyle::drawCard(display, x, 63, w, 402, "UVNITŘ - DEMO");

    if (w >= 500) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display);
        display.setCursor(leftX, 125);
        display.print("Obývák");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, 150);
        display.printf("%.1f °C", dm.inside.livingRoomTempC);

        ScreenStyle::useBody(display);
        display.setCursor(leftX, 235);
        display.print("Ložnice");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, 260);
        display.printf("%.1f °C", dm.inside.bedroomTempC);

        if (dm.pool.enabled) {
            ScreenStyle::useBody(display);
            display.setCursor(rightX, 125);
            display.print("Bazén");
            ScreenStyle::useValue(display);
            display.setCursor(rightX, 150);
            display.printf("%.1f °C", dm.inside.poolTempC);
        }
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 125);
    display.print("Obývák");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 150);
    display.printf("%.1f °C", dm.inside.livingRoomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, 195);
    display.print("Ložnice");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, 220);
    display.printf("%.1f °C", dm.inside.bedroomTempC);

    if (dm.pool.enabled) {
        ScreenStyle::useBody(display);
        display.setCursor(valueX, 265);
        display.print("Bazén");
        ScreenStyle::useValue(display);
        display.setCursor(valueX, 290);
        display.printf("%.1f °C", dm.inside.poolTempC);
    }
}

} // namespace

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    const bool showWeather = dm.weather.enabled;
    const bool showEnergy = dm.solar.enabled;

    if (showWeather && showEnergy) {
        drawWeatherCard(display, dm, 75, 225);
        drawEnergyCard(display, dm, 315, 225);
        drawIndoorCard(display, dm, 555, 230);
    } else if (showWeather) {
        drawWeatherCard(display, dm, 75, 345);
        drawIndoorCard(display, dm, 435, 350);
    } else if (showEnergy) {
        drawEnergyCard(display, dm, 75, 465);
        drawIndoorCard(display, dm, 555, 230);
    } else {
        drawIndoorCard(display, dm, 75, 710);
    }

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void HomeScreen::buildNavigationLayout(const DataModel& dm, NavigationLayout& layout) const {
    layout.clear();

    const bool showWeather = dm.weather.enabled;
    const bool showEnergy = dm.solar.enabled;

    if (showWeather && showEnergy) {
        layout.add("weather-card", 75, 63, 225, 402);
        layout.add("energy-card", 315, 63, 225, 402);
        layout.add("indoor-card", 555, 63, 230, 402);
    } else if (showWeather) {
        layout.add("weather-card", 75, 63, 345, 402);
        layout.add("indoor-card", 435, 63, 350, 402);
    } else if (showEnergy) {
        layout.add("energy-card", 75, 63, 465, 402);
        layout.add("indoor-card", 555, 63, 230, 402);
    } else {
        layout.add("indoor-card", 75, 63, 710, 402);
    }
}
