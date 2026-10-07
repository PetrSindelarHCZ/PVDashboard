#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"
#include "../display/DashboardSansV2.h"
#include "../display/assets/DashboardSansV2Reference.h"

namespace {

constexpr int16_t V2GridX = 88;
constexpr int16_t V2GridY = 66;
constexpr int16_t V2CardW = 335;
constexpr int16_t V2CardH = 122;
constexpr int16_t V2ColumnGap = 12;
constexpr int16_t V2RowGap = 8;

struct V2Sample {
    uint8_t px;
    DashboardSansV2::Weight weight;
    const char* label;
    const char* text;
};

const V2Sample V2Samples[] = {
    {12, DashboardSansV2::Weight::Regular, "12 REGULAR", "Dnes 23.4 kWh"},
    {16, DashboardSansV2::Weight::Regular, "16 REGULAR", "Dnes 23.4 kWh"},
    {16, DashboardSansV2::Weight::Bold,    "16 BOLD",    "FVE 5.4 kW"},
    {22, DashboardSansV2::Weight::Bold,    "22 BOLD",    "FVE 5.4 kW"},
    {28, DashboardSansV2::Weight::Bold,    "28 BOLD",    "23.4 kWh"},
    {36, DashboardSansV2::Weight::Bold,    "36 BOLD",    "5.4 kW"},
};

void renderFontTest(IDisplay& display) {
    ScreenStyle::useStrongBody(display);
    display.setCursor(92, 58);
    display.print("DASHBOARD SANS V2 - FAZE A");

    for (uint8_t i = 0; i < 6; ++i) {
        const uint8_t column = i % 2;
        const uint8_t row = i / 2;
        const int16_t x =
            V2GridX + static_cast<int16_t>(column) *
            (V2CardW + V2ColumnGap);
        const int16_t y =
            V2GridY + static_cast<int16_t>(row) *
            (V2CardH + V2RowGap);

        display.drawRoundRect(x, y, V2CardW, V2CardH, 4, 0);

        ScreenStyle::useBody(display);
        display.setCursor(x + 10, y + 25);
        display.print(V2Samples[i].label);

        display.drawLine(
            x + 8, y + 34,
            x + V2CardW - 8, y + 34,
            0);

        DashboardSansV2::drawText(
            display,
            x + 14,
            y + 46,
            V2Samples[i].text,
            V2Samples[i].px,
            V2Samples[i].weight);
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

    for (uint8_t i = 0; i < 6; ++i) {
        const uint8_t column = i % 2;
        const uint8_t row = i / 2;
        layout.add(
            "font-v2-" + String(V2Samples[i].px) + "-" + String(i),
            V2GridX + static_cast<int16_t>(column) *
                (V2CardW + V2ColumnGap),
            V2GridY + static_cast<int16_t>(row) *
                (V2CardH + V2RowGap),
            V2CardW,
            V2CardH);
    }
}
