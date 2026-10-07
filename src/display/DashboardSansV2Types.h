#pragma once
#include <Arduino.h>
#include "IDisplay.h"

namespace DashboardSansV2 {

enum class Weight : uint8_t { Regular, Bold };

struct Glyph {
    uint16_t code;
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
    const char* bitmapBase64;
};

inline int8_t decodeBase64(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return 0;
}

inline uint8_t bitmapByte(const Face& face, uint16_t byteIndex) {
    const uint32_t group = byteIndex / 3u;
    const uint8_t rem = byteIndex % 3u;
    const uint32_t base = group * 4u;

    const uint8_t a = decodeBase64(
        static_cast<char>(pgm_read_byte(face.bitmapBase64 + base + 0)));
    const uint8_t b = decodeBase64(
        static_cast<char>(pgm_read_byte(face.bitmapBase64 + base + 1)));
    const uint8_t c = decodeBase64(
        static_cast<char>(pgm_read_byte(face.bitmapBase64 + base + 2)));
    const uint8_t d = decodeBase64(
        static_cast<char>(pgm_read_byte(face.bitmapBase64 + base + 3)));

    if (rem == 0) return static_cast<uint8_t>((a << 2) | (b >> 4));
    if (rem == 1) return static_cast<uint8_t>((b << 4) | (c >> 2));
    return static_cast<uint8_t>((c << 6) | d);
}

inline bool readGlyph(const Face& face, uint16_t code, Glyph& out) {
    for (uint8_t i = 0; i < face.glyphCount; ++i) {
        memcpy_P(&out, &face.glyphs[i], sizeof(Glyph));
        if (out.code == code) return true;
    }
    return false;
}

inline uint16_t nextUtf8Codepoint(const String& text, size_t& index) {
    if (index >= text.length()) return 0;

    const uint8_t b0 = static_cast<uint8_t>(text[index++]);
    if ((b0 & 0x80u) == 0) return b0;

    if ((b0 & 0xE0u) == 0xC0u && index < text.length()) {
        const uint8_t b1 = static_cast<uint8_t>(text[index++]);
        if ((b1 & 0xC0u) == 0x80u) {
            return static_cast<uint16_t>(
                ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu));
        }
        return '?';
    }

    if ((b0 & 0xF0u) == 0xE0u && index + 1 < text.length()) {
        const uint8_t b1 = static_cast<uint8_t>(text[index++]);
        const uint8_t b2 = static_cast<uint8_t>(text[index++]);
        if ((b1 & 0xC0u) == 0x80u && (b2 & 0xC0u) == 0x80u) {
            return static_cast<uint16_t>(
                ((b0 & 0x0Fu) << 12) |
                ((b1 & 0x3Fu) << 6) |
                (b2 & 0x3Fu));
        }
        return '?';
    }

    // The dashboard font tables intentionally cover BMP characters only.
    // Consume remaining UTF-8 continuation bytes of unsupported sequences.
    while (index < text.length() &&
           (static_cast<uint8_t>(text[index]) & 0xC0u) == 0x80u) {
        ++index;
    }
    return '?';
}

inline void drawGlyph(
    IDisplay& display,
    int16_t x,
    int16_t top,
    const Face& face,
    const Glyph& glyph,
    uint16_t color = 0) {

    if (glyph.w == 0 || glyph.h == 0) return;

    uint16_t cachedByteIndex = 0xFFFFu;
    uint8_t cachedByte = 0;

    for (uint8_t yy = 0; yy < glyph.h; ++yy) {
        for (uint8_t xx = 0; xx < glyph.w; ++xx) {
            const uint8_t stride =
                static_cast<uint8_t>((glyph.w + 7u) / 8u);
            const uint16_t localByteIndex =
                static_cast<uint16_t>(yy) * stride + (xx / 8u);

            if (localByteIndex != cachedByteIndex) {
                cachedByteIndex = localByteIndex;
                cachedByte = bitmapByte(
                    face,
                    glyph.bitmapOffset + localByteIndex);
            }

            if ((cachedByte & (0x80u >> (xx & 7u))) != 0) {
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
    size_t index = 0;
    while (index < text.length()) {
        const uint16_t codepoint = nextUtf8Codepoint(text, index);
        if (readGlyph(face, codepoint, glyph)) {
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
    size_t index = 0;
    while (index < text.length()) {
        const uint16_t codepoint = nextUtf8Codepoint(text, index);
        if (!readGlyph(face, codepoint, glyph)) continue;
        drawGlyph(display, x, top, face, glyph, color);
        x += glyph.advance;
    }
}

} // namespace DashboardSansV2
