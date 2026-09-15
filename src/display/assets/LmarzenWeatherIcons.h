#pragma once

#include <Arduino.h>

namespace LmarzenWeatherIcons {

constexpr int16_t Size = 48;

// Returns a 48x48 monochrome bitmap copied from lmarzen/esp32-weather-epd.
// The source bitmaps use white=1 / black=0 and therefore must be rendered
// with drawInvertedBitmap().
const uint8_t* bitmapForWmo(uint8_t weatherCode);

} // namespace LmarzenWeatherIcons
