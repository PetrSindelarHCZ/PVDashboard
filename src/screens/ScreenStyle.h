#pragma once

#include "../display/IDisplay.h"
#include "../display/DisplayFonts.h"
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
    d.setCursor(535, 31);
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

inline void drawBoldRect(IDisplay& d, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
    d.drawRect(x, y, w, h, c);
    d.drawRect(x + 1, y + 1, w - 2, h - 2, c);
}

inline void drawBoldCircle(IDisplay& d, int16_t x, int16_t y, int16_t r, uint16_t c) {
    d.drawCircle(x, y, r, c);
    d.drawCircle(x, y, r - 1, c);
}
inline bool isFogCode(uint8_t code) {
    return code == 45 || code == 48;
}
inline bool isRainCode(uint8_t code) {
    return (code >= 51 && code <= 67) || (code >= 80 && code <= 82);
}
inline bool isSnowCode(uint8_t code) {
    return (code >= 71 && code <= 77) || code == 85 || code == 86;
}
inline bool isThunderstormCode(uint8_t code) {
    return code >= 95;
}
inline void drawSunSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldCircle(d, x, y, 9, c);
    drawBoldCircle(d, x, y, 4, c);
    drawBoldLine(d, x, y - 17, x, y - 13, c);
    drawBoldLine(d, x, y + 13, x, y + 17, c);
    drawBoldLine(d, x - 17, y, x - 13, y, c);
    drawBoldLine(d, x + 13, y, x + 17, y, c);
    drawBoldLine(d, x - 12, y - 12, x - 9, y - 9, c);
    drawBoldLine(d, x + 9, y + 9, x + 12, y + 12, c);
    drawBoldLine(d, x + 9, y - 9, x + 12, y - 12, c);
    drawBoldLine(d, x - 12, y + 12, x - 9, y + 9, c);
}
inline void drawCloudSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldCircle(d, x - 11, y + 4, 8, c);
    drawBoldCircle(d, x, y - 2, 12, c);
    drawBoldCircle(d, x + 13, y + 3, 9, c);
    drawBoldLine(d, x - 22, y + 11, x + 23, y + 11, c);
    drawBoldLine(d, x - 19, y + 5, x - 19, y + 11, c);
    drawBoldLine(d, x + 22, y + 4, x + 22, y + 11, c);
}
inline void drawRainSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldLine(d, x - 11, y, x - 15, y + 8, c);
    drawBoldLine(d, x, y, x - 4, y + 8, c);
    drawBoldLine(d, x + 11, y, x + 7, y + 8, c);
}
inline void drawSnowSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    for (int16_t offset = -10; offset <= 10; offset += 10) {
        d.drawLine(x + offset - 3, y, x + offset + 3, y + 6, c);
        d.drawLine(x + offset + 3, y, x + offset - 3, y + 6, c);
        d.drawLine(x + offset, y - 1, x + offset, y + 7, c);
    }
}
inline void drawWeatherSymbol(IDisplay& d, int16_t x, int16_t y, uint8_t code, uint16_t c = 0) {
    if (isFogCode(code)) {
        drawBoldLine(d, x - 21, y - 9, x + 16, y - 9, c);
        drawBoldLine(d, x - 15, y, x + 22, y, c);
        drawBoldLine(d, x - 21, y + 9, x + 13, y + 9, c);
        return;
    }
    if (code == 0) {
        drawSunSymbol(d, x, y, c);
        return;
    }
    if (code == 1 || code == 2) {
        drawSunSymbol(d, x - 9, y - 8, c);
        drawCloudSymbol(d, x + 6, y + 7, c);
    } else {
        drawCloudSymbol(d, x, y - 3, c);
    }
    if (isThunderstormCode(code)) {
        drawBoldLine(d, x + 2, y + 10, x - 5, y + 21, c);
        drawBoldLine(d, x - 5, y + 21, x + 2, y + 20, c);
        drawBoldLine(d, x + 2, y + 20, x - 4, y + 31, c);
    } else if (isSnowCode(code)) {
        drawSnowSymbol(d, x, y + 16, c);
    } else if (isRainCode(code)) {
        drawRainSymbol(d, x, y + 15, c);
    }
}

