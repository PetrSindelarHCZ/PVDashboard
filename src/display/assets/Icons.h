#pragma once

#include <Arduino.h>

namespace IconAssets {

enum class WeatherSize : uint8_t {
    Small24,
    Medium40,
    Large56
};

enum class SidebarIcon : uint8_t {
    Home,
    Solar,
    Pool,
    Weather,
    Settings
};

struct Bitmap {
    const uint8_t* data;
    int16_t width;
    int16_t height;

    Bitmap() : data(nullptr), width(0), height(0) {}
    Bitmap(const uint8_t* bitmapData, int16_t bitmapWidth, int16_t bitmapHeight)
        : data(bitmapData), width(bitmapWidth), height(bitmapHeight) {}
};

Bitmap weather(uint8_t weatherCode, WeatherSize size);
Bitmap sidebar(SidebarIcon icon);

} // namespace IconAssets
