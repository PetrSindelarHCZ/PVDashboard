#pragma once

#include <Arduino.h>
#include "../IDisplay.h"

namespace WidgetIcons {

enum class Icon : uint8_t {
    Auto = 0,
    None,
    Temperature,
    Humidity,
    Pressure,
    Pool,
    Home,
    Outdoor,
    Solar,
    Power,
    Battery,
    Weather,
    Water,
    Heating,
    Fan,
    Wifi,
    Sensor,
    Count
};

inline const char* key(Icon icon) {
    switch (icon) {
        case Icon::Auto:        return "auto";
        case Icon::None:        return "none";
        case Icon::Temperature: return "temperature";
        case Icon::Humidity:    return "humidity";
        case Icon::Pressure:    return "pressure";
        case Icon::Pool:        return "pool";
        case Icon::Home:        return "home";
        case Icon::Outdoor:     return "outdoor";
        case Icon::Solar:       return "solar";
        case Icon::Power:       return "power";
        case Icon::Battery:     return "battery";
        case Icon::Weather:     return "weather";
        case Icon::Water:       return "water";
        case Icon::Heating:     return "heating";
        case Icon::Fan:         return "fan";
        case Icon::Wifi:        return "wifi";
        case Icon::Sensor:      return "sensor";
        case Icon::Count:       break;
    }
    return "auto";
}

inline const char* label(Icon icon) {
    switch (icon) {
        case Icon::Auto:        return "Automatická";
        case Icon::None:        return "Bez ikony";
        case Icon::Temperature: return "Teplota";
        case Icon::Humidity:    return "Vlhkost";
        case Icon::Pressure:    return "Tlak";
        case Icon::Pool:        return "Bazén";
        case Icon::Home:        return "Dům";
        case Icon::Outdoor:     return "Venku";
        case Icon::Solar:       return "FVE";
        case Icon::Power:       return "Výkon";
        case Icon::Battery:     return "Baterie";
        case Icon::Weather:     return "Počasí";
        case Icon::Water:       return "Voda";
        case Icon::Heating:     return "Topení";
        case Icon::Fan:         return "Ventilátor";
        case Icon::Wifi:        return "Wi-Fi";
        case Icon::Sensor:      return "Senzor";
        case Icon::Count:       break;
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

inline void drawHouse(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 3, y + 14, x + 16, y + 4, c);
    d.drawLine(x + 16, y + 4, x + 29, y + 14, c);
    d.drawLine(x + 6, y + 13, x + 6, y + 28, c);
    d.drawLine(x + 26, y + 13, x + 26, y + 28, c);
    d.drawLine(x + 6, y + 28, x + 26, y + 28, c);
    d.fillRect(x + 14, y + 20, 5, 8, c);
}

inline void drawTemperature(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawRoundRect(x + 12, y + 3, 8, 21, 4, c);
    d.drawLine(x + 16, y + 8, x + 16, y + 24, c);
    d.fillCircle(x + 16, y + 25, 5, c);
}

inline void drawDrop(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 16, y + 3, x + 8, y + 16, c);
    d.drawLine(x + 16, y + 3, x + 24, y + 16, c);
    d.drawLine(x + 8, y + 16, x + 8, y + 21, c);
    d.drawLine(x + 24, y + 16, x + 24, y + 21, c);
    d.drawCircle(x + 16, y + 21, 8, c);
    d.drawLine(x + 11, y + 25, x + 14, y + 27, c);
}

inline void drawPressure(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawCircle(x + 16, y + 16, 12, c);
    d.drawCircle(x + 16, y + 16, 11, c);
    d.drawLine(x + 16, y + 16, x + 22, y + 10, c);
    d.fillCircle(x + 16, y + 16, 2, c);
    d.drawLine(x + 8, y + 21, x + 24, y + 21, c);
}

inline void drawPool(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 9, y + 4, x + 9, y + 18, c);
    d.drawLine(x + 20, y + 4, x + 20, y + 18, c);
    d.drawLine(x + 9, y + 5, x + 20, y + 5, c);
    d.drawLine(x + 9, y + 11, x + 20, y + 11, c);
    for (int16_t o = 0; o < 2; ++o) {
        d.drawLine(x + 2, y + 23 + o, x + 7, y + 21 + o, c);
        d.drawLine(x + 7, y + 21 + o, x + 12, y + 23 + o, c);
        d.drawLine(x + 12, y + 23 + o, x + 17, y + 21 + o, c);
        d.drawLine(x + 17, y + 21 + o, x + 22, y + 23 + o, c);
        d.drawLine(x + 22, y + 23 + o, x + 29, y + 21 + o, c);
    }
}

inline void drawOutdoor(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 16, y + 8, x + 16, y + 28, c);
    d.fillCircle(x + 16, y + 29, 1, c);
    d.drawLine(x + 11, y + 11, x + 6, y + 6, c);
    d.drawLine(x + 21, y + 11, x + 26, y + 6, c);
    d.drawLine(x + 8, y + 17, x + 2, y + 11, c);
    d.drawLine(x + 24, y + 17, x + 30, y + 11, c);
}

