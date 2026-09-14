#include "HomeScreen.h"
#include "ScreenStyle.h"

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawHeader(display, "DOMOV", dm);

    ScreenStyle::drawCard(display, 15, 65, 245, 345, "VENKU");
    ScreenStyle::useMetric(display);
    display.setCursor(30, 145);
    display.printf("%.1f C", dm.weather.outdoorTempC);

    ScreenStyle::useBody(display);
    display.setCursor(30, 185);
    display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
    display.setCursor(30, 235);
    display.print("Dnesni rozsah");

    ScreenStyle::useValue(display);
    display.setCursor(30, 270);
    display.printf("%.0f / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);

    ScreenStyle::useBody(display);
    display.setCursor(30, 325);
    display.printf("Stav: %s", dm.weather.conditionText.c_str());

    ScreenStyle::drawCard(display, 275, 65, 245, 345, "ENERGIE");
    ScreenStyle::useBody(display);
    display.setCursor(290, 125);
    display.print("Vyroba FVE");
    ScreenStyle::useValue(display);
    display.setCursor(290, 150);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(290, 195);
    display.print("Spotreba domu");
    ScreenStyle::useValue(display);
    display.setCursor(290, 220);
    display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(290, 265);
    display.print("Distribuce");
    ScreenStyle::useValue(display);
    display.setCursor(290, 290);
    display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(290, 335);
    display.print("Baterie");
    ScreenStyle::useValue(display);
    display.setCursor(290, 360);
    display.printf("%.0f %% (%+.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);

    ScreenStyle::drawCard(display, 535, 65, 250, 345, "UVNITR");
    ScreenStyle::useBody(display);
    display.setCursor(550, 125);
    display.print("Obyvak");
    ScreenStyle::useValue(display);
    display.setCursor(550, 150);
    display.printf("%.1f C", dm.inside.livingRoomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(550, 195);
    display.print("Loznice");
    ScreenStyle::useValue(display);
    display.setCursor(550, 220);
    display.printf("%.1f C", dm.inside.bedroomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(550, 265);
    display.print("CO2 v mistnosti");
    ScreenStyle::useValue(display);
    display.setCursor(550, 290);
    display.printf("%d ppm", dm.inside.co2Ppm);

    ScreenStyle::useBody(display);
    display.setCursor(550, 335);
    display.print("Bazen");
    ScreenStyle::useValue(display);
    display.setCursor(550, 360);
    display.printf("%.1f C", dm.inside.poolTempC);

    String wifi = dm.system.wifiConnected
        ? String("WiFi ") + dm.system.ipAddress + " (" + dm.system.wifiRssi + " dBm)"
        : String("WiFi nepripojeno");
    ScreenStyle::drawFooter(display, wifi,
                            String("[OK] ") + dm.system.statusMessage);
}