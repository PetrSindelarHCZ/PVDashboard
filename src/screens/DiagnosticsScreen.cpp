#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"
#include "../display/DashboardSans.h"

namespace {

constexpr uint8_t DashboardSansSizes[] = {
    8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36
};

constexpr int16_t FontGridX = 88;
constexpr int16_t FontGridY = 63;
constexpr int16_t FontColumnW = 337;
constexpr int16_t FontRowH = 62;
constexpr int16_t FontColumnGap = 12;
constexpr int16_t FontRowGap = 4;

void drawDashboardSansSample(
    IDisplay& display,
    int16_t x,
    int16_t y,
    uint8_t px) {

    display.drawRoundRect(x, y, FontColumnW, FontRowH, 4, 0);

    ScreenStyle::useStrongBody(display);
    display.setCursor(x + 8, y + 24);
    display.printf("%u", static_cast<unsigned>(px));

    const int16_t sampleX = x + 47;

    if (px <= 24) {
        DashboardSans::drawText(
            display, sampleX, y + 5,
            "Příliš žluťoučký kůň",
            px, DashboardSans::Weight::Regular);

        DashboardSans::drawText(
            display, sampleX, y + 31,
            "FVE 5.4 kW  Dnes 23.4 kWh",
            px, DashboardSans::Weight::Bold);
    } else {
        DashboardSans::drawText(
            display, sampleX, y + 12,
            "FVE 5.4 kW",
            px, DashboardSans::Weight::Bold);
    }
}

void renderDiagnosticsOverview(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawCard(display, 75, 63, 342, 402, "ESP32 A SÍŤ");
    ScreenStyle::useBody(display);
    display.setCursor(90, 135);
    display.printf("Firmware: %s", FIRMWARE_NAME);
    display.setCursor(90, 170);
    display.printf("Build: %s", FIRMWARE_BUILD_DATE);
    display.setCursor(90, 205);
    display.printf("Volná RAM: %lu KB", (unsigned long)(dm.system.freeHeapBytes / 1024));
    display.setCursor(90, 240);
    display.printf("Uptime: %lu s", (unsigned long)dm.system.uptimeSeconds);
    display.setCursor(90, 275);
    display.printf("WiFi: %s", dm.system.wifiConnected ? "Připojeno" : "Odpojeno");
    display.setCursor(90, 310);
    display.printf("IP: %s", dm.system.ipAddress.c_str());
    display.setCursor(90, 345);
    display.printf("Signál: %d dBm", dm.system.wifiRssi);
    display.setCursor(90, 380);
    display.printf("NTP: %s", dm.system.ntpSynced ? "Synchronizováno" : "Čeká na sync");

    ScreenStyle::drawCard(display, 427, 63, 358, 402, "INTEGRACE A SLUŽBY");
    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 135);
    display.print("GOODWE | UDP 8899");
    ScreenStyle::useBody(display);
    display.setCursor(457, 165);
    display.printf("Status: %s",
                   !dm.solar.enabled ? "Vypnuto" :
                   (dm.solar.status.available ? "Data dostupná" : "Nedostupné"));
    display.setCursor(457, 195);
    display.printf("Chyby: %u", dm.solar.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 235);
    display.print("AZ ROUTER | HTTP");
    ScreenStyle::useBody(display);
    display.setCursor(457, 265);
    display.printf("Status: %s",
                   !dm.azrouter.enabled ? "Vypnuto" :
                   (dm.azrouter.status.available ? "Data dostupná" : "Nedostupné"));
    display.setCursor(457, 295);
    display.printf("Chyby: %u", dm.azrouter.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 335);
    display.print("WEB SERVER");
    ScreenStyle::useBody(display);
    display.setCursor(457, 365);
    display.print("REST API / Mobile UI | port 80");
    display.setCursor(457, 395);
    display.printf("Obrazovka: %s", dm.system.currentScreenId.c_str());
}

void renderFontTest(IDisplay& display) {
    ScreenStyle::useStrongBody(display);
    display.setCursor(92, 58);
    display.print("DASHBOARD SANS v1");

    for (uint8_t i = 0; i < 12; ++i) {
        const uint8_t column = i / 6;
        const uint8_t row = i % 6;
        const int16_t x =
            FontGridX + static_cast<int16_t>(column) *
            (FontColumnW + FontColumnGap);
        const int16_t y =
            FontGridY + static_cast<int16_t>(row) *
            (FontRowH + FontRowGap);
        drawDashboardSansSample(display, x, y, DashboardSansSizes[i]);
    }
}

} // namespace

void DiagnosticsScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    const uint8_t subpage =
        dm.system.navigationSubpageIndex < 2
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 0) {
        renderDiagnosticsOverview(display, dm);
    } else {
        renderFontTest(display);
    }

    ScreenStyle::drawSubpageDots(display, subpage, 2);

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void DiagnosticsScreen::buildNavigationLayout(
    const DataModel& dm,
    NavigationLayout& layout) const {

    layout.clear();

    const uint8_t subpage =
        dm.system.navigationSubpageIndex < 2
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 0) {
        layout.add("system-card", 75, 63, 342, 402);
        layout.add("services-card", 427, 63, 358, 402);
        return;
    }

    for (uint8_t i = 0; i < 12; ++i) {
        const uint8_t column = i / 6;
        const uint8_t row = i % 6;
        layout.add(
            "font-" + String(DashboardSansSizes[i]),
            FontGridX + static_cast<int16_t>(column) *
                (FontColumnW + FontColumnGap),
            FontGridY + static_cast<int16_t>(row) *
                (FontRowH + FontRowGap),
            FontColumnW,
            FontRowH);
    }
}
