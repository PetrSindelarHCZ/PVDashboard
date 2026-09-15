#pragma once

#include <U8g2_for_Adafruit_GFX.h>

namespace DisplayFonts {

inline const uint8_t* body()        { return u8g2_font_helvR10_te; }
inline const uint8_t* strongBody()  { return u8g2_font_helvB10_te; }
inline const uint8_t* sectionTitle(){ return u8g2_font_helvB10_te; }
inline const uint8_t* title()       { return u8g2_font_helvB14_te; }
inline const uint8_t* value()       { return u8g2_font_helvB14_te; }
inline const uint8_t* metric()      { return u8g2_font_helvB18_te; }

} // namespace DisplayFonts
