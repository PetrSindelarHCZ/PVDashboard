#pragma once

#include <Arduino.h>

namespace SidebarIcons {

enum class Icon : uint8_t {
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

Bitmap get(Icon icon);

} // namespace SidebarIcons
