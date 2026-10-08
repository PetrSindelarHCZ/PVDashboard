#include "SolarScreen.h"
#include "ScreenStyle.h"
#include "../display/EInkGraph.h"
#include <math.h>

namespace {

void drawUnavailableCard(IDisplay& display, const char* title, const char* text) {
    ScreenStyle::drawCard(display, 65, 60, 720, 405, title);
    ScreenStyle::useMetric(display);
    display.setCursor(105, 180);
    display.print("Nedostupné");
    ScreenStyle::useBody(display);
    display.setCursor(105, 225);
    display.print(text);
}

void renderSolar(IDisplay& display, const DataModel& dm) {
    if (!dm.solar.enabled) {
        drawUnavailableCard(display, "GOODWE", "GoodWe je v nastavení vypnutý.");
        return;
    }

    ScreenStyle::drawCard(display, 65, 80, 220, 165, "SOLÁRNÍ VÝROBA");
    ScreenStyle::useMetric(display);
    display.setCursor(90, 170);
    if (dm.solar.status.available) display.printf("%.0f W", dm.solar.productionPowerW);
    else display.print("-- W");
    ScreenStyle::useBody(display);
    display.setCursor(90, 210);
    if (dm.solar.status.available) display.printf("Dnes: %.1f kWh", dm.solar.energyTodayKWh);
    else display.print("Dnes: -- kWh");
    display.setCursor(90, 238);
    display.printf("Status: %s", dm.solar.status.available ? "Online" : "Nedostupné");

    ScreenStyle::drawCard(display, 300, 80, 220, 165, "BATERIE");
    ScreenStyle::useMetric(display);
    display.setCursor(330, 170);
    if (dm.solar.status.available && dm.solar.batteryPresent) display.printf("%.0f %%", dm.solar.batterySocPercent);
    else if (dm.solar.status.available) display.print("Bez baterie");
    else display.print("-- %");
    ScreenStyle::useBody(display);
    display.setCursor(330, 210);
    if (dm.solar.status.available && dm.solar.batteryPresent) display.printf("Tok: %+.0f W", dm.solar.batteryPowerW);
    else if (dm.solar.status.available) display.print("Nepřipojena");
    else display.print("Tok: -- W");
    display.setCursor(330, 238);
    if (dm.solar.status.available && dm.solar.batteryPresent) {
        display.printf("%s", dm.solar.batteryPowerW < 0 ? "Nabíjení" :
                       (dm.solar.batteryPowerW > 0 ? "Vybíjení" : "Klid"));
    } else if (dm.solar.status.available) {
        display.print("Baterie není osazena");
    } else {
        display.print("Nedostupné");
    }

    ScreenStyle::drawCard(display, 535, 80, 250, 165, "DISTRIBUCE");
    ScreenStyle::useMetric(display);
    display.setCursor(570, 170);
    if (dm.solar.status.available) display.printf("%+.0f W", dm.solar.gridPowerW);
    else display.print("-- W");
    ScreenStyle::useBody(display);
    display.setCursor(570, 210);
    if (dm.solar.status.available)
        display.print(dm.solar.gridPowerW >= 0 ? "Přetok do sítě" : "Nákup ze sítě");
    else
        display.print("Data nedostupná");
    display.setCursor(570, 238);
    if (dm.solar.status.available) display.printf("Dům: %.0f W", dm.solar.houseConsumptionW);
    else display.print("Dům: -- W");

    ScreenStyle::drawCard(display, 65, 260, 720, 205, "DNEŠNÍ PRŮBĚH FVE");

    if (!dm.solar.status.available || dm.solar.historyCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 365);
        display.print("Čekám na historická data výroby.");
        return;
    }

    EInkGraphPoint points[SolarHistorySampleCount];
    float maxPower = 1000.0f;

    for (uint8_t i = 0; i < dm.solar.historyCount; ++i) {
        const SolarHistorySample& sample = dm.solar.history[i];
        points[i].x = static_cast<float>(sample.minuteOfDay) / 60.0f;
        points[i].y = sample.productionPowerW;
        if (sample.productionPowerW > maxPower) maxPower = sample.productionPowerW;
    }

    maxPower = ceilf(maxPower / 1000.0f) * 1000.0f;
    if (maxPower < 1000.0f) maxPower = 1000.0f;

    const int16_t graphX = 112;
    const int16_t graphY = 330;
    const int16_t graphW = 650;
    const int16_t graphH = 90;
    EInkGraph graph(display, graphX, graphY, graphW, graphH);
    graph.setXRange(0.0f, 24.0f);
    graph.setYRange(0.0f, maxPower);
    graph.drawHorizontalGrid(4);
    graph.drawVerticalGrid(4);
    graph.drawLineSeries(points, dm.solar.historyCount, false);

    ScreenStyle::useBody(display);
    for (uint8_t i = 0; i <= 4; ++i) {
        const float watts = maxPower - (maxPower * i / 4.0f);
        const int16_t y = graph.mapY(watts);
        display.setCursor(83, y + 5);
        if (watts >= 1000.0f) display.printf("%.0fk", watts / 1000.0f);
        else display.printf("%.0f", watts);
    }

    const uint8_t hours[] = {0, 6, 12, 18, 24};
    for (uint8_t i = 0; i < 5; ++i) {
        const int16_t x = graph.mapX(static_cast<float>(hours[i]));
        display.setCursor(x - (hours[i] >= 10 ? 10 : 5), 450);
        if (hours[i] == 24) display.print("24h");
        else display.printf("%u", hours[i]);
    }
}

} // namespace

void SolarScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);
    if (!dm.solar.enabled) {
        drawUnavailableCard(display, "FOTOVOLTAIKA", "GoodWe je v nastavení vypnutý.");
        return;
    }
    renderSolar(display, dm);
    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void SolarScreen::buildNavigationLayout(const DataModel&, NavigationLayout& layout) const {
    layout.clear();
    layout.add("goodwe-production", 65, 80, 220, 165);
    layout.add("goodwe-battery", 300, 80, 220, 165);
    layout.add("goodwe-grid", 535, 80, 250, 165);
    layout.add("goodwe-history", 65, 260, 720, 205);
}
