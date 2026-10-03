#pragma once

#include <Arduino.h>
#include "../IDisplay.h"
#include "SidebarIcons.h"

namespace WidgetIcons {

// Keep this list deliberately limited to icons that are already used and
// visually approved elsewhere in the dashboard. New artwork should be added
// only after it has been reviewed in the UI.
enum class Icon : uint8_t {
    Auto = 0,
    None,
    Home,
    Outdoor,
    Pool,
    Solar,
    AZRouter,
    Weather,
    Count
};

inline const char* key(Icon icon) {
    switch (icon) {
        case Icon::Auto:     return "auto";
        case Icon::None:     return "none";
        case Icon::Home:     return "home";
        case Icon::Outdoor:  return "outdoor";
        case Icon::Pool:     return "pool";
        case Icon::Solar:    return "solar";
        case Icon::AZRouter: return "azrouter";
        case Icon::Weather:  return "weather";
        case Icon::Count:    break;
    }
    return "auto";
}

inline const char* label(Icon icon) {
    switch (icon) {
        case Icon::Auto:     return "Automatická";
        case Icon::None:     return "Bez ikony";
        case Icon::Home:     return "Dům";
        case Icon::Outdoor:  return "Venku / RF čidlo";
        case Icon::Pool:     return "Bazén";
        case Icon::Solar:    return "FVE";
        case Icon::AZRouter: return "AZRouter";
        case Icon::Weather:  return "Počasí";
        case Icon::Count:    break;
    }
    return "";
}

inline Icon fromKey(const String& value) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(Icon::Count); ++i) {
        const Icon icon = static_cast<Icon>(i);
        if (value.equalsIgnoreCase(key(icon))) return icon;
    }
    return Icon::Auto;
}

inline bool valid(uint8_t value) {
    return value < static_cast<uint8_t>(Icon::Count);
}

inline Icon defaultForWidgetType(const String& type) {
    if (type == "weather") return Icon::Weather;
    if (type == "fve-summary" || type == "energy") return Icon::Solar;
    if (type == "azrouter-summary") return Icon::AZRouter;
    if (type == "indoor") return Icon::Home;
    if (type == "rf-sensor") return Icon::Outdoor;
    if (type == "pool-summary") return Icon::Pool;
    if (type == "consumption-summary") return Icon::Home;

    // A generic custom card did not historically have an approved icon.
    return Icon::None;
}

inline Icon resolved(uint8_t configured, const String& type) {
    if (!valid(configured)) return defaultForWidgetType(type);
    const Icon icon = static_cast<Icon>(configured);
    return icon == Icon::Auto ? defaultForWidgetType(type) : icon;
}

inline bool sidebarIcon(Icon icon, SidebarIcons::Icon& result) {
    switch (icon) {
        case Icon::Home:
            result = SidebarIcons::Icon::Home;
            return true;
        case Icon::Pool:
            result = SidebarIcons::Icon::Pool;
            return true;
        case Icon::Solar:
            result = SidebarIcons::Icon::Solar;
            return true;
        case Icon::AZRouter:
            result = SidebarIcons::Icon::AZRouter;
            return true;
        case Icon::Weather:
            result = SidebarIcons::Icon::Weather;
            return true;
        default:
            return false;
    }
}

inline void drawApprovedSidebarIcon(
    IDisplay& d,
    SidebarIcons::Icon iconId,
    int16_t x,
    int16_t y,
    uint16_t color) {

    const SidebarIcons::Bitmap icon = SidebarIcons::get(iconId);
    if (icon.data == nullptr || icon.width != 40 || icon.height != 40) return;

    // Same 40 -> 32 raster reduction that the approved Home card headers
    // used before WidgetIcons existed.
    constexpr int16_t target = 32;
    constexpr int16_t sourceRowBytes = 5;

    for (int16_t ty = 0; ty < target; ++ty) {
        for (int16_t tx = 0; tx < target; ++tx) {
            const int16_t sx0 = (tx * 40) / target;
            const int16_t sx1 = ((tx + 1) * 40) / target;
            const int16_t sy0 = (ty * 40) / target;
            const int16_t sy1 = ((ty + 1) * 40) / target;
            bool set = false;

            for (int16_t sy = sy0; sy < sy1 && !set; ++sy) {
                for (int16_t sx = sx0; sx < sx1; ++sx) {
                    const uint8_t value =
                        pgm_read_byte(icon.data + sy * sourceRowBytes + sx / 8);
                    if (value & (0x80 >> (sx & 7))) {
                        set = true;
                        break;
                    }
                }
            }
            if (set) d.drawPixel(x + tx, y + ty, color);
        }
    }
}

inline void drawOutdoorApproved(
    IDisplay& d,
    int16_t x,
    int16_t y,
    uint16_t color) {

    // Exact RF header symbol that was used on the approved Home RF card.
    d.drawLine(x + 15, y + 9, x + 15, y + 29, color);
    d.fillCircle(x + 15, y + 30, 1, color);
    d.drawLine(x + 10, y + 11, x + 5, y + 6, color);
    d.drawLine(x + 20, y + 11, x + 25, y + 6, color);
    d.drawLine(x + 7, y + 16, x + 1, y + 10, color);
    d.drawLine(x + 23, y + 16, x + 29, y + 10, color);
}

inline void draw(
    IDisplay& d,
    Icon icon,
    int16_t x,
    int16_t y,
    uint16_t color = 0) {

    if (icon == Icon::None || icon == Icon::Auto || icon == Icon::Count) return;

    if (icon == Icon::Outdoor) {
        drawOutdoorApproved(d, x, y, color);
        return;
    }

    SidebarIcons::Icon sidebarId = SidebarIcons::Icon::Home;
    if (sidebarIcon(icon, sidebarId))
        drawApprovedSidebarIcon(d, sidebarId, x, y, color);
}

} // namespace WidgetIcons
