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
    const uint8_t* data = nullptr;
    int16_t width = 0;
    int16_t height = 0;
};

Bitmap weather(uint8_t weatherCode, WeatherSize size);
Bitmap sidebar(SidebarIcon icon);

} // namespace IconAssets
