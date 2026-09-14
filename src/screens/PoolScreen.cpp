#include "PoolScreen.h"
#include "ScreenStyle.h"

void PoolScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawHeader(display, "BAZEN", dm);

    ScreenStyle::drawCard(display, 15, 65, 180, 120, "TEPLOTA VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(25, 135);
    display.printf("%.1f C", dm.pool.waterTempC);
    ScreenStyle::useBody(display);
    display.setCursor(25, 165);
    display.printf("Cil: %.1f C", dm.pool.targetTempC);

    ScreenStyle::drawCard(display, 210, 65, 180, 120, "PH VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(220, 135);
    display.printf("%.1f", dm.pool.ph);
    ScreenStyle::useBody(display);
    display.setCursor(220, 165);
    display.print("Ideal: 7.0-7.4");

    ScreenStyle::drawCard(display, 405, 65, 180, 120, "VOLNY CHLOR");
    ScreenStyle::useMetric(display);
    display.setCursor(415, 135);
    display.printf("%.1f", dm.pool.freeChlorineMgL);
    ScreenStyle::useBody(display);
    display.setCursor(415, 165);
    display.print("mg/l | Ideal 0.3-1.0");

    ScreenStyle::drawCard(display, 600, 65, 185, 120, "VZDUCH");
    ScreenStyle::useMetric(display);
    display.setCursor(610, 135);
    display.printf("%.1f C", dm.pool.airTempC);
    ScreenStyle::useBody(display);
    display.setCursor(610, 165);
    display.printf("Vlhkost: %d %%", dm.pool.airHumidityPercent);

    ScreenStyle::drawCard(display, 15, 200, 480, 210, "STAV TECHNOLOGIE");
    ScreenStyle::useBody(display);
    display.setCursor(30, 275);
    display.print("Filtrace:");
    display.setCursor(200, 275);
    display.print(dm.pool.filtrationRunning ? "Bezi (automatika)" : "Vypnuto");
    display.setCursor(30, 315);
    display.print("Ohrev FVE:");
    display.setCursor(200, 315);
    display.print(dm.pool.heatingActive ? "Aktivni" : "Vypnuto");
    display.setCursor(30, 355);
    display.print("Davkovani pH / Cl:");
    display.setCursor(200, 355);
    display.print("OK");
    display.setCursor(30, 395);
    display.print("Osvetleni / UV:");
    display.setCursor(200, 395);
    display.print("Vypnuto");

    ScreenStyle::drawCard(display, 510, 200, 275, 210, "INFORMACE");
    ScreenStyle::useBody(display);
    display.setCursor(525, 275);
    display.print("Objem: 32 m3");
    display.setCursor(525, 315);
    display.print("Posledni udrzba: 6. 9.");
    display.setCursor(525, 355);
    display.print("Status: Vse v poradku");

    ScreenStyle::drawFooter(display, "Modul Bazen | pripraven pro senzory");
}