inline void drawSolar(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawRect(x + 4, y + 11, 24, 14, c);
    d.drawLine(x + 10, y + 11, x + 8, y + 25, c);
    d.drawLine(x + 17, y + 11, x + 17, y + 25, c);
    d.drawLine(x + 24, y + 11, x + 26, y + 25, c);
    d.drawLine(x + 4, y + 18, x + 28, y + 18, c);
    d.drawCircle(x + 7, y + 6, 3, c);
}

inline void drawPower(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 18, y + 3, x + 9, y + 17, c);
    d.drawLine(x + 9, y + 17, x + 16, y + 17, c);
    d.drawLine(x + 16, y + 17, x + 13, y + 29, c);
    d.drawLine(x + 13, y + 29, x + 24, y + 14, c);
    d.drawLine(x + 24, y + 14, x + 17, y + 14, c);
    d.drawLine(x + 17, y + 14, x + 18, y + 3, c);
}

inline void drawBattery(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawRoundRect(x + 3, y + 9, 24, 15, 2, c);
    d.fillRect(x + 27, y + 13, 3, 7, c);
    d.fillRect(x + 7, y + 13, 16, 7, c);
}

inline void drawWeather(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawCircle(x + 11, y + 11, 6, c);
    d.drawLine(x + 11, y + 2, x + 11, y + 5, c);
    d.drawLine(x + 3, y + 11, x + 6, y + 11, c);
    d.drawLine(x + 16, y + 6, x + 19, y + 3, c);
    d.drawCircle(x + 19, y + 20, 8, c);
    d.drawCircle(x + 11, y + 22, 6, c);
    d.drawLine(x + 7, y + 27, x + 26, y + 27, c);
}

inline void drawHeating(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 9, y + 28, x + 9, y + 7, c);
    d.drawLine(x + 16, y + 28, x + 16, y + 4, c);
    d.drawLine(x + 23, y + 28, x + 23, y + 7, c);
    d.drawLine(x + 6, y + 28, x + 26, y + 28, c);
    d.drawLine(x + 7, y + 8, x + 11, y + 4, c);
    d.drawLine(x + 14, y + 5, x + 18, y + 1, c);
    d.drawLine(x + 21, y + 8, x + 25, y + 4, c);
}

inline void drawFan(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.fillCircle(x + 16, y + 16, 2, c);
    d.drawCircle(x + 16, y + 16, 12, c);
    d.fillCircle(x + 16, y + 8, 4, c);
    d.fillCircle(x + 23, y + 20, 4, c);
    d.fillCircle(x + 9, y + 20, 4, c);
}

inline void drawWifi(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawLine(x + 4, y + 12, x + 8, y + 8, c);
    d.drawLine(x + 8, y + 8, x + 16, y + 5, c);
    d.drawLine(x + 16, y + 5, x + 24, y + 8, c);
    d.drawLine(x + 24, y + 8, x + 28, y + 12, c);
    d.drawLine(x + 9, y + 17, x + 12, y + 14, c);
    d.drawLine(x + 12, y + 14, x + 16, y + 13, c);
    d.drawLine(x + 16, y + 13, x + 20, y + 14, c);
    d.drawLine(x + 20, y + 14, x + 23, y + 17, c);
    d.fillCircle(x + 16, y + 23, 2, c);
}

inline void drawSensor(IDisplay& d, int16_t x, int16_t y, uint16_t c) {
    d.drawRoundRect(x + 5, y + 5, 22, 22, 4, c);
    d.drawCircle(x + 16, y + 16, 6, c);
    d.fillCircle(x + 16, y + 16, 2, c);
    d.drawLine(x + 16, y + 2, x + 16, y + 5, c);
    d.drawLine(x + 16, y + 27, x + 16, y + 30, c);
    d.drawLine(x + 2, y + 16, x + 5, y + 16, c);
    d.drawLine(x + 27, y + 16, x + 30, y + 16, c);
}

inline void draw(IDisplay& d, Icon icon, int16_t x, int16_t y, uint16_t color = 0) {
    switch (icon) {
        case Icon::Temperature: drawTemperature(d, x, y, color); break;
        case Icon::Humidity:
        case Icon::Water:       drawDrop(d, x, y, color); break;
        case Icon::Pressure:    drawPressure(d, x, y, color); break;
        case Icon::Pool:        drawPool(d, x, y, color); break;
        case Icon::Home:        drawHouse(d, x, y, color); break;
        case Icon::Outdoor:     drawOutdoor(d, x, y, color); break;
        case Icon::Solar:       drawSolar(d, x, y, color); break;
        case Icon::Power:       drawPower(d, x, y, color); break;
        case Icon::Battery:     drawBattery(d, x, y, color); break;
        case Icon::Weather:     drawWeather(d, x, y, color); break;
        case Icon::Heating:     drawHeating(d, x, y, color); break;
        case Icon::Fan:         drawFan(d, x, y, color); break;
        case Icon::Wifi:        drawWifi(d, x, y, color); break;
        case Icon::Sensor:      drawSensor(d, x, y, color); break;
        case Icon::Auto:
        case Icon::None:
        case Icon::Count:       break;
    }
}

} // namespace WidgetIcons
