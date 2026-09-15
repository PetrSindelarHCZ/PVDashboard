#include "HomeScreen.h"
#include "ScreenStyle.h"

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 225, 402, "VENKU");
    if (dm.weather.status.available) {
        ScreenStyle::drawWeatherSymbol(display, 248, 130, dm.weather.weatherCode);

        ScreenStyle::useMetric(display);
        display.setCursor(90, 145);
        display.printf("%.1f C", dm.weather.outdoorTempC);

        ScreenStyle::useBody(display);
        display.setCursor(90, 185);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(90, 235);
        display.print("Dnesni rozsah");
        ScreenStyle::useValue(display);
        display.setCursor(90, 270);
        display.printf("%.0f / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
        ScreenStyle::useBody(display);
        display.setCursor(90, 325);
        display.printf("Stav: %s", dm.weather.conditionText.c_str());
        display.setCursor(90, 375);
        display.print(dm.weather.provider);
    } else {
        ScreenStyle::useValue(display);
        display.setCursor(90, 145);
        display.print("--.- C");
        ScreenStyle::useBody(display);
        display.setCursor(90, 195);
        display.print("Pocasi nedostupne");
        display.setCursor(90, 235);
        display.print(dm.weather.status.lastError);
    }

    ScreenStyle::drawCard(display, 315, 63, 225, 402, "ENERGIE");
    ScreenStyle::useBody(display);
    display.setCursor(330, 125);
    display.print("Vyroba FVE");
    ScreenStyle::useValue(display);
    display.setCursor(330, 150);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(330, 195);
    display.print("Spotreba domu");
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

    ScreenStyle::drawCard(display, 555, 63, 230, 402, "UVNITR - DEMO");
    ScreenStyle::useBody(display);
    display.setCursor(570, 125);
    display.print("Obyvak");
    ScreenStyle::useValue(display);
    display.setCursor(570, 150);
    display.printf("%.1f C", dm.inside.livingRoomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(570, 195);
    display.print("Loznice");
    ScreenStyle::useValue(display);
    display.setCursor(570, 220);
    display.printf("%.1f C", dm.inside.bedroomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(570, 265);
    display.print("CO2 v mistnosti");
    ScreenStyle::useValue(display);
    display.setCursor(570, 290);
    display.printf("%d ppm", dm.inside.co2Ppm);

    ScreenStyle::useBody(display);
    display.setCursor(570, 335);
    display.print("Bazen");
    ScreenStyle::useValue(display);
    display.setCursor(570, 360);
    display.printf("%.1f C", dm.inside.poolTempC);
}