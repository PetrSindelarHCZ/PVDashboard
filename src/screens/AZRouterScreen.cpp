#include "AZRouterScreen.h"
#include "ScreenStyle.h"

namespace {

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


void drawUnavailableCard(IDisplay& display, const char* title, const char* text) {
    ScreenStyle::drawCard(display, 50, 80, 750, 400, title);
    ScreenStyle::useMetric(display);
    display.setCursor(105, 180);
    display.print("Nedostupné");
    ScreenStyle::useBody(display);
    display.setCursor(105, 225);
    display.print(text);
}

void renderAZPhase(IDisplay& display, const AZRouterData& az, uint8_t phase,
                   int16_t x, int16_t width) {
    String title = "L" + String(phase + 1);
    ScreenStyle::drawCard(display, x, 210, width, 160, title.c_str());

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

    ScreenStyle::drawCard(display, 50, 80, 750, 105, "AZ ROUTER MASTER");
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

    ScreenStyle::drawCard(display, 50, 365, 750, 115, "ULOŽENÁ ENERGIE");
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

void AZRouterScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);
    renderAZRouter(display, dm);
    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void AZRouterScreen::buildNavigationLayout(const DataModel&, NavigationLayout& layout) const {
    layout.clear();
    layout.add("az-master", 50, 80, 750, 105);
    layout.add("az-l1", 50, 195, 225, 160);
    layout.add("az-l2", 290, 195, 225, 160);
    layout.add("az-l3", 530, 195, 270, 160);
    layout.add("az-energy", 50, 365, 750, 115);
}
