#include "HomeScreen.h"
#include "ScreenStyle.h"

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    // Pokud je modul Počasí vypnutý, nezobrazujeme ani prázdnou weather kartu.
    // Energetická část využije uvolněné místo.
    if (!dm.weather.enabled) {
        ScreenStyle::drawCard(display, 75, 63, 465, 402, "ENERGIE");

        ScreenStyle::useBody(display);
        display.setCursor(95, 125);
        display.print("Výroba FVE");
        ScreenStyle::useValue(display);
        display.setCursor(95, 150);
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(95, 235);
        display.print("Spotřeba domu");
        ScreenStyle::useValue(display);
        display.setCursor(95, 260);
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(320, 125);
        display.print("Distribuce");
        ScreenStyle::useValue(display);
        display.setCursor(320, 150);
        display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(320, 235);
        display.print("Baterie");
        ScreenStyle::useValue(display);
        display.setCursor(320, 260);
        display.printf("%.0f %%", dm.solar.batterySocPercent);
        ScreenStyle::useBody(display);
        display.setCursor(320, 300);
        display.printf("%+.0f W", dm.solar.batteryPowerW);

        ScreenStyle::drawCard(display, 555, 63, 230, 402, "UVNITŘ - DEMO");
        ScreenStyle::useBody(display);
        display.setCursor(570, 125);
        display.print("Obývák");
        ScreenStyle::useValue(display);
        display.setCursor(570, 150);
        display.printf("%.1f °C", dm.inside.livingRoomTempC);

        ScreenStyle::useBody(display);
        display.setCursor(570, 195);
        display.print("Ložnice");
        ScreenStyle::useValue(display);
        display.setCursor(570, 220);
        display.printf("%.1f °C", dm.inside.bedroomTempC);

        ScreenStyle::useBody(display);
        display.setCursor(570, 265);
        display.print("CO2 v místnosti");
        ScreenStyle::useValue(display);
        display.setCursor(570, 290);
        display.printf("%d ppm", dm.inside.co2Ppm);

        ScreenStyle::useBody(display);
        display.setCursor(570, 335);
        display.print("Bazén");
        ScreenStyle::useValue(display);
        display.setCursor(570, 360);
        display.printf("%.1f °C", dm.inside.poolTempC);

        NavigationLayout navigationLayout;
        buildNavigationLayout(dm, navigationLayout);
        ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
        return;
    }

    ScreenStyle::drawCard(display, 75, 63, 225, 402, "VENKU");
    if (dm.weather.status.available) {
        ScreenStyle::drawWeatherSymbol(display, 248, 130, dm.weather.weatherCode);

        ScreenStyle::useMetric(display);
        display.setCursor(90, 145);
        display.printf("%.1f °C", dm.weather.outdoorTempC);

        ScreenStyle::useBody(display);
        display.setCursor(90, 185);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(90, 235);
        display.print("Dnešní rozsah");
        ScreenStyle::useValue(display);
        display.setCursor(90, 270);
        display.printf("%.0f / %.0f °C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
        ScreenStyle::useBody(display);
        display.setCursor(90, 325);
        display.printf("Stav: %s", dm.weather.conditionText.c_str());
        display.setCursor(90, 375);
        display.print(dm.weather.locationName.isEmpty() ? dm.weather.provider : dm.weather.locationName);
    } else {
        ScreenStyle::useValue(display);
        display.setCursor(90, 145);
        display.print("--.- °C");
        ScreenStyle::useBody(display);
        display.setCursor(90, 195);
        display.print("Počasí nedostupné");
        display.setCursor(90, 235);
        display.print(dm.weather.status.lastError);
    }

    if (!dm.weather.provider.isEmpty()) {
        ScreenStyle::useBody(display);
        const String providerLabel = "Data: " + dm.weather.provider;
        display.setCursor(290 - display.textWidth(providerLabel), 445);
        display.print(providerLabel);
    }

    ScreenStyle::drawCard(display, 315, 63, 225, 402, "ENERGIE");
    ScreenStyle::useBody(display);
    display.setCursor(330, 125);
    display.print("Výroba FVE");
    ScreenStyle::useValue(display);
    display.setCursor(330, 150);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(330, 195);
    display.print("Spotřeba domu");
    ScreenStyle::useValue(display);
    display.setCursor(330, 220);
    display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(330, 265);
    display.print("Distribuce");
    ScreenStyle::useValue(display);
    display.setCursor(330, 290);
    display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(330, 335);
    display.print("Baterie");
    ScreenStyle::useValue(display);
    display.setCursor(330, 360);
    display.printf("%.0f %% (%+.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);

    ScreenStyle::drawCard(display, 555, 63, 230, 402, "UVNITŘ - DEMO");
    ScreenStyle::useBody(display);
    display.setCursor(570, 125);
    display.print("Obývák");
    ScreenStyle::useValue(display);
    display.setCursor(570, 150);
    display.printf("%.1f °C", dm.inside.livingRoomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(570, 195);
    display.print("Ložnice");
    ScreenStyle::useValue(display);
    display.setCursor(570, 220);
    display.printf("%.1f °C", dm.inside.bedroomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(570, 265);
    display.print("CO2 v místnosti");
    ScreenStyle::useValue(display);
    display.setCursor(570, 290);
    display.printf("%d ppm", dm.inside.co2Ppm);

    ScreenStyle::useBody(display);
    display.setCursor(570, 335);
    display.print("Bazén");
    ScreenStyle::useValue(display);
    display.setCursor(570, 360);
    display.printf("%.1f °C", dm.inside.poolTempC);

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void HomeScreen::buildNavigationLayout(const DataModel& dm, NavigationLayout& layout) const {
    layout.clear();

    // These rectangles describe the current rendered layout. The controller
    // derives neighbours from geometry every time; it never knows that
    // "weather" is left of "energy". When Home becomes configuration-driven,
    // the page builder can feed the same NavigationLayout directly from the
    // active widget configuration.
    if (dm.weather.enabled) {
        layout.add("weather-card", 75, 63, 225, 402);
        layout.add("energy-card", 315, 63, 225, 402);
        layout.add("indoor-card", 555, 63, 230, 402);
    } else {
        layout.add("energy-card", 75, 63, 465, 402);
        layout.add("indoor-card", 555, 63, 230, 402);
    }

    // entryPointId deliberately remains empty: the generic rule selects the
    // left-most focusable widget (and top-most on ties).
}
