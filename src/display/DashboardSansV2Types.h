#pragma once
#include <Arduino.h>
#include "IDisplay.h"

namespace DashboardSansV2 {

enum class Weight : uint8_t { Regular, Bold };

struct Glyph {
    uint8_t code;
    uint8_t w;
    uint8_t h;
    uint8_t advance;
    int8_t xOffset;
    int8_t yOffset;
    uint16_t bitmapOffset;
};

struct Face {
    uint8_t px;
    uint8_t lineHeight;
    const Glyph* glyphs;
    uint8_t glyphCount;
    const uint8_t* bitmap;
};

inline bool readGlyph(const Face& face, char ch, Glyph& out) {
    const uint8_t code = static_cast<uint8_t>(ch);
    for (uint8_t i = 0; i < face.glyphCount; ++i) {
        memcpy_P(&out, &face.glyphs[i], sizeof(Glyph));
        if (out.code == code) return true;
    }
    return false;
}

inline void drawGlyph(
    IDisplay& display,
    int16_t x,
    int16_t top,
    const Face& face,
    const Glyph& glyph,
    uint16_t color = 0) {

    if (glyph.w == 0 || glyph.h == 0) return;

    for (uint8_t yy = 0; yy < glyph.h; ++yy) {
        for (uint8_t xx = 0; xx < glyph.w; ++xx) {
            const uint16_t bitIndex =
                static_cast<uint16_t>(yy) * glyph.w + xx;
            const uint8_t byte = pgm_read_byte(
                face.bitmap +
                glyph.bitmapOffset +
                bitIndex / 8u);

            if ((byte & (0x80u >> (bitIndex & 7u))) != 0) {
                display.drawPixel(
                    x + glyph.xOffset + xx,
                    top + glyph.yOffset + yy,
                    color);
            }
        }
    }
}

inline int16_t textWidth(
    const Face& face,
    const String& text) {

    int16_t width = 0;
    Glyph glyph;
    for (size_t i = 0; i < text.length(); ++i) {
        if (readGlyph(face, text[i], glyph)) {
            width += glyph.advance;
        }
    }
    return width;
}

inline void drawText(
    IDisplay& display,
    int16_t x,
    int16_t top,
    const Face& face,
    const String& text,
    uint16_t color = 0) {

    Glyph glyph;
    for (size_t i = 0; i < text.length(); ++i) {
        if (!readGlyph(face, text[i], glyph)) continue;
        drawGlyph(display, x, top, face, glyph, color);
        x += glyph.advance;
    }
}

} // namespace DashboardSansV2
