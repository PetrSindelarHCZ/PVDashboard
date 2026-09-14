#pragma once

#include "../display/IDisplay.h"
#include "../data/DataModel.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

namespace ScreenStyle {

constexpr int16_t Width = 800;
constexpr int16_t HeaderHeight = 48;
constexpr int16_t FooterY = 430;
constexpr int16_t CardRadius = 6;

inline void useTitle(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSansBold12pt7b);
}

inline void useSectionTitle(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSansBold9pt7b);
}

inline void useMetric(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSansBold18pt7b);
}

inline void useValue(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSansBold12pt7b);
}

inline void useBody(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSans9pt7b);
}

inline void useStrongBody(IDisplay& display) {
    display.setTextColor(0);
    display.setFont(&FreeSansBold9pt7b);
}

inline void drawHeader(IDisplay& display, const char* title, const DataModel& dm) {
    display.fillRect(0, 0, Width, HeaderHeight, 0);
    display.setTextColor(1);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 33);
    display.print(title);

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(520, 32);
    display.print(dm.system.dateStr);

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(690, 35);
    display.print(dm.system.timeStr);
}

inline void drawCard(IDisplay& display, int16_t x, int16_t y, int16_t w, int16_t h,
                     const char* title) {
    display.drawRoundRect(x, y, w, h, CardRadius, 0);
    useSectionTitle(display);
    display.setCursor(x + 12, y + 27);
    display.print(title);
    display.drawLine(x + 10, y + 34, x + w - 10, y + 34, 0);
}

inline void drawFooter(IDisplay& display, const String& left, const String& right = "") {
    display.fillRect(0, FooterY, Width, 480 - FooterY, 0);
    display.setTextColor(1);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 462);
    display.print(left);

    if (right.length() > 0) {
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(560, 462);
        display.print(right);
    }
}

} // namespace ScreenStyle