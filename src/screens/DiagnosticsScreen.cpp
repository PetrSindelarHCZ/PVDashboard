#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"
#include <U8g2_for_Adafruit_GFX.h>

namespace {

struct FontCandidate {
    const char* label;
    const char* note;
    const uint8_t* regular;
    const uint8_t* bold;
};

const FontCandidate FontCandidates[] = {
    {"A  T0", "současný", u8g2_font_t0_18_te, u8g2_font_t0_18b_te},
    {"B  Helvetica", "sans-serif", u8g2_font_helvR18_te, u8g2_font_helvB18_te},
    {"C  New Century", "serif", u8g2_font_ncenR18_te, u8g2_font_ncenB18_te},
    {"D  Lucida Sans", "sans-serif", u8g2_font_luRS18_te, u8g2_font_luBS18_te}
};

constexpr int16_t FontCardX = 88;
constexpr int16_t FontCardW = 684;
constexpr int16_t FontCardH = 88;
constexpr int16_t FontFirstY = 63;
constexpr int16_t FontGap = 9;

void drawFontCandidate(
    IDisplay& display,
    int16_t y,
    const FontCandidate& candidate) {

    display.drawRoundRect(FontCardX, y, FontCardW, FontCardH, 5, 0);

    // Labels remain in the current dashboard font; only the sample changes.
    ScreenStyle::useStrongBody(display);
    display.setCursor(FontCardX + 12, y + 27);
    display.print(candidate.label);

    ScreenStyle::useBody(display);
    display.setCursor(FontCardX + 12, y + 57);
    display.print(candidate.note);

    display.drawLine(
        FontCardX + 170, y + 8,
        FontCardX + 170, y + FontCardH - 8,
        0);

    display.setTextColor(0);
    display.setUnicodeFont(candidate.regular);
    display.setCursor(FontCardX + 185, y + 30);
    display.print("Příliš žluťoučký kůň");

    display.setUnicodeFont(candidate.bold);
    display.setCursor(FontCardX + 185, y + 66);
    display.print("FVE 5.4 kW   Dnes 23.4 kWh");
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
    for (uint8_t i = 0; i < 4; ++i) {
        drawFontCandidate(
            display,
            FontFirstY + static_cast<int16_t>(i) * (FontCardH + FontGap),
            FontCandidates[i]);
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

    for (uint8_t i = 0; i < 4; ++i) {
        layout.add(
            "font-" + String(static_cast<char>('a' + i)),
            FontCardX,
            FontFirstY + static_cast<int16_t>(i) * (FontCardH + FontGap),
            FontCardW,
            FontCardH);
    }
}
