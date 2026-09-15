#include "PoolScreen.h"
#include "ScreenStyle.h"

void PoolScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 166, 132, "TEPLOTA VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(85, 135);
    display.printf("%.1f C", dm.pool.waterTempC);
    ScreenStyle::useBody(display);
    display.setCursor(85, 165);
    display.printf("Cil: %.1f C", dm.pool.targetTempC);

    ScreenStyle::drawCard(display, 251, 63, 166, 132, "PH VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(261, 135);
    display.printf("%.1f", dm.pool.ph);
    ScreenStyle::useBody(display);
    display.setCursor(261, 165);
    display.print("Ideal: 7.0-7.4");

    ScreenStyle::drawCard(display, 427, 63, 166, 132, "VOLNY CHLOR");
    ScreenStyle::useMetric(display);
    display.setCursor(437, 135);
    display.printf("%.1f", dm.pool.freeChlorineMgL);
    ScreenStyle::useBody(display);
    display.setCursor(437, 165);
    display.print("mg/l | Ideal 0.3-1.0");

    ScreenStyle::drawCard(display, 603, 63, 182, 132, "VZDUCH");
    ScreenStyle::useMetric(display);
    display.setCursor(613, 135);
    display.printf("%.1f C", dm.pool.airTempC);
    ScreenStyle::useBody(display);
    display.setCursor(613, 165);
    display.printf("Vlhkost: %d %%", dm.pool.airHumidityPercent);

    ScreenStyle::drawCard(display, 75, 210, 450, 255, "STAV TECHNOLOGIE");
    ScreenStyle::useBody(display);
    display.setCursor(90, 290);
    display.print("Filtrace:");
    display.setCursor(260, 290);
    display.print(dm.pool.filtrationRunning ? "Bezi (automatika)" : "Vypnuto");
    display.setCursor(90, 335);
    display.print("Ohrev FVE:");
    display.setCursor(260, 335);
    display.print(dm.pool.heatingActive ? "Aktivni" : "Vypnuto");
    display.setCursor(90, 380);
    display.print("Davkovani pH / Cl:");
    display.setCursor(260, 380);
    display.print("OK");
    display.setCursor(90, 425);
    display.print("Osvetleni / UV:");
    display.setCursor(260, 425);
    display.print("Vypnuto");

    ScreenStyle::drawCard(display, 535, 210, 250, 255, "INFORMACE - DEMO");
    ScreenStyle::useBody(display);
    display.setCursor(550, 290);
    display.print("Objem: 32 m3");
    display.setCursor(550, 335);
    display.print("Posledni udrzba: 6. 9.");
    display.setCursor(550, 380);
    display.print("Demonstracni hodnoty");

}
