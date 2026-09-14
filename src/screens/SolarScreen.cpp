#include "SolarScreen.h"
#include "ScreenStyle.h"

void SolarScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawHeader(display, "FVE A AZ ROUTER", dm);

    ScreenStyle::drawCard(display, 15, 65, 245, 165, "SOLAR VYROBA");
    ScreenStyle::useMetric(display);
    display.setCursor(30, 145);
    display.printf("%.0f W", dm.solar.productionPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(30, 185);
    display.printf("Dnes: %.1f kWh", dm.solar.energyTodayKWh);
    display.setCursor(30, 210);
    display.printf("Status: %s", dm.solar.status.available ? "Online" : "Nedostupne");

    ScreenStyle::drawCard(display, 275, 65, 245, 165, "BATERIE");
    ScreenStyle::useMetric(display);
    display.setCursor(290, 145);
    display.printf("%.0f %%", dm.solar.batterySocPercent);
    ScreenStyle::useBody(display);
    display.setCursor(290, 185);
    display.printf("Tok: %+.0f W", dm.solar.batteryPowerW);
    display.setCursor(290, 210);
    display.printf("Stav: %s", dm.solar.batteryPowerW < 0 ? "Nabijeni" :
                   (dm.solar.batteryPowerW > 0 ? "Vybijeni" : "Klid"));

    ScreenStyle::drawCard(display, 535, 65, 250, 165, "DISTRIBUCE");
    ScreenStyle::useMetric(display);
    display.setCursor(550, 145);
    display.printf("%+.0f W", dm.solar.gridPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(550, 185);
    display.print(dm.solar.gridPowerW >= 0 ? "Pretok do site" : "Nakup ze site");

    ScreenStyle::drawCard(display, 15, 245, 245, 165, "SPOTREBA DOMU");
    ScreenStyle::useMetric(display);
    display.setCursor(30, 325);
    display.printf("%.0f W", dm.solar.houseConsumptionW);
    ScreenStyle::useBody(display);
    display.setCursor(30, 365);
    display.print("Okamzity prikon");

    ScreenStyle::drawCard(display, 275, 245, 245, 165, "AZ ROUTER");
    ScreenStyle::useMetric(display);
    display.setCursor(290, 325);
    display.printf("%.0f W", dm.azrouter.routedPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(290, 365);
    display.printf("Bojler: %.1f C", dm.azrouter.boilerTempC);
    display.setCursor(290, 390);
    display.printf("Dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);

    ScreenStyle::drawCard(display, 535, 245, 250, 165, "STAV ZDROJU");
    ScreenStyle::useBody(display);
    display.setCursor(550, 310);
    display.printf("GoodWe UDP: %s", dm.solar.status.available ? "OK" : "Offline");
    display.setCursor(550, 345);
    display.printf("AZRouter HTTP: %s", dm.azrouter.status.available ? "OK" : "Offline");
    display.setCursor(550, 380);
    display.print("Samospotreba: OK");

    ScreenStyle::drawFooter(display, "GoodWe GW10K-ET + AZ Router Smart Control");
}