#pragma once

#include <U8g2_for_Adafruit_GFX.h>

namespace DisplayFonts {

// The Adobe Helvetica BDFs used by U8g2 do not contain the full Czech Latin
// Extended-A character set, even when the generated font has the _te suffix.
// The t0 Unicode BDF family does contain these glyphs (čďěňřšťž and uppercase
// variants), so use it for all UI text that may contain Czech UTF-8.
//
// Content fonts are intentionally one step larger than the first t0 iteration;
// the original 11/14/18 px set looked too small on the 800x480 7.5" panel.
inline const uint8_t* body()         { return u8g2_font_t0_14_te; }
inline const uint8_t* strongBody()   { return u8g2_font_t0_14b_te; }
inline const uint8_t* sectionTitle() { return u8g2_font_t0_14b_te; }
inline const uint8_t* title()        { return u8g2_font_t0_16b_te; }
inline const uint8_t* value()        { return u8g2_font_t0_16b_te; }
inline const uint8_t* metric()       { return u8g2_font_t0_22b_te; }

// Header has its own roles so future tuning of content typography cannot break
// the fixed 48 px top bar.
inline const uint8_t* headerStatus() { return u8g2_font_t0_11b_te; }
inline const uint8_t* headerDate()   { return u8g2_font_t0_11b_te; }
inline const uint8_t* headerTime()   { return u8g2_font_t0_18b_te; }

} // namespace DisplayFonts
