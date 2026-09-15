#pragma once

#include <U8g2_for_Adafruit_GFX.h>

namespace DisplayFonts {

// Czech-capable Unicode font family. The t0 fonts contain the Latin Extended-A
// glyphs needed for Czech UTF-8 text.
inline const uint8_t* body()         { return u8g2_font_t0_18_te; }
inline const uint8_t* strongBody()   { return u8g2_font_t0_18b_te; }
inline const uint8_t* sectionTitle() { return u8g2_font_t0_16b_te; }
inline const uint8_t* title()        { return u8g2_font_t0_22b_te; }
inline const uint8_t* value()        { return u8g2_font_t0_22b_te; }
inline const uint8_t* metric()       { return u8g2_font_t0_22b_te; }

} // namespace DisplayFonts
