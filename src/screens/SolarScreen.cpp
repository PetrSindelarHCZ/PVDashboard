#include "SolarScreen.h"
#include "ScreenStyle.h"
#include "../display/EInkGraph.h"
#include <math.h>

namespace {

enum class SolarPage : uint8_t {
    Overview,
    GoodWe,
    AZRouter
};

uint8_t pageCount(const DataModel& dm) {
    uint8_t count = 1; // Přehled je vždy první.
    if (dm.solar.enabled) ++count;
    if (dm.azrouter.enabled) ++count;
    return count;
}

SolarPage pageForIndex(const DataModel& dm, uint8_t index) {
    if (index == 0) return SolarPage::Overview;
    uint8_t cursor = 1;
    if (dm.solar.enabled) {
        if (index == cursor) return SolarPage::GoodWe;
        ++cursor;
    }
    if (dm.azrouter.enabled && index == cursor) return SolarPage::AZRouter;
    return SolarPage::Overview;
}

const char* pageLabel(SolarPage page) {
    switch (page) {
        case SolarPage::Overview: return "PŘEHLED";
        case SolarPage::GoodWe: return "GOODWE";
        case SolarPage::AZRouter: return "AZROUTER";
    }
    return "";
}

const char* systemStatusText(const AZRouterData& az) {
    if (!az.status.available) return "Nedostupný";
    if (!az.hasSystemStatus) return "Online";
    switch (az.systemStatusCode) {
        case 0: return "Online";
        case 1: return "Offline";
        case 2: return "Aktualizace";
        default: return "Neznámý";
    }
}

const char* modeText(const AZRouterData& az) {
    if (!az.hasMode) return "--";
    switch (az.modeCode) {
        case 0: return "Summer";
        case 1: return "Winter";
        default: return "Neznámý";
    }
}

void drawPagerTabs(IDisplay& display, const DataModel& dm, uint8_t currentIndex) {
    const uint8_t count = pageCount(dm);
    if (count <= 1) return;

    constexpr int16_t left = ScreenStyle::ContentLeft;
    constexpr int16_t right = ScreenStyle::ContentRight;
    constexpr int16_t y = 62;
    constexpr int16_t h = 28;
    constexpr int16_t gap = 8;
    const int16_t available = right - left;
    const int16_t w = (available - static_cast<int16_t>((count - 1) * gap)) / count;

    for (uint8_t i = 0; i < count; ++i) {
        const int16_t x = left + i * (w + gap);
        const bool selected = i == currentIndex;
        if (selected) display.fillRoundRect(x, y, w, h, 5, 0);
        else display.drawRoundRect(x, y, w, h, 5, 0);

        ScreenStyle::useStrongBody(display, selected ? 1 : 0);
        const String label = pageLabel(pageForIndex(dm, i));
        const int16_t textX = x + (w - display.textWidth(label)) / 2;
        display.setCursor(textX, y + 20);
        display.print(label);
    }
}

void drawUnavailableCard(IDisplay& display, const char* title, const char* text) {
    ScreenStyle::drawCard(display, 75, 98, 710, 367, title);
    ScreenStyle::useMetric(display);
    display.setCursor(105, 180);
    display.print("Nedostupné");
    ScreenStyle::useBody(display);
    display.setCursor(105, 225);
    display.print(text);
}

void renderOverview(IDisplay& display, const DataModel& dm) {
    if (!dm.solar.enabled && dm.azrouter.enabled) {
        ScreenStyle::drawCard(display, 75, 98, 345, 170, "AZ ROUTER");
        ScreenStyle::useMetric(display);
        display.setCursor(95, 175);
        if (dm.azrouter.status.available && dm.azrouter.hasRoutedPower)
            display.printf("%.0f W", dm.azrouter.routedPowerW);
        else
            display.print("-- W");
        ScreenStyle::useBody(display);
        display.setCursor(95, 215);
        if (dm.azrouter.hasRoutedEnergyToday)
            display.printf("Dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);
        else
            display.print("Dnes: -- kWh");

        ScreenStyle::drawCard(display, 440, 98, 345, 170, "SÍŤ");
        ScreenStyle::useMetric(display);
        display.setCursor(460, 175);
        if (dm.azrouter.status.available && dm.azrouter.hasGridPower)
            display.printf("%+.0f W", dm.azrouter.gridPowerW);
        else
            display.print("-- W");
        ScreenStyle::useBody(display);
        display.setCursor(460, 215);
        display.printf("Stav: %s", systemStatusText(dm.azrouter));

        ScreenStyle::drawCard(display, 75, 283, 710, 182, "VYTĚŽOVÁNÍ PO FÁZÍCH");
        ScreenStyle::useValue(display);
        for (uint8_t phase = 0; phase < 3; ++phase) {
            const int16_t x = 105 + phase * 220;
            display.setCursor(x, 350);
            display.printf("L%u", phase + 1);
            ScreenStyle::useMetric(display);
            display.setCursor(x, 405);
            if (dm.azrouter.hasRoutedPhasePower[phase])
                display.printf("%.0f W", dm.azrouter.routedPhasePowerW[phase]);
            else
                display.print("-- W");
            ScreenStyle::useValue(display);
        }
        return;
    }

    // GoodWe summary.
    ScreenStyle::drawCard(display, 75, 98, 225, 160, "VÝROBA");
    ScreenStyle::useMetric(display);
    display.setCursor(90, 170);
    if (dm.solar.enabled && dm.solar.status.available)
        display.printf("%.0f W", dm.solar.productionPowerW);
    else
        display.print("-- W");
    ScreenStyle::useBody(display);
    display.setCursor(90, 215);
    if (dm.solar.enabled && dm.solar.status.available)
        display.printf("Dnes: %.1f kWh", dm.solar.energyTodayKWh);
    else
        display.print("Dnes: -- kWh");

    ScreenStyle::drawCard(display, 315, 98, 225, 160, "DŮM");
    ScreenStyle::useMetric(display);
    display.setCursor(330, 170);
    if (dm.solar.enabled && dm.solar.status.available)
        display.printf("%.0f W", dm.solar.houseConsumptionW);
    else
        display.print("-- W");
    ScreenStyle::useBody(display);
    display.setCursor(330, 215);
    display.printf("GoodWe: %s", dm.solar.status.available ? "Online" : "Offline");

    ScreenStyle::drawCard(display, 555, 98, 230, 160, "BATERIE");
    ScreenStyle::useMetric(display);
    display.setCursor(570, 170);
    if (dm.solar.enabled && dm.solar.status.available && dm.solar.batteryPresent)
        display.printf("%.0f %%", dm.solar.batterySocPercent);
    else if (dm.solar.enabled && dm.solar.status.available)
        display.print("Bez baterie");
    else
        display.print("-- %");
    ScreenStyle::useBody(display);
    display.setCursor(570, 215);
    if (dm.solar.enabled && dm.solar.status.available && dm.solar.batteryPresent)
        display.printf("%+.0f W", dm.solar.batteryPowerW);
    else if (dm.solar.enabled && dm.solar.status.available)
        display.print("Nepřipojena");
    else
        display.print("-- W");

    ScreenStyle::drawCard(display, 75, 273, 225, 192, "SÍŤ");
    ScreenStyle::useMetric(display);
    display.setCursor(90, 350);
    if (dm.solar.enabled && dm.solar.status.available)
        display.printf("%+.0f W", dm.solar.gridPowerW);
    else
        display.print("-- W");
    ScreenStyle::useBody(display);
    display.setCursor(90, 395);
    if (dm.solar.enabled && dm.solar.status.available)
        display.print(dm.solar.gridPowerW >= 0 ? "Přetok" : "Nákup");
    else
        display.print("Nedostupné");

    ScreenStyle::drawCard(display, 315, 273, 470, 192, "AZ ROUTER");
    ScreenStyle::useMetric(display);
    display.setCursor(335, 350);
    if (dm.azrouter.enabled && dm.azrouter.status.available && dm.azrouter.hasRoutedPower)
        display.printf("%.0f W", dm.azrouter.routedPowerW);
    else
        display.print("-- W");

    ScreenStyle::useBody(display);
    display.setCursor(335, 390);
    if (dm.azrouter.enabled && dm.azrouter.hasRoutedEnergyToday)
        display.printf("Dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);
    else
        display.print("Dnes: -- kWh");

    display.setCursor(335, 425);
    for (uint8_t phase = 0; phase < 3; ++phase) {
        if (phase > 0) display.print("   ");
        display.printf("L%u ", phase + 1);
        if (dm.azrouter.enabled && dm.azrouter.hasRoutedPhasePower[phase])
            display.printf("%.0f W", dm.azrouter.routedPhasePowerW[phase]);
        else
            display.print("-- W");
    }
}

void renderGoodWe(IDisplay& display, const DataModel& dm) {
    if (!dm.solar.enabled) {
        drawUnavailableCard(display, "GOODWE", "GoodWe je v nastavení vypnutý.");
        return;
    }

    ScreenStyle::drawCard(display, 75, 98, 225, 165, "SOLÁRNÍ VÝROBA");
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

    ScreenStyle::drawCard(display, 315, 98, 225, 165, "BATERIE");
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

    ScreenStyle::drawCard(display, 555, 98, 230, 165, "DISTRIBUCE");
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

    ScreenStyle::drawCard(display, 75, 278, 710, 187, "DNEŠNÍ PRŮBĚH FVE");

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

void renderAZPhase(IDisplay& display, const AZRouterData& az, uint8_t phase,
                   int16_t x, int16_t width) {
    String title = "L" + String(phase + 1);
    ScreenStyle::drawCard(display, x, 213, width, 160, title.c_str());

    ScreenStyle::useStrongBody(display);
    display.setCursor(x + 15, 258);
    if (az.hasGridPhaseStatus[phase])
        display.print(az.gridPhaseConnected[phase] ? "Připojena" : "Odpojena");
    else
        display.print("Stav: --");

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, 289);
    display.print("Síť: ");
    if (az.hasGridPhasePower[phase]) display.printf("%+.0f W", az.gridPhasePowerW[phase]);
    else display.print("-- W");
    display.print(" / ");
    if (az.hasGridPhaseCurrent[phase]) display.printf("%+.1f A", az.gridPhaseCurrentA[phase]);
    else display.print("-- A");

    display.setCursor(x + 15, 320);
    display.print("Napětí: ");
    if (az.hasGridPhaseVoltage[phase]) display.printf("%.1f V", az.gridPhaseVoltageV[phase]);
    else display.print("-- V");

    display.setCursor(x + 15, 351);
    display.print("Vytěženo: ");
    if (az.hasRoutedPhasePower[phase]) display.printf("%.0f W", az.routedPhasePowerW[phase]);
    else display.print("-- W");
}

void renderAZRouter(IDisplay& display, const DataModel& dm) {
    if (!dm.azrouter.enabled) {
        drawUnavailableCard(display, "AZ ROUTER", "AZRouter je v nastavení vypnutý.");
        return;
    }

    const AZRouterData& az = dm.azrouter;

    ScreenStyle::drawCard(display, 75, 98, 710, 105, "AZ ROUTER MASTER");
    ScreenStyle::useMetric(display);
    display.setCursor(95, 170);
    if (az.status.available && az.hasRoutedPower)
        display.printf("%.0f W", az.routedPowerW);
    else
        display.print("-- W");

    ScreenStyle::useBody(display);
    display.setCursor(300, 143);
    display.printf("Systém: %s", systemStatusText(az));
    display.setCursor(300, 174);
    display.printf("Režim: %s", modeText(az));

    display.setCursor(475, 143);
    if (az.hasHdo) display.printf("HDO: %s", az.hdoOn ? "ON" : "OFF");
    else display.print("HDO: --");
    display.setCursor(475, 174);
    if (az.hasMasterBoost) display.printf("Boost: %s", az.masterBoost ? "ON" : "OFF");
    else display.print("Boost: --");

    display.setCursor(625, 143);
    if (az.hasSystemTemp) display.printf("%.1f °C", az.systemTempC);
    else display.print("-- °C");
    display.setCursor(625, 174);
    display.printf("Auth: %s", az.authMode.c_str());

    renderAZPhase(display, az, 0, 75, 225);
    renderAZPhase(display, az, 1, 315, 225);
    renderAZPhase(display, az, 2, 555, 230);

    ScreenStyle::drawCard(display, 75, 383, 710, 82, "ULOŽENÁ ENERGIE");
    ScreenStyle::useBody(display);
    display.setCursor(95, 431);
    if (az.hasRoutedEnergyTotal) display.printf("Celkem %.0f kWh", az.routedEnergyTotalKWh);
    else display.print("Celkem -- kWh");

    display.setCursor(270, 431);
    if (az.hasRoutedEnergyYear) display.printf("Rok %.0f", az.routedEnergyYearKWh);
    else display.print("Rok --");

    display.setCursor(390, 431);
    if (az.hasRoutedEnergyMonth) display.printf("Měsíc %.0f", az.routedEnergyMonthKWh);
    else display.print("Měsíc --");

    display.setCursor(520, 431);
    if (az.hasRoutedEnergyWeek) display.printf("Týden %.0f", az.routedEnergyWeekKWh);
    else display.print("Týden --");

    display.setCursor(650, 431);
    if (az.hasRoutedEnergyToday) display.printf("Dnes %.0f", az.routedEnergyTodayKWh);
    else display.print("Dnes --");
}

} // namespace

uint8_t SolarScreen::getNavigationSubpageCount(const DataModel& dataModel) const {
    return pageCount(dataModel);
}

void SolarScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    if (!dm.solar.enabled && !dm.azrouter.enabled) {
        drawUnavailableCard(display, "FOTOVOLTAIKA", "GoodWe i AZRouter jsou v nastavení vypnuté.");
        return;
    }

    const uint8_t count = pageCount(dm);
    const uint8_t pageIndex =
        dm.system.navigationSubpageIndex < count
            ? dm.system.navigationSubpageIndex
            : 0;

    drawPagerTabs(display, dm, pageIndex);

    switch (pageForIndex(dm, pageIndex)) {
        case SolarPage::Overview:
            renderOverview(display, dm);
            break;
        case SolarPage::GoodWe:
            renderGoodWe(display, dm);
            break;
        case SolarPage::AZRouter:
            renderAZRouter(display, dm);
            break;
    }

    ScreenStyle::drawSubpageDots(display, pageIndex, count);

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void SolarScreen::buildNavigationLayout(const DataModel& dm, NavigationLayout& layout) const {
    layout.clear();

    const uint8_t count = pageCount(dm);
    const uint8_t pageIndex =
        dm.system.navigationSubpageIndex < count
            ? dm.system.navigationSubpageIndex
            : 0;

    switch (pageForIndex(dm, pageIndex)) {
        case SolarPage::Overview:
            if (!dm.solar.enabled && dm.azrouter.enabled) {
                layout.add("overview-az", 75, 98, 345, 170);
                layout.add("overview-grid", 440, 98, 345, 170);
                layout.add("overview-phases", 75, 283, 710, 182);
            } else {
                layout.add("overview-production", 75, 98, 225, 160);
                layout.add("overview-house", 315, 98, 225, 160);
                layout.add("overview-battery", 555, 98, 230, 160);
                layout.add("overview-grid", 75, 273, 225, 192);
                layout.add("overview-az", 315, 273, 470, 192);
            }
            break;

        case SolarPage::GoodWe:
            layout.add("goodwe-production", 75, 98, 225, 165);
            layout.add("goodwe-battery", 315, 98, 225, 165);
            layout.add("goodwe-grid", 555, 98, 230, 165);
            layout.add("goodwe-history", 75, 278, 710, 187);
            break;

        case SolarPage::AZRouter:
            layout.add("az-master", 75, 98, 710, 105);
            layout.add("az-l1", 75, 213, 225, 160);
            layout.add("az-l2", 315, 213, 225, 160);
            layout.add("az-l3", 555, 213, 230, 160);
            layout.add("az-energy", 75, 383, 710, 82);
            break;
    }
}
