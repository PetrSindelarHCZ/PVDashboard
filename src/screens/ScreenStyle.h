#pragma once

#include "../display/IDisplay.h"
#include "../display/DisplayFonts.h"
#include "../display/assets/Icons.h"
#include "../display/assets/LmarzenWeatherIcons.h"
#include "../display/assets/SidebarIcons.h"
#include "../data/DataModel.h"
#include <Fonts/FreeSansBold18pt7b.h>


namespace ScreenStyle {

constexpr int16_t Width = 800;
constexpr int16_t Height = 480;
constexpr int16_t HeaderHeight = 48;
constexpr int16_t SidebarWidth = 60;
constexpr int16_t ContentLeft = 75;
constexpr int16_t ContentRight = 785;
constexpr int16_t ContentTop = 63;
constexpr int16_t ContentBottom = 465;
constexpr int16_t CardRadius = 6;

inline void useTitle(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::title()); }
inline void useSectionTitle(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::sectionTitle()); }
inline void useMetric(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::metric()); }
inline void useValue(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::value()); }
inline void useBody(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::body()); }
inline void useStrongBody(IDisplay& d) { d.setTextColor(0); d.setUnicodeFont(DisplayFonts::strongBody()); }

// All header symbols occupy a 32 x 32 box. The disconnected slash has a
// black outline so it stays distinct from the white strokes underneath.
inline void drawDisconnected(IDisplay& d, int16_t x, int16_t y) {
    for (int8_t offset = -3; offset <= 3; ++offset)
        d.drawLine(x + 3, y + 3 + offset, x + 28, y + 28 + offset, 0);
    for (int8_t offset = -1; offset <= 1; ++offset)
        d.drawLine(x + 3, y + 3 + offset, x + 28, y + 28 + offset, 1);
}

inline void drawWifi(IDisplay& d, int16_t x, int16_t y, bool connected, uint8_t level, bool accessPoint) {
    // Rasterized circular bands, clipped to the upper 90-degree sector.
    // Disconnected: keep the full symbol visible beneath the slash.
    const uint8_t arcs = accessPoint ? 3 : (connected ? (level == 0 ? 1 : level) : 3);
    for (int16_t dy = -23; dy <= -4; ++dy) {
        for (int16_t dx = -23; dx <= 23; ++dx) {
            if (abs(dx) > -dy) continue;
            const int16_t r2 = dx * dx + dy * dy;
            if ((arcs >= 3 && r2 >= 20 * 20 && r2 <= 23 * 23) ||
                (arcs >= 2 && r2 >= 13 * 13 && r2 <= 16 * 16) ||
                (r2 >= 6 * 6 && r2 <= 9 * 9))
                d.drawPixel(x + 16 + dx, y + 27 + dy, 1);
        }
    }
    d.fillCircle(x + 16, y + 27, 2, 1);
    if (accessPoint) {
        // Small 5x7 A/P glyphs on a black badge at the lower left.
        // Draw pixels directly so the badge never changes the text font state.
        static const uint8_t aRows[] = {14, 17, 17, 31, 17, 17, 17};
        static const uint8_t pRows[] = {30, 17, 17, 30, 16, 16, 16};
        d.fillRect(x, y + 23, 13, 9, 0);
        for (uint8_t row = 0; row < 7; ++row) {
            for (uint8_t col = 0; col < 5; ++col) {
                if (aRows[row] & (16 >> col)) d.drawPixel(x + 1 + col, y + 24 + row, 1);
                if (pRows[row] & (16 >> col)) d.drawPixel(x + 7 + col, y + 24 + row, 1);
            }
        }
    } else if (!connected) {
        drawDisconnected(d, x, y);
    }
}

inline void drawSolarStatus(IDisplay& d, int16_t x, int16_t y, bool available) {
    // Solar panel with cells and a stand (GoodWe).
    d.drawRoundRect(x + 2, y + 3, 28, 21, 2, 1);
    d.drawRoundRect(x + 3, y + 4, 26, 19, 1, 1);
    d.fillRect(x + 11, y + 5, 2, 17, 1);
    d.fillRect(x + 20, y + 5, 2, 17, 1);
    d.fillRect(x + 4, y + 12, 24, 2, 1);
    d.fillRect(x + 15, y + 24, 2, 4, 1);
    d.fillRect(x + 9, y + 28, 14, 2, 1);
    if (!available) drawDisconnected(d, x, y);
}

inline void drawRouterStatus(IDisplay& d, int16_t x, int16_t y, bool available) {
    // Heating element in a tank (AZRouter surplus-energy heating).
    d.drawRoundRect(x + 3, y + 2, 26, 28, 5, 1);
    d.drawRoundRect(x + 4, y + 3, 24, 26, 4, 1);
    d.fillRect(x + 9, y + 8, 14, 2, 1);
    d.fillRect(x + 21, y + 10, 2, 5, 1);
    d.fillRect(x + 9, y + 14, 14, 2, 1);
    d.fillRect(x + 9, y + 16, 2, 5, 1);
    d.fillRect(x + 9, y + 20, 14, 2, 1);
    if (!available) drawDisconnected(d, x, y);
}

