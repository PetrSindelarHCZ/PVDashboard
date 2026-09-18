#include "PoolScreen.h"
#include "ScreenStyle.h"

void PoolScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 166, 132, "TEPLOTA VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(85, 135);
    display.printf("%.1f °C", dm.pool.waterTempC);
    ScreenStyle::useBody(display);
    display.setCursor(85, 165);
    display.printf("Cíl: %.1f °C", dm.pool.targetTempC);

    ScreenStyle::drawCard(display, 251, 63, 166, 132, "PH VODY");
    ScreenStyle::useMetric(display);
    display.setCursor(261, 135);
    display.printf("%.1f", dm.pool.ph);
    ScreenStyle::useBody(display);
    display.setCursor(261, 165);
    display.print("Ideál: 7.0-7.4");

    ScreenStyle::drawCard(display, 427, 63, 166, 132, "VOLNÝ CHLÓR");
    ScreenStyle::useMetric(display);
    display.setCursor(437, 135);
    display.printf("%.1f", dm.pool.freeChlorineMgL);
    ScreenStyle::useBody(display);
    display.setCursor(437, 165);
    display.print("mg/l | Ideál 0.3-1.0");

    ScreenStyle::drawCard(display, 603, 63, 182, 132, "VZDUCH");
    ScreenStyle::useMetric(display);
    display.setCursor(613, 135);
    display.printf("%.1f °C", dm.pool.airTempC);
    ScreenStyle::useBody(display);
    display.setCursor(613, 165);
    display.printf("Vlhkost: %d %%", dm.pool.airHumidityPercent);

    ScreenStyle::drawCard(display, 75, 210, 450, 255, "STAV TECHNOLOGIE");
    ScreenStyle::useBody(display);
    display.setCursor(90, 290);
    display.print("Filtrace:");
    display.setCursor(260, 290);
    display.print(dm.pool.filtrationRunning ? "Běží (automatika)" : "Vypnuto");
    display.setCursor(90, 335);
    display.print("Ohřev FVE:");
    display.setCursor(260, 335);
    display.print(dm.pool.heatingActive ? "Aktivní" : "Vypnuto");
    display.setCursor(90, 380);
    display.print("Dávkování pH / Cl:");
    display.setCursor(260, 380);
    display.print("OK");
    display.setCursor(90, 425);
    display.print("Osvětlení / UV:");
    display.setCursor(260, 425);
    display.print("Vypnuto");

    ScreenStyle::drawCard(display, 535, 210, 250, 255, "INFORMACE - DEMO");
    ScreenStyle::useBody(display);
    display.setCursor(550, 290);
    display.print("Objem: 32 m3");
    display.setCursor(550, 335);
    display.print("Poslední údržba: 6. 9.");
    display.setCursor(550, 380);
    display.print("Demonstrační hodnoty");

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void PoolScreen::buildNavigationLayout(const DataModel&, NavigationLayout& layout) const {
    layout.clear();
    layout.add("water-card", 75, 63, 166, 132);
    layout.add("ph-card", 251, 63, 166, 132);
    layout.add("chlorine-card", 427, 63, 166, 132);
    layout.add("air-card", 603, 63, 182, 132);
    layout.add("technology-card", 75, 210, 450, 255);
    layout.add("info-card", 535, 210, 250, 255);
}
