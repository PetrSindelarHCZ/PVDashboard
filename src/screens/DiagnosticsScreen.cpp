#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"
#include "../display/DashboardSansV2.h"
#include "../display/fonts/KpiFontCandidates.h"
#include "../display/assets/DashboardSansV2Reference.h"

namespace {

constexpr int16_t V2GridX = 88;
constexpr int16_t V2GridY = 66;
constexpr int16_t V2CardW = 684;
constexpr int16_t V2CardH = 61;
constexpr int16_t V2RowGap = 5;
constexpr int16_t V2LabelW = 66;
constexpr int16_t V2ColumnW = 292;

const uint8_t V2SmallSizes[] = {8, 10, 12, 14, 16, 18};
const uint8_t V2LargeSizes[] = {20, 22, 24, 28, 32, 36};

const char* sampleForSize(uint8_t px) {
    if (px <= 18) return "Dnes 23.4 kWh";
    if (px <= 24) return "FVE 5.4 kW";
    return "5.4 kW";
}

void renderFontMatrixPage(
    IDisplay& display,
    const uint8_t* sizes,
    const char* title) {

    ScreenStyle::useStrongBody(display);
    display.setCursor(92, 58);
    display.print(title);

    for (uint8_t i = 0; i < 6; ++i) {
        const uint8_t px = sizes[i];
        const int16_t y =
            V2GridY + static_cast<int16_t>(i) * (V2CardH + V2RowGap);

        display.drawRoundRect(
            V2GridX, y,
            V2CardW, V2CardH,
            4, 0);

        ScreenStyle::useStrongBody(display);
        display.setCursor(V2GridX + 8, y + 23);
        display.printf("%u px", static_cast<unsigned>(px));

        ScreenStyle::useBody(display);
        display.setCursor(V2GridX + 8, y + 48);
        display.print("R / B");

        display.drawLine(
            V2GridX + V2LabelW, y + 5,
            V2GridX + V2LabelW, y + V2CardH - 5,
            0);

        display.drawLine(
            V2GridX + V2LabelW + V2ColumnW, y + 5,
            V2GridX + V2LabelW + V2ColumnW, y + V2CardH - 5,
            0);

        const char* sample = sampleForSize(px);
        const int16_t regularX = V2GridX + V2LabelW + 10;
        const int16_t boldX =
            V2GridX + V2LabelW + V2ColumnW + 10;

        // Regular and Bold of the same nominal size share one visual
        // metrics box. The guides make cap-height/baseline mismatches obvious
        // on both WebUI preview and the real e-paper panel.
        const int16_t capTop =
            y + max<int16_t>(5, (V2CardH - px) / 2);
        const int16_t baseline = capTop + px;

        display.drawLine(
            regularX, capTop,
            regularX + V2ColumnW - 20, capTop,
            0);
        display.drawLine(
            boldX, capTop,
            boldX + V2ColumnW - 20, capTop,
            0);
        display.drawLine(
            regularX, baseline,
            regularX + V2ColumnW - 20, baseline,
            0);
        display.drawLine(
            boldX, baseline,
            boldX + V2ColumnW - 20, baseline,
            0);

        DashboardSansV2::drawText(
            display,
            regularX,
            capTop,
            sample,
            px,
            DashboardSansV2::Weight::Regular);

        DashboardSansV2::drawText(
            display,
            boldX,
            capTop,
            sample,
            px,
            DashboardSansV2::Weight::Bold);
    }
}


void renderKpiFontCandidatesPage(IDisplay& display) {
    struct Sample {
        uint8_t px;
        const DashboardSansV2::Face* face;
        const char* text;
    };

    const Sample samples[] = {
        {24, &KpiFontCandidates::Quantico24Face, "Příliš žluťoučký kůň"},
        {28, &KpiFontCandidates::Quantico28Face, "Český Brod"},
        {32, &KpiFontCandidates::Quantico32Face, "Výroba 5.4 kW"},
        {36, &KpiFontCandidates::Quantico36Face, "Síť 230 V"},
    };

    constexpr int16_t cardX = 104;
    constexpr int16_t cardW = 656;
    constexpr int16_t cardH = 82;
    constexpr int16_t firstY = 78;
    constexpr int16_t gap = 10;
    constexpr int16_t labelW = 112;

    ScreenStyle::useStrongBody(display);
    display.setCursor(108, 61);
    display.print("DASHBOARD KPI - QUANTICO CZ");

    for (uint8_t i = 0; i < 4; ++i) {
        const int16_t y = firstY + static_cast<int16_t>(i) * (cardH + gap);
        display.drawRoundRect(cardX, y, cardW, cardH, 5, 0);

        ScreenStyle::useStrongBody(display);
        display.setCursor(cardX + 12, y + 30);
        display.printf("%u BOLD", static_cast<unsigned>(samples[i].px));

        ScreenStyle::useBody(display);
        display.setCursor(cardX + 12, y + 58);
        display.print("1 BIT");

        const int16_t dividerX = cardX + labelW;
        display.drawLine(dividerX, y + 8, dividerX, y + cardH - 8, 0);

        DashboardSansV2::drawText(
            display,
            dividerX + 18,
            y + 14,
            *samples[i].face,
            samples[i].text);
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
        dm.system.navigationSubpageIndex < 5
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 4) {
        // Exact 800x480 / 1-bit V2 reference. Do not draw dashboard chrome
        // over it; this page is intended for direct panel evaluation.
        renderDashboardSansV2Reference(display);
        return;
    }

    ScreenStyle::drawChrome(display, dm);

    if (subpage == 0) {
        renderDiagnosticsOverview(display, dm);
    } else if (subpage == 1) {
        renderFontMatrixPage(
            display,
            V2SmallSizes,
            "DASHBOARD SANS V2 - 8..18");
    } else if (subpage == 2) {
        renderFontMatrixPage(
            display,
            V2LargeSizes,
            "DASHBOARD SANS V2 - 20..36");
    } else {
        renderKpiFontCandidatesPage(display);
    }

    ScreenStyle::drawSubpageDots(display, subpage, 5);

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void DiagnosticsScreen::buildNavigationLayout(
    const DataModel& dm,
    NavigationLayout& layout) const {

    layout.clear();

    const uint8_t subpage =
        dm.system.navigationSubpageIndex < 5
            ? dm.system.navigationSubpageIndex
            : 0;

    if (subpage == 0) {
        layout.add("system-card", 75, 63, 342, 402);
        layout.add("services-card", 427, 63, 358, 402);
        return;
    }

    if (subpage == 4) {
        // Exact reference page is visual-only; keep page focus empty.
        return;
    }

    if (subpage == 3) {
        constexpr int16_t cardX = 104;
        constexpr int16_t cardW = 656;
        constexpr int16_t cardH = 82;
        constexpr int16_t firstY = 78;
        constexpr int16_t gap = 10;

        for (uint8_t i = 0; i < 4; ++i) {
            layout.add(
                "font-kpi-quantico-" + String(i),
                cardX,
                firstY + static_cast<int16_t>(i) * (cardH + gap),
                cardW,
                cardH);
        }
        return;
    }

    const uint8_t* sizes =
        subpage == 1 ? V2SmallSizes : V2LargeSizes;

    for (uint8_t i = 0; i < 6; ++i) {
        layout.add(
            "font-v2-" + String(sizes[i]),
            V2GridX,
            V2GridY + static_cast<int16_t>(i) *
                (V2CardH + V2RowGap),
            V2CardW,
            V2CardH);
    }
}
