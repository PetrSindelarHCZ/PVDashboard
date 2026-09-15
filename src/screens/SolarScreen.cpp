#include "SolarScreen.h"
#include "ScreenStyle.h"

void SolarScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 225, 190, "SOLÁRNÍ VÝROBA");
    ScreenStyle::useMetric(display);
    display.setCursor(90, 145);
    display.printf("%.0f W", dm.solar.productionPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(90, 185);
    display.printf("Dnes: %.1f kWh", dm.solar.energyTodayKWh);
    display.setCursor(90, 210);
    display.printf("Status: %s", dm.solar.status.available ? "Online" : "Nedostupné");

    ScreenStyle::drawCard(display, 315, 63, 225, 190, "BATERIE");
    ScreenStyle::useMetric(display);
    display.setCursor(330, 145);
    display.printf("%.0f %%", dm.solar.batterySocPercent);
    ScreenStyle::useBody(display);
    display.setCursor(330, 185);
    display.printf("Tok: %+.0f W", dm.solar.batteryPowerW);
    display.setCursor(330, 210);
    display.printf("Stav: %s", dm.solar.batteryPowerW < 0 ? "Nabíjení" :
                   (dm.solar.batteryPowerW > 0 ? "Vybíjení" : "Klid"));

    ScreenStyle::drawCard(display, 555, 63, 230, 190, "DISTRIBUCE");
    ScreenStyle::useMetric(display);
    display.setCursor(570, 145);
    display.printf("%+.0f W", dm.solar.gridPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(570, 185);
    display.print(dm.solar.gridPowerW >= 0 ? "Přetok do sítě" : "Nákup ze sítě");

    ScreenStyle::drawCard(display, 75, 268, 225, 197, "SPOTŘEBA DOMU");
    ScreenStyle::useMetric(display);
    display.setCursor(90, 350);
    display.printf("%.0f W", dm.solar.houseConsumptionW);
    ScreenStyle::useBody(display);
    display.setCursor(90, 395);
    display.print("Okamžitý příkon");

    ScreenStyle::drawCard(display, 315, 268, 225, 197, "AZ ROUTER");
    ScreenStyle::useMetric(display);
    display.setCursor(330, 350);
    display.printf("%.0f W", dm.azrouter.routedPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(330, 395);
    display.printf("Bojler: %.1f °C", dm.azrouter.boilerTempC);
    display.setCursor(330, 425);
    display.printf("Dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);

    ScreenStyle::drawCard(display, 555, 268, 230, 197, "STAV ZDROJŮ");
    ScreenStyle::useBody(display);
    display.setCursor(570, 333);
    display.printf("GoodWe UDP: %s", dm.solar.status.available ? "OK" : "Offline");
    display.setCursor(570, 375);
    display.printf("AZRouter HTTP: %s", dm.azrouter.status.available ? "OK" : "Offline");
    display.setCursor(570, 417);
    display.print("Samospotřeba: OK");
}
