#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"
#include "../display/DashboardSans.h"
#include "../display/assets/DashboardSansV2Reference.h"

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

    if (px <= 14) {
        DashboardSans::drawText(
            display, sampleX, y + 5,
            "Příliš žluťoučký kůň",
            px, DashboardSans::Weight::Regular);
        DashboardSans::drawText(
            display, sampleX, y + 31,
            "FVE 5.4 kW Dnes 23.4",
            px, DashboardSans::Weight::Bold);
    } else if (px <= 20) {
        DashboardSans::drawText(
            display, sampleX, y + 5,
            "Příliš žluťoučký",
            px, DashboardSans::Weight::Regular);
        DashboardSans::drawText(
            display, sampleX, y + 31,
            "FVE 5.4 kW",
            px, DashboardSans::Weight::Bold);
    } else if (px <= 24) {
        DashboardSans::drawText(
            display, sampleX, y + 5,
            "Český Brod",
            px, DashboardSans::Weight::Regular);
        DashboardSans::drawText(
            display, sampleX, y + 31,
            "5.4 kW",
            px, DashboardSans::Weight::Bold);
    } else {
        DashboardSans::drawText(
            display, sampleX, y + 12,
            "5.4 kW",
            px, DashboardSans::Weight::Bold);
    }
}

class ReferenceBase64Reader {
public:
    uint8_t readByte() {
        while (_bits < 8) {
            const char ch = DashboardSansV2Reference::Data[_charIndex++];
            if (ch == '\0') return 0;
            const int8_t value = decode(ch);
            if (value < 0) continue;
            _accumulator = (_accumulator << 6) | static_cast<uint8_t>(value);
            _bits += 6;
        }
        _bits -= 8;
        return static_cast<uint8_t>((_accumulator >> _bits) & 0xFFu);
    }

    uint32_t readVarint() {
        uint32_t value = 0;
        uint8_t shift = 0;
        while (true) {
            const uint8_t byte = readByte();
            value |= static_cast<uint32_t>(byte & 0x7Fu) << shift;
            if ((byte & 0x80u) == 0) return value;
            shift += 7;
        }
    }

private:
    static int8_t decode(char ch) {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    }

    size_t _charIndex = 0;
    uint32_t _accumulator = 0;
    uint8_t _bits = 0;
};

void renderDashboardSansV2Reference(IDisplay& display) {
    display.fillRect(
        0, 0,
        DashboardSansV2Reference::Width,
        DashboardSansV2Reference::Height,
        1);

    ReferenceBase64Reader reader;
    uint16_t x = 0;
    uint16_t y = 0;

    for (uint16_t i = 0; i < DashboardSansV2Reference::RectCount; ++i) {
        const uint16_t dy = static_cast<uint16_t>(reader.readVarint());
        const uint16_t dx = static_cast<uint16_t>(reader.readVarint());
        const uint16_t w = static_cast<uint16_t>(reader.readVarint());
        const uint16_t h = static_cast<uint16_t>(reader.readVarint());

        if (dy != 0) {
            y += dy;
            x = 0;
        }
        x += dx;
        display.fillRect(x, y, w, h, 0);
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
    const uint8_t subpage =
        dm.system.navigationSubpageIndex < 3
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 2) {
        // Exact 800x480 / 1-bit V2 reference. Do not draw dashboard chrome
        // over it; this page is intended for direct panel evaluation.
        renderDashboardSansV2Reference(display);
        return;
    }

    ScreenStyle::drawChrome(display, dm);

    if (subpage == 0) {
        renderDiagnosticsOverview(display, dm);
    } else {
        renderFontTest(display);
    }

    ScreenStyle::drawSubpageDots(display, subpage, 3);

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void DiagnosticsScreen::buildNavigationLayout(
    const DataModel& dm,
    NavigationLayout& layout) const {

    layout.clear();

    const uint8_t subpage =
        dm.system.navigationSubpageIndex < 3
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 0) {
        layout.add("system-card", 75, 63, 342, 402);
        layout.add("services-card", 427, 63, 358, 402);
        return;
    }

    if (subpage == 2) {
        // Exact reference page is visual-only; keep page focus empty.
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
