#include "SolarScreen.h"
#include "ScreenStyle.h"
#include "../display/EInkGraph.h"
#include <math.h>

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
    display.setCursor(570, 215);
    display.printf("Dům: %.0f W", dm.solar.houseConsumptionW);

    ScreenStyle::drawCard(display, 75, 268, 470, 197, "DNEŠNÍ PRŮBĚH FVE");

    if (!dm.solar.status.available || dm.solar.historyCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 350);
        display.print("Čekám na historická data výroby.");
    } else {
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
        const int16_t graphY = 320;
        const int16_t graphW = 410;
        const int16_t graphH = 105;
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

    ScreenStyle::drawCard(display, 555, 268, 230, 197, "AZ ROUTER / STAV");
    ScreenStyle::useMetric(display);
    display.setCursor(570, 340);
    display.printf("%.0f W", dm.azrouter.routedPowerW);
    ScreenStyle::useBody(display);
    display.setCursor(570, 375);
    display.printf("Bojler: %.1f °C", dm.azrouter.boilerTempC);
    display.setCursor(570, 405);
    display.printf("Dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);
    display.setCursor(570, 438);
    display.printf("GW %s | AZ %s",
                   dm.solar.status.available ? "OK" : "OFF",
                   dm.azrouter.status.available ? "OK" : "OFF");
}