inline void drawHomeIcon(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldLine(d, x - 17, y - 2, x, y - 17, c);
    drawBoldLine(d, x, y - 17, x + 17, y - 2, c);
    drawBoldRect(d, x - 12, y - 2, 24, 18, c);
    drawBoldRect(d, x - 4, y + 6, 8, 10, c);
}

inline void drawSolarIcon(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldRect(d, x - 17, y - 13, 34, 22, c);
    drawBoldLine(d, x - 6, y - 13, x - 6, y + 9, c);
    drawBoldLine(d, x + 6, y - 13, x + 6, y + 9, c);
    drawBoldLine(d, x - 17, y - 2, x + 17, y - 2, c);
    drawBoldLine(d, x, y + 9, x, y + 15, c);
    drawBoldLine(d, x - 11, y + 15, x + 11, y + 15, c);
}

inline void drawPoolIcon(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    for (int16_t row = -10; row <= 10; row += 10) {
        drawBoldLine(d, x - 18, y + row, x - 11, y + row - 3, c);
        drawBoldLine(d, x - 11, y + row - 3, x - 3, y + row, c);
        drawBoldLine(d, x - 3, y + row, x + 5, y + row + 3, c);
        drawBoldLine(d, x + 5, y + row + 3, x + 12, y + row, c);
        drawBoldLine(d, x + 12, y + row, x + 18, y + row - 2, c);
    }
}

inline void drawWeatherIcon(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawSunSymbol(d, x - 8, y - 7, c);
    drawCloudSymbol(d, x + 6, y + 8, c);
}

inline void drawGearIcon(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    drawBoldCircle(d, x, y, 12, c);
    drawBoldCircle(d, x, y, 4, c);
    drawBoldLine(d, x, y - 18, x, y - 12, c);
    drawBoldLine(d, x, y + 12, x, y + 18, c);
    drawBoldLine(d, x - 18, y, x - 12, y, c);
    drawBoldLine(d, x + 12, y, x + 18, y, c);
    drawBoldLine(d, x - 13, y - 13, x - 9, y - 9, c);
    drawBoldLine(d, x + 9, y + 9, x + 13, y + 13, c);
    drawBoldLine(d, x + 9, y - 9, x + 13, y - 13, c);
    drawBoldLine(d, x - 13, y + 13, x - 9, y + 9, c);
}
inline void drawMenuItem(IDisplay& d, int16_t y, const char* id, const DataModel& dm, uint8_t icon) {
    const bool active = dm.system.currentScreenId.equalsIgnoreCase(id) ||
        (strcmp(id, "weather") == 0 && dm.system.currentScreenId.startsWith("weather-hourly-"));
    if (active) d.fillRoundRect(6, y - 28, 48, 56, 7, 0);
    const uint16_t c = active ? 1 : 0;
    if (icon == 0) drawHomeIcon(d, 30, y, c);
    else if (icon == 1) drawSolarIcon(d, 30, y, c);
    else if (icon == 2) drawPoolIcon(d, 30, y, c);
    else if (icon == 3) drawWeatherIcon(d, 30, y, c);
    else drawGearIcon(d, 30, y, c);
}
inline void drawSidebar(IDisplay& d, const DataModel& dm) {
    d.drawLine(SidebarWidth, HeaderHeight, SidebarWidth, Height - 1, 0);
    drawMenuItem(d, 88, "home", dm, 0); drawMenuItem(d, 170, "solar", dm, 1);
    drawMenuItem(d, 252, "pool", dm, 2); drawMenuItem(d, 334, "weather", dm, 3);
    drawMenuItem(d, 416, "diagnostics", dm, 4);
}
inline void drawChrome(IDisplay& d, const DataModel& dm) { drawHeader(d, dm); drawSidebar(d, dm); }

inline void drawCard(IDisplay& d, int16_t x, int16_t y, int16_t w, int16_t h, const char* title) {
    d.drawRoundRect(x, y, w, h, CardRadius, 0);
    useSectionTitle(d); d.setCursor(x + 12, y + 27); d.print(title);
    d.drawLine(x + 10, y + 34, x + w - 10, y + 34, 0);
}

} // namespace ScreenStyle