inline void drawHeader(IDisplay& d, const DataModel& dm) {
    d.fillRect(0, 0, Width, HeaderHeight, 0);
    const bool online = dm.system.wifiConnected;
    drawWifi(d, 8, 8, online, dm.system.wifiSignalLevel, dm.system.wifiAccessPoint);
    drawSolarStatus(d, 56, 8, online && dm.solar.status.available);
    drawRouterStatus(d, 104, 8, online && dm.azrouter.status.available);

    // The header was cleared above, so invalid time also erases old e-ink text.
    if (!dm.system.ntpSynced) return;

    d.setTextColor(1);
    d.setFont(&FreeSansBold18pt7b);
    const int16_t timeX = Width - 12 - d.textWidth(dm.system.timeStr);
    d.setCursor(timeX, 35);
    d.print(dm.system.timeStr);

    d.setUnicodeFont(DisplayFonts::strongBody());
    String date = dm.system.dateStr;
    constexpr int16_t dateLeft = 160;
    const int16_t dateRight = timeX - 20;
    const int16_t availableWidth = dateRight - dateLeft;
    // Truncate only complete UTF-8 code points if a future label is too long.
    if (d.textWidth(date) > availableWidth) {
        while (date.length() && d.textWidth(date + "...") > availableWidth) {
            unsigned int end = date.length() - 1;
            while (end > 0 && (static_cast<uint8_t>(date[end]) & 0xC0) == 0x80) --end;
            date.remove(end);
        }
        date += "...";
    }
    d.setCursor(dateRight - d.textWidth(date), 31);
    d.print(date);
}

inline void drawBoldLine(IDisplay& d, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                         uint16_t c) {
    d.drawLine(x0, y0, x1, y1, c);
    if (abs(x1 - x0) >= abs(y1 - y0)) d.drawLine(x0, y0 + 1, x1, y1 + 1, c);
    else d.drawLine(x0 + 1, y0, x1 + 1, y1, c);
}

inline void drawWeatherSymbol(IDisplay& d, int16_t centerX, int16_t centerY,
                              uint8_t code,
                              IconAssets::WeatherSize size = IconAssets::WeatherSize::Medium40,
                              uint16_t color = 0) {
    (void)size;
    const uint8_t* bitmap = LmarzenWeatherIcons::bitmapForWmo(code);
    if (bitmap == nullptr) return;
    constexpr int16_t iconSize = LmarzenWeatherIcons::Size;
    d.drawInvertedBitmap(centerX - iconSize / 2,
                         centerY - iconSize / 2,
                         bitmap,
                         iconSize,
                         iconSize,
                         color);
}

inline void drawMenuItem(IDisplay& d, int16_t y, const char* id,
                         const DataModel& dm, SidebarIcons::Icon iconId) {
    const bool active = dm.system.currentScreenId.equalsIgnoreCase(id) ||
        (strcmp(id, "weather") == 0 && dm.system.currentScreenId.startsWith("weather-hourly-"));

    constexpr int16_t tileX = 4;
    constexpr int16_t tileW = 52;
    constexpr int16_t tileH = 58;
    constexpr int16_t tileRadius = 8;
    const int16_t tileY = y - tileH / 2;

    if (active) d.fillRoundRect(tileX, tileY, tileW, tileH, tileRadius, 0);

    const SidebarIcons::Bitmap icon = SidebarIcons::get(iconId);
    if (icon.data != nullptr) {
        d.drawBitmap(30 - icon.width / 2,
                     y - icon.height / 2,
                     icon.data,
                     icon.width,
                     icon.height,
                     active ? 1 : 0);
    }
}

inline void drawSidebar(IDisplay& d, const DataModel& dm) {
    d.drawLine(SidebarWidth, HeaderHeight, SidebarWidth, Height - 1, 0);

    // Horní položky skládáme těsně pod sebe od horního okraje sidebaru.
    // Dlaždice mají výšku 58 px; 2 px mezera dává krok 60 px.
    constexpr int16_t firstCenterY = HeaderHeight + 2 + 29;
    constexpr int16_t itemStep = 60;
    int16_t y = firstCenterY;

    drawMenuItem(d, y, "home",  dm, SidebarIcons::Icon::Home);  y += itemStep;
    drawMenuItem(d, y, "solar", dm, SidebarIcons::Icon::Solar); y += itemStep;
    drawMenuItem(d, y, "pool",  dm, SidebarIcons::Icon::Pool);  y += itemStep;

    if (dm.weather.enabled) {
        drawMenuItem(d, y, "weather", dm, SidebarIcons::Icon::Weather);
    }

    // Nastavení / diagnostika zůstává vždy zarovnané ke spodnímu okraji.
    constexpr int16_t settingsCenterY = Height - 1 - 29;
    drawMenuItem(d, settingsCenterY, "diagnostics", dm, SidebarIcons::Icon::Settings);
}

inline void drawChrome(IDisplay& d, const DataModel& dm) {
    drawHeader(d, dm);
    drawSidebar(d, dm);
}

inline void drawCard(IDisplay& d, int16_t x, int16_t y, int16_t w, int16_t h, const char* title) {
    d.drawRoundRect(x, y, w, h, CardRadius, 0);
    useSectionTitle(d);
    d.setCursor(x + 12, y + 27);
    d.print(title);
    d.drawLine(x + 10, y + 34, x + w - 10, y + 34, 0);
}

} // namespace ScreenStyle
