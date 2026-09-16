#pragma once

#include "../display/IDisplay.h"
#include "../display/DisplayFonts.h"
#include "../display/assets/Icons.h"
#include "../display/assets/LmarzenWeatherIcons.h"
#include "../data/DataModel.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

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

inline void drawCheck(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x, y + 4, x + 4, y + 8, c); d.drawLine(x + 4, y + 8, x + 11, y, c);
    d.drawLine(x, y + 5, x + 4, y + 9, c); d.drawLine(x + 4, y + 9, x + 11, y + 1, c);
}

inline void drawCross(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x, y, x + 9, y + 9, c); d.drawLine(x + 9, y, x, y + 9, c);
    d.drawLine(x + 1, y, x + 9, y + 8, c); d.drawLine(x + 8, y, x, y + 8, c);
}

inline uint8_t wifiLevel(const DataModel& dm) {
    if (!dm.system.wifiConnected) return 0;
    if (dm.system.wifiRssi >= -55) return 4;
    if (dm.system.wifiRssi >= -67) return 3;
    if (dm.system.wifiRssi >= -75) return 2;
    return 1;
}

inline void drawWifi(IDisplay& d, int16_t x, int16_t y, const DataModel& dm, uint16_t c) {
    const uint8_t level = wifiLevel(dm);
    d.fillCircle(x, y + 10, 2, c);
    if (level >= 2) { d.drawLine(x - 5, y + 7, x, y + 3, c); d.drawLine(x, y + 3, x + 5, y + 7, c); }
    if (level >= 3) { d.drawLine(x - 10, y + 3, x, y - 4, c); d.drawLine(x, y - 4, x + 10, y + 3, c); }
    if (level >= 4) { d.drawLine(x - 15, y - 2, x, y - 12, c); d.drawLine(x, y - 12, x + 15, y - 2, c); }
    if (!dm.system.wifiConnected) {
        d.drawLine(x - 14, y - 11, x + 14, y + 12, c);
        d.drawLine(x - 13, y - 12, x + 15, y + 11, c);
    }
}

inline void drawSourceStatus(IDisplay& d, int16_t x, const char* label, bool available) {
    d.setTextColor(1);
    d.setFont(&FreeSansBold9pt7b);
    d.setCursor(x, 31);
    d.print(label);
    if (available) drawCheck(d, x + 31, 18, 1); else drawCross(d, x + 32, 18, 1);
}

inline void drawHeader(IDisplay& d, const DataModel& dm) {
    d.fillRect(0, 0, Width, HeaderHeight, 0);
    drawWifi(d, 24, 25, dm, 1);
    const bool online = dm.system.wifiConnected;
    drawSourceStatus(d, 55, "GW", online && dm.solar.status.available);
    drawSourceStatus(d, 110, "AZ", online && dm.azrouter.status.available);

    d.setTextColor(1);
    d.setFont(&FreeSansBold9pt7b);
    d.setCursor(400, 31);
    d.print(dm.system.dateStr);

    d.setFont(&FreeSansBold18pt7b);
    d.setCursor(690, 35);
    d.print(dm.system.timeStr);
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
    (void)size; // First lmarzen iteration deliberately uses native 48x48 everywhere.
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

inline void drawMenuItem(IDisplay& d, int16_t y, const char* id, const char* shortLabel,
                         const DataModel& dm, IconAssets::SidebarIcon iconId) {
    const bool active = dm.system.currentScreenId.equalsIgnoreCase(id) ||
        (strcmp(id, "weather") == 0 && dm.system.currentScreenId.startsWith("weather-hourly-"));

    // Navigation-rail tile: enough room for a 32 px pictogram plus a short label.
    constexpr int16_t tileX = 4;
    constexpr int16_t tileW = 52;
    constexpr int16_t tileH = 68;
    constexpr int16_t tileRadius = 8;
    const int16_t tileY = y - tileH / 2;

    if (active) {
        d.fillRoundRect(tileX, tileY, tileW, tileH, tileRadius, 0);
        // Small white notch makes the current page easier to find at a glance.
        d.fillRoundRect(tileX + 1, y - 10, 3, 20, 1, 1);
    }

    const IconAssets::Bitmap icon = IconAssets::sidebar(iconId);
    if (icon.data != nullptr) {
        const int16_t iconCenterY = y - 10;
        d.drawBitmap(30 - icon.width / 2,
                     iconCenterY - icon.height / 2,
                     icon.data,
                     icon.width,
                     icon.height,
                     active ? 1 : 0);
    }

    // Keep labels deliberately short so they stay readable on the 60 px rail.
    d.setFont(&FreeSansBold9pt7b);
    d.setTextColor(active ? 1 : 0);
    const int16_t labelWidth = d.textWidth(String(shortLabel));
    int16_t labelX = (SidebarWidth - labelWidth) / 2;
    if (labelX < 2) labelX = 2;
    d.setCursor(labelX, y + 27);
    d.print(shortLabel);
}

inline void drawSidebar(IDisplay& d, const DataModel& dm) {
    d.drawLine(SidebarWidth, HeaderHeight, SidebarWidth, Height - 1, 0);

    // Five evenly spaced navigation destinations. Text is intentionally ASCII
    // because this rail uses the proven GFX font path (same as the header).
    drawMenuItem(d, 90,  "home",        "DOM", dm, IconAssets::SidebarIcon::Home);
    drawMenuItem(d, 172, "solar",       "FVE", dm, IconAssets::SidebarIcon::Solar);
    drawMenuItem(d, 254, "pool",        "BAZ", dm, IconAssets::SidebarIcon::Pool);
    drawMenuItem(d, 336, "weather",     "MET", dm, IconAssets::SidebarIcon::Weather);
    drawMenuItem(d, 418, "diagnostics", "SYS", dm, IconAssets::SidebarIcon::Settings);
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